#pragma once

#include <gdiplus.h>
#include <map>
#include <memory>
#include <set>
#include <string>

// 保存物品 ID 与 JAR 中 PNG 的对应关系；图片在首次显示时才解压和解码。
class CItemTextureStore
{
public:
	CItemTextureStore();
	~CItemTextureStore();

	void Register(const CString& strItem, const CString& strJarPath, const std::string& strZipPath);
	BOOL Draw(CDC* pDC, const CRect& rect, const CString& strItem);

private:
	struct TextureSource
	{
		CString strJarPath;
		std::string strZipPath;
	};

	Gdiplus::Bitmap* GetBitmap(const std::wstring& strItem);

	ULONG_PTR m_gdiplusToken;
	std::map<std::wstring, TextureSource> m_sources;
	std::map<std::wstring, std::unique_ptr<Gdiplus::Bitmap>> m_bitmaps;
	std::set<std::wstring> m_failed;
};
