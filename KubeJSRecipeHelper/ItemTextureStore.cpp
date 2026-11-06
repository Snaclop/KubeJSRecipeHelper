#include "pch.h"
#include "ItemTextureStore.h"
#include "third_party/miniz/miniz.h"

#include <algorithm>
#include <objidl.h>

#pragma comment(lib, "gdiplus.lib")

CItemTextureStore::CItemTextureStore()
	: m_uGdiplusToken(0)
{
	Gdiplus::GdiplusStartupInput input;
	Gdiplus::GdiplusStartup(&m_uGdiplusToken, &input, nullptr);
}

CItemTextureStore::~CItemTextureStore()
{
	m_mapBitmaps.clear();
	if (m_uGdiplusToken != 0)
		Gdiplus::GdiplusShutdown(m_uGdiplusToken);
}

void CItemTextureStore::Register(const CString& strItem, const CString& strJarPath,
	const std::string& strZipPath)
{
	const std::wstring strKey(strItem.GetString());
	m_mapSources[strKey] = { strJarPath, strZipPath };
	m_mapBitmaps.erase(strKey);
	m_setFailed.erase(strKey);
}

Gdiplus::Bitmap* CItemTextureStore::GetBitmap(const std::wstring& strItem)
{
	if (m_uGdiplusToken == 0 || m_setFailed.count(strItem) != 0)
		return nullptr;

	auto itBitmap = m_mapBitmaps.find(strItem);
	if (itBitmap != m_mapBitmaps.end())
		return itBitmap->second.get();

	auto itSource = m_mapSources.find(strItem);
	if (itSource == m_mapSources.end())
		return nullptr;

	const TextureSource& source = itSource->second;
	mz_zip_archive zip = {};
	if (!mz_zip_reader_init_file(&zip, CT2A(source.strJarPath, CP_UTF8), 0))
	{
		m_setFailed.insert(strItem);
		return nullptr;
	}

	const int nIndex = mz_zip_reader_locate_file(&zip, source.strZipPath.c_str(), nullptr, 0);
	size_t nSize = 0;
	void* pData = nIndex >= 0 ? mz_zip_reader_extract_to_heap(&zip, nIndex, &nSize, 0) : nullptr;
	mz_zip_reader_end(&zip);

	// 不把异常大的资源交给图片解码器。
	if (pData == nullptr || nSize == 0 || nSize > 16 * 1024 * 1024)
	{
		if (pData != nullptr)
			mz_free(pData);
		m_setFailed.insert(strItem);
		return nullptr;
	}

	IStream* pStream = nullptr;
	if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &pStream)))
	{
		mz_free(pData);
		m_setFailed.insert(strItem);
		return nullptr;
	}

	ULONG uWritten = 0;
	const HRESULT hrWrite = pStream->Write(pData, static_cast<ULONG>(nSize), &uWritten);
	mz_free(pData);
	LARGE_INTEGER position = {};
	const HRESULT hrSeek = pStream->Seek(position, STREAM_SEEK_SET, nullptr);

	std::unique_ptr<Gdiplus::Bitmap> pBitmap;
	if (SUCCEEDED(hrWrite) && uWritten == nSize && SUCCEEDED(hrSeek))
	{
		std::unique_ptr<Gdiplus::Bitmap> pDecoded(Gdiplus::Bitmap::FromStream(pStream));
		if (pDecoded && pDecoded->GetLastStatus() == Gdiplus::Ok &&
			pDecoded->GetWidth() > 0 && pDecoded->GetHeight() > 0 &&
			pDecoded->GetWidth() <= 4096 && pDecoded->GetHeight() <= 4096)
		{
			pBitmap.reset(pDecoded->Clone(0, 0, pDecoded->GetWidth(), pDecoded->GetHeight(),
				PixelFormat32bppARGB));
			if (pBitmap && pBitmap->GetLastStatus() != Gdiplus::Ok)
				pBitmap.reset();
		}
	}
	pStream->Release();

	if (!pBitmap)
	{
		m_setFailed.insert(strItem);
		return nullptr;
	}

	return m_mapBitmaps.emplace(strItem, std::move(pBitmap)).first->second.get();
}

BOOL CItemTextureStore::Draw(CDC* pDC, const CRect& rcRect, const CString& strItem)
{
	Gdiplus::Bitmap* pBitmap = GetBitmap(std::wstring(strItem.GetString()));
	if (pBitmap == nullptr)
		return FALSE;

	const UINT uSourceWidth = pBitmap->GetWidth();
	UINT uSourceHeight = pBitmap->GetHeight();
	// 动画贴图纵向拼接多帧，只显示第一帧。
	if (uSourceHeight >= uSourceWidth * 2 && uSourceHeight % uSourceWidth == 0)
		uSourceHeight = uSourceWidth;

	CRect rcImage(rcRect);
	rcImage.DeflateRect(3, 3);
	if (rcImage.IsRectEmpty())
		return FALSE;

	const double dScale = (std::min)(static_cast<double>(rcImage.Width()) / uSourceWidth,
		static_cast<double>(rcImage.Height()) / uSourceHeight);
	const int nWidth = (std::max)(1, static_cast<int>(uSourceWidth * dScale));
	const int nHeight = (std::max)(1, static_cast<int>(uSourceHeight * dScale));
	const int nLeft = rcImage.left + (rcImage.Width() - nWidth) / 2;
	const int nTop = rcImage.top + (rcImage.Height() - nHeight) / 2;

	Gdiplus::Graphics graphics(pDC->GetSafeHdc());
	graphics.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
	graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
	return graphics.DrawImage(pBitmap, Gdiplus::Rect(nLeft, nTop, nWidth, nHeight),
		0, 0, uSourceWidth, uSourceHeight, Gdiplus::UnitPixel) == Gdiplus::Ok;
}
