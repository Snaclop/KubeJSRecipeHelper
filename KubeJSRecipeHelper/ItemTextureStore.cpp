#include "pch.h"
#include "ItemTextureStore.h"
#include "miniz.h"

#include <algorithm>
#include <objidl.h>

#pragma comment(lib, "gdiplus.lib")

CItemTextureStore::CItemTextureStore()
	: m_gdiplusToken(0)
{
	Gdiplus::GdiplusStartupInput input;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &input, nullptr);
}

CItemTextureStore::~CItemTextureStore()
{
	m_bitmaps.clear();
	if (m_gdiplusToken != 0)
		Gdiplus::GdiplusShutdown(m_gdiplusToken);
}

void CItemTextureStore::Register(const CString& strItem, const CString& strJarPath,
	const std::string& strZipPath)
{
	const std::wstring strKey(strItem.GetString());
	m_sources[strKey] = { strJarPath, strZipPath };
	m_bitmaps.erase(strKey);
	m_failed.erase(strKey);
}

Gdiplus::Bitmap* CItemTextureStore::GetBitmap(const std::wstring& strItem)
{
	if (m_gdiplusToken == 0 || m_failed.count(strItem) != 0)
		return nullptr;

	auto itBitmap = m_bitmaps.find(strItem);
	if (itBitmap != m_bitmaps.end())
		return itBitmap->second.get();

	auto itSource = m_sources.find(strItem);
	if (itSource == m_sources.end())
		return nullptr;

	const TextureSource& source = itSource->second;
	mz_zip_archive zip = {};
	if (!mz_zip_reader_init_file(&zip, CT2A(source.strJarPath, CP_UTF8), 0))
	{
		m_failed.insert(strItem);
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
		m_failed.insert(strItem);
		return nullptr;
	}

	IStream* pStream = nullptr;
	if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &pStream)))
	{
		mz_free(pData);
		m_failed.insert(strItem);
		return nullptr;
	}

	ULONG nWritten = 0;
	const HRESULT hrWrite = pStream->Write(pData, static_cast<ULONG>(nSize), &nWritten);
	mz_free(pData);
	LARGE_INTEGER pos = {};
	const HRESULT hrSeek = pStream->Seek(pos, STREAM_SEEK_SET, nullptr);

	std::unique_ptr<Gdiplus::Bitmap> bitmap;
	if (SUCCEEDED(hrWrite) && nWritten == nSize && SUCCEEDED(hrSeek))
	{
		std::unique_ptr<Gdiplus::Bitmap> decoded(Gdiplus::Bitmap::FromStream(pStream));
		if (decoded && decoded->GetLastStatus() == Gdiplus::Ok &&
			decoded->GetWidth() > 0 && decoded->GetHeight() > 0 &&
			decoded->GetWidth() <= 4096 && decoded->GetHeight() <= 4096)
		{
			bitmap.reset(decoded->Clone(0, 0, decoded->GetWidth(), decoded->GetHeight(),
				PixelFormat32bppARGB));
			if (bitmap && bitmap->GetLastStatus() != Gdiplus::Ok)
				bitmap.reset();
		}
	}
	pStream->Release();

	if (!bitmap)
	{
		m_failed.insert(strItem);
		return nullptr;
	}

	return m_bitmaps.emplace(strItem, std::move(bitmap)).first->second.get();
}

BOOL CItemTextureStore::Draw(CDC* pDC, const CRect& rect, const CString& strItem)
{
	Gdiplus::Bitmap* pBitmap = GetBitmap(std::wstring(strItem.GetString()));
	if (pBitmap == nullptr)
		return FALSE;

	const UINT nSourceWidth = pBitmap->GetWidth();
	UINT nSourceHeight = pBitmap->GetHeight();
	// 动画贴图纵向拼接多帧，只显示第一帧。
	if (nSourceHeight >= nSourceWidth * 2 && nSourceHeight % nSourceWidth == 0)
		nSourceHeight = nSourceWidth;

	CRect rcImage(rect);
	rcImage.DeflateRect(3, 3);
	if (rcImage.IsRectEmpty())
		return FALSE;

	const double scale = (std::min)(static_cast<double>(rcImage.Width()) / nSourceWidth,
		static_cast<double>(rcImage.Height()) / nSourceHeight);
	const int nWidth = (std::max)(1, static_cast<int>(nSourceWidth * scale));
	const int nHeight = (std::max)(1, static_cast<int>(nSourceHeight * scale));
	const int nLeft = rcImage.left + (rcImage.Width() - nWidth) / 2;
	const int nTop = rcImage.top + (rcImage.Height() - nHeight) / 2;

	Gdiplus::Graphics graphics(pDC->GetSafeHdc());
	graphics.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
	graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
	return graphics.DrawImage(pBitmap, Gdiplus::Rect(nLeft, nTop, nWidth, nHeight),
		0, 0, nSourceWidth, nSourceHeight, Gdiplus::UnitPixel) == Gdiplus::Ok;
}
