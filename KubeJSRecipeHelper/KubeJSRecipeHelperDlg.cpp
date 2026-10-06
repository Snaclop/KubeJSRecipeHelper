
// KubeJSRecipeHelperDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "KubeJSRecipeHelper.h"
#include "KubeJSRecipeHelperDlg.h"
#include "afxdialogex.h"
#include "CDlgRecipeChanges.h"
#include "miniz.h"
#include <atlbase.h>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CKubeJSRecipeHelperDlg 对话框



CKubeJSRecipeHelperDlg::CKubeJSRecipeHelperDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_KUBEJSRECIPEHELPER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CKubeJSRecipeHelperDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CKubeJSRecipeHelperDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTNVANILLA, &CKubeJSRecipeHelperDlg::OnBnClickedBtnvanilla)
	ON_BN_CLICKED(IDC_BTNCREATE, &CKubeJSRecipeHelperDlg::OnBnClickedBtncreate)
	ON_BN_CLICKED(IDC_BTNIMPORT, &CKubeJSRecipeHelperDlg::OnBnClickedBtnimport)
	ON_BN_CLICKED(IDC_BTNCLEAR, &CKubeJSRecipeHelperDlg::OnBnClickedBtnclear)
	ON_BN_CLICKED(IDC_BTNDWNLOAD, &CKubeJSRecipeHelperDlg::OnBnClickedBtndwnload)
	ON_BN_CLICKED(IDC_BTNOTHER, &CKubeJSRecipeHelperDlg::OnBnClickedBtnother)
	ON_BN_CLICKED(IDC_BTNREMOVE, &CKubeJSRecipeHelperDlg::OnBnClickedBtnremove)
	ON_BN_CLICKED(IDC_BTNMODIFY, &CKubeJSRecipeHelperDlg::OnBnClickedBtnmodify)
END_MESSAGE_MAP()


// CKubeJSRecipeHelperDlg 消息处理程序

BOOL CKubeJSRecipeHelperDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CKubeJSRecipeHelperDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CKubeJSRecipeHelperDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CKubeJSRecipeHelperDlg::ShowRecipeScript(const CString& strScript)
{
	// 输出框里只留刚生成的脚本，接着就能用“输出到 js 文件”把它写出去
	if (!strScript.IsEmpty())
		SetDlgItemText(IDC_EDITOUTPUT, strScript);
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnvanilla()
{
	CDlgVanilla dlg(this);
	dlg.SetItemSource(&m_arrNames);
	dlg.SetCatalog(&m_entryTypes);
	dlg.SetTextureStore(&m_itemTextures);
	if (dlg.DoModal() != IDOK)
		return;

	ShowRecipeScript(dlg.GetRecipeScript());
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtncreate()
{
	CKubeJSRecipeHelperApp* pApp = (CKubeJSRecipeHelperApp*)AfxGetApp();
	const BOOL previous = pApp->m_bIsCreate;
	pApp->m_bIsCreate = TRUE;
	CDlgCreate dlg(this);
	dlg.SetItemSource(&m_arrNames);
	dlg.SetCatalog(&m_entryTypes);
	dlg.SetTextureStore(&m_itemTextures);
	const INT_PTR result = dlg.DoModal();
	pApp->m_bIsCreate = previous;
	if (result == IDOK)
		ShowRecipeScript(dlg.GetRecipeScript());
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnimport()
{
	CFileDialog dlgFile(TRUE, _T("jar"), nullptr,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT,
		_T("JAR 文件 (*.jar)|*.jar|所有文件 (*.*)|*.*||"), this);
	if (dlgFile.DoModal() != IDOK)
		return;

	// 直接枚举系统对话框的全部结果，避免固定文件名缓冲区截断多选列表。
	CComPtr<IShellItemArray> selectedFiles;
	selectedFiles.Attach(dlgFile.GetResults());
	DWORD fileCount = 0;
	if (!selectedFiles || FAILED(selectedFiles->GetCount(&fileCount)))
	{
		AfxMessageBox(_T("无法读取所选文件列表。"), MB_ICONERROR);
		return;
	}

	CStringArray paths;
	for (DWORD i = 0; i < fileCount; ++i)
	{
		CComPtr<IShellItem> item;
		CComHeapPtr<wchar_t> path;
		if (FAILED(selectedFiles->GetItemAt(i, &item)) ||
			FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)))
		{
			AfxMessageBox(_T("无法读取所选文件的路径。"), MB_ICONERROR);
			return;
		}
		paths.Add(CString(path));
	}

	CString failures;
	int failedCount = 0;
	{
		CWaitCursor wait;
		// 按选择列表顺序解析，一个文件完成后才处理下一个。
		for (INT_PTR i = 0; i < paths.GetSize(); ++i)
		{
			CString progress;
			progress.Format(_T("正在解析 %d/%d：\r\n%s"),
				(int)i + 1, (int)paths.GetSize(), paths[i].GetString());
			SetDlgItemText(IDC_EDITOUTPUT, progress);
			GetDlgItem(IDC_EDITOUTPUT)->UpdateWindow();

			CString* pNames = nullptr;
			const int nCount = ParseJarModels(paths[i], pNames);
			if (nCount < 0)
			{
				++failedCount;
				failures += paths[i] + _T("\r\n");
			}
			else
			{
				for (int n = 0; n < nCount; ++n)
					m_arrNames.Add(pNames[n]);
			}
			delete[] pNames;
		}
	}

	CString summary;
	summary.Format(_T("导入完成：成功 %d 个 JAR，失败 %d 个。\r\n\r\n"),
		(int)paths.GetSize() - failedCount, failedCount);
	RefreshNamesOutput(summary);
	if (failedCount > 0)
		AfxMessageBox(summary + _T("以下文件解析失败，已继续处理其他文件：\r\n") + failures,
			MB_ICONWARNING);
}

void CKubeJSRecipeHelperDlg::RefreshNamesOutput(const CString& strSummary)
{
	CString strOutput;
	strOutput.Format(_T("共找到 %d 个 item：\r\n"), (int)m_arrNames.GetSize());
	strOutput = strSummary + strOutput;
	for (INT_PTR i = 0; i < m_arrNames.GetSize(); ++i)
	{
		strOutput += m_arrNames[i];
		strOutput += _T("\r\n");
	}
	SetDlgItemText(IDC_EDITOUTPUT, strOutput);
}

namespace
{
	const std::string strJsonExt = ".json";
	const std::string strPngExt = ".png";

	// 模型文件的拆分结果
	struct ModelPath
	{
		std::string strNamespace;	// 命名空间
		std::string strKind;		// item 或 block
		std::string strRelative;	// models/<kind>/ 之后的相对路径（含 .json）
	};

	// 是不是以 .json 结尾的文件名
	BOOL IsJsonFile(const std::string& strName)
	{
		return strName.size() > strJsonExt.size() &&
			strName.compare(strName.size() - strJsonExt.size(), strJsonExt.size(), strJsonExt) == 0;
	}

	// 去掉结尾的 .json
	std::string WithoutJsonExt(const std::string& strName)
	{
		return strName.substr(0, strName.size() - strJsonExt.size());
	}

	// 把 "assets/create/models/item/wrench/item.json" 拆成 (create, item, wrench/item.json)。
	// 不是 assets/<命名空间>/models/<item|block>/… 形式的路径返回 FALSE。
	BOOL SplitModelPath(const char* pszPath, ModelPath& model)
	{
		const std::string strPath(pszPath);
		const std::string strAssets = "assets/";
		const std::string strModels = "models/";

		if (strPath.compare(0, strAssets.size(), strAssets) != 0)
			return FALSE;

		// 命名空间
		const size_t nNamespaceEnd = strPath.find('/', strAssets.size());
		if (nNamespaceEnd == std::string::npos)
			return FALSE;

		// models/
		const size_t nModels = nNamespaceEnd + 1;
		if (strPath.compare(nModels, strModels.size(), strModels) != 0)
			return FALSE;

		// item/ 或 block/
		const size_t nKind = nModels + strModels.size();
		const size_t nKindEnd = strPath.find('/', nKind);
		if (nKindEnd == std::string::npos)
			return FALSE;

		const std::string strKind = strPath.substr(nKind, nKindEnd - nKind);
		if (strKind != "item" && strKind != "block")
			return FALSE;

		const std::string strRelative = strPath.substr(nKindEnd + 1);
		if (!IsJsonFile(strRelative))
			return FALSE;

		model.strNamespace = strPath.substr(strAssets.size(), nNamespaceEnd - strAssets.size());
		model.strKind = strKind;
		model.strRelative = strRelative;
		return TRUE;
	}

	// 把模型引用（"minecraft:item/clock"、"item/clock"、"create:block/cogwheel"）拆成模型路径。
	// 引用一般不写 .json，没写命名空间时按 minecraft 算，跟游戏里的规则一致；
	// 不是 <item|block>/… 形式的引用（builtin/entity 之类）返回 FALSE。
	BOOL SplitModelRef(const std::string& strRef, ModelPath& model)
	{
		std::string strNamespace = "minecraft";
		std::string strPath = strRef;

		const size_t nColon = strRef.find(':');
		if (nColon != std::string::npos)
		{
			strNamespace = strRef.substr(0, nColon);
			strPath = strRef.substr(nColon + 1);
		}

		if (IsJsonFile(strPath))
			strPath = WithoutJsonExt(strPath);

		const size_t nSlash = strPath.find('/');
		if (nSlash == std::string::npos)
			return FALSE;

		const std::string strKind = strPath.substr(0, nSlash);
		if (strKind != "item" && strKind != "block")
			return FALSE;

		const std::string strName = strPath.substr(nSlash + 1);
		if (strName.empty())
			return FALSE;

		model.strNamespace = strNamespace;
		model.strKind = strKind;
		model.strRelative = strName + strJsonExt;
		return TRUE;
	}

	// 把 "assets/create/textures/item/wrench.png" 拆成 (create, item/wrench)。
	// 不是 assets/<命名空间>/textures/…png 形式的路径返回 FALSE。
	BOOL SplitTexturePath(const char* pszPath, std::string& strNamespace, std::string& strRelative)
	{
		const std::string strPath(pszPath);
		const std::string strAssets = "assets/";
		const std::string strTextures = "textures/";

		if (strPath.compare(0, strAssets.size(), strAssets) != 0)
			return FALSE;

		const size_t nNamespaceEnd = strPath.find('/', strAssets.size());
		if (nNamespaceEnd == std::string::npos)
			return FALSE;

		const size_t nTextures = nNamespaceEnd + 1;
		if (strPath.compare(nTextures, strTextures.size(), strTextures) != 0)
			return FALSE;

		// textures/ 下面按用途分目录（item/block/entity/fluid/…），这里不限制
		const std::string strName = strPath.substr(nTextures + strTextures.size());
		if (strName.size() <= strPngExt.size() ||
			strName.compare(strName.size() - strPngExt.size(), strPngExt.size(), strPngExt) != 0)
			return FALSE;

		strNamespace = strPath.substr(strAssets.size(), nNamespaceEnd - strAssets.size());
		strRelative = strName.substr(0, strName.size() - strPngExt.size());
		return TRUE;
	}

	// 从贴图相对路径里认出流体："fluid/<名称>_still.png" 或 "fluid/<名称>_flow.png"。
	// 流体没有模型，只有静止和流动这两张贴图
	BOOL SplitFluidName(const std::string& strRelative, std::string& strName)
	{
		const std::string strFluid = "fluid/";
		if (strRelative.compare(0, strFluid.size(), strFluid) != 0)
			return FALSE;

		// 流体贴图直接放在 fluid/ 下，出现子目录说明不是
		const std::string strRest = strRelative.substr(strFluid.size());
		if (strRest.find('/') != std::string::npos)
			return FALSE;

		const char* arrSuffix[] = { "_still", "_flow" };
		for (size_t i = 0; i < _countof(arrSuffix); ++i)
		{
			const std::string strSuffix = arrSuffix[i];
			if (strRest.size() > strSuffix.size() &&
				strRest.compare(strRest.size() - strSuffix.size(), strSuffix.size(), strSuffix) == 0)
			{
				strName = strRest.substr(0, strRest.size() - strSuffix.size());
				return TRUE;
			}
		}

		return FALSE;
	}

	// 流体贴图的拆分结果
	struct FluidPath
	{
		std::string strNamespace;	// 命名空间
		std::string strName;		// 流体名（贴图名去掉 _still / _flow）
	};

	// 把 "assets/create/lang/en_us.json" 拆成 (create, en_us.json)
	BOOL SplitLangPath(const char* pszPath, std::string& strNamespace, std::string& strFile)
	{
		const std::string strPath(pszPath);
		const std::string strAssets = "assets/";
		const std::string strLang = "lang/";

		if (strPath.compare(0, strAssets.size(), strAssets) != 0)
			return FALSE;

		const size_t nNamespaceEnd = strPath.find('/', strAssets.size());
		if (nNamespaceEnd == std::string::npos)
			return FALSE;

		const size_t nLang = nNamespaceEnd + 1;
		if (strPath.compare(nLang, strLang.size(), strLang) != 0)
			return FALSE;

		// lang 目录下不再有子目录，出现子目录说明不是语言文件
		const std::string strName = strPath.substr(nLang + strLang.size());
		if (!IsJsonFile(strName) || strName.find('/') != std::string::npos)
			return FALSE;

		strNamespace = strPath.substr(strAssets.size(), nNamespaceEnd - strAssets.size());
		strFile = strName;
		return TRUE;
	}

	// 是不是流体标签文件：data/<命名空间>/tags/fluids/**.json
	//
	// 流体没有物品模型，原版的 water / lava 连 textures/fluid/ 都没有（贴图是
	// textures/block/water_still.png，和方块混在一起），只有标签里认得出它们。
	// 标签的成员（minecraft:water）写在文件内容里，这里先只认路径
	BOOL IsFluidTagPath(const char* pszPath)
	{
		const std::string strPath(pszPath);
		const std::string strData = "data/";
		const char* arrTagDir[] = { "tags/fluids/", "tags/fluid/" };

		if (strPath.compare(0, strData.size(), strData) != 0)
			return FALSE;

		const size_t nNamespaceEnd = strPath.find('/', strData.size());
		if (nNamespaceEnd == std::string::npos)
			return FALSE;

		const size_t nTags = nNamespaceEnd + 1;
		BOOL bTagDir = FALSE;
		for (size_t i = 0; i < _countof(arrTagDir); ++i)
		{
			const std::string strTagDir = arrTagDir[i];
			if (strPath.compare(nTags, strTagDir.size(), strTagDir) == 0)
			{
				bTagDir = TRUE;
				break;
			}
		}
		if (!bTagDir)
			return FALSE;

		// 标签目录下还能再分子目录，最后的文件名是 json 就行
		return IsJsonFile(strPath);
	}

	// 把流体标签里的一个值拆成 id："minecraft:water" -> (minecraft, water)。
	// 引别的标签的值（"#forge:milk"）和不成 id 的字符串返回 FALSE
	BOOL SplitFluidId(const std::string& strValue, std::string& strNamespace, std::string& strName)
	{
		if (strValue.empty() || strValue[0] == '#')
			return FALSE;

		const size_t nColon = strValue.find(':');
		if (nColon == std::string::npos || nColon + 1 >= strValue.size())
			return FALSE;

		// 资源名只由小写字母、数字和 _ - . / 组成，免得把别的字符串当成流体 id
		for (size_t i = 0; i < strValue.size(); ++i)
		{
			const char c = strValue[i];
			if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
				c == '_' || c == '-' || c == '.' || c == '/' || c == ':')
				continue;
			return FALSE;
		}

		strNamespace = strValue.substr(0, nColon);
		strName = strValue.substr(nColon + 1);
		return TRUE;
	}

	// 判断某个模型文件在不在这个 jar 里的键
	std::string MakePathKey(const std::string& strNamespace, const std::string& strKind, const std::string& strRelative)
	{
		return strNamespace + "/" + strKind + "/" + strRelative;
	}

	// 贴图集合里的键："命名空间/相对路径"（相对路径不含 .png）
	std::string MakeRefKey(const std::string& strNamespace, const std::string& strRelative)
	{
		return strNamespace + "/" + strRelative;
	}

	// 解压 zip 里第 nIndex 个成员，失败时返回空串
	std::string ExtractZipText(mz_zip_archive* pZip, int nIndex)
	{
		std::string strText;
		if (pZip == nullptr || nIndex < 0)
			return strText;

		size_t nSize = 0;
		void* pData = mz_zip_reader_extract_to_heap(pZip, (mz_uint)nIndex, &nSize, 0);
		if (pData != nullptr)
		{
			strText.assign(static_cast<const char*>(pData), nSize);
			mz_free(pData);
		}

		return strText;
	}

	// 按需解压并缓存 zip 里的文本文件
	struct TextCache
	{
		mz_zip_archive* pZip;

		std::map<std::string, std::string> mapText;	// 键 -> 内容（解压一次后缓存）

		TextCache() : pZip(nullptr) {}

	protected:
		// 取内容，没登记或解压失败时返回空串
		const std::string& GetTextAt(const std::string& strKey, int nIndex)
		{
			std::map<std::string, std::string>::iterator it = mapText.find(strKey);
			if (it != mapText.end())
				return it->second;

			return mapText.insert(std::make_pair(strKey, ExtractZipText(pZip, nIndex))).first->second;
		}
	};

	// 语言文件缓存
	//
	// 物品 id 里的 '/' 在语言文件里写成 '.'（id 为 sophisticatedbackpacks:sawmill/sawmill_upgrade
	// 的物品，语言键是 item.sophisticatedbackpacks.sawmill.sawmill_upgrade），所以反过来可以用
	// 语言键是否存在，确认某个名字到底是不是真正的物品 id；流体名（fluid.create.tea）也在这里查
	struct LangCache : TextCache
	{
		std::map<std::string, int>  mapIndex;	// 命名空间 -> 语言文件在 zip 中的序号
		std::map<std::string, BOOL> mapEnUs;	// 命名空间 -> 当前用的是不是 en_us.json

		// 记录一个语言文件（同一个命名空间优先用 en_us.json）
		void Add(const std::string& strNamespace, int nIndex, BOOL bEnUs)
		{
			std::map<std::string, BOOL>::iterator it = mapEnUs.find(strNamespace);
			if (it != mapEnUs.end() && (it->second || !bEnUs))
				return;

			mapIndex[strNamespace] = nIndex;
			mapEnUs[strNamespace] = bEnUs;
		}

		// 这个命名空间有没有语言文件可查
		BOOL Has(const std::string& strNamespace) const
		{
			return mapIndex.find(strNamespace) != mapIndex.end();
		}

		// 语言文件里有没有 "item.<命名空间>.<名称>" 这样的键
		BOOL HasKey(const std::string& strNamespace, const char* pszType, const std::string& strName)
		{
			const std::string& strText = GetText(strNamespace);
			if (strText.empty())
				return FALSE;

			// id 里的 '/' 在语言键里是 '.'
			std::string strKey = std::string("\"") + pszType + "." + strNamespace + ".";
			for (size_t i = 0; i < strName.size(); ++i)
				strKey += (strName[i] == '/' ? '.' : strName[i]);
			strKey += "\"";

			return strText.find(strKey) != std::string::npos;
		}

		// 语言文件里有没有这个名字的物品键或方块键
		// （方块做成的物品两种键都可能出现，只看一种会漏）
		BOOL HasName(const std::string& strNamespace, const std::string& strName)
		{
			return HasKey(strNamespace, "item", strName) || HasKey(strNamespace, "block", strName);
		}

	private:
		// 取语言文件内容（解压一次后缓存），取不到时返回空串
		const std::string& GetText(const std::string& strNamespace)
		{
			std::map<std::string, int>::const_iterator itIndex = mapIndex.find(strNamespace);
			return GetTextAt(strNamespace, itIndex == mapIndex.end() ? -1 : itIndex->second);
		}
	};

	// 模型文件缓存
	//
	// 键统一用 "命名空间/<kind>/相对路径"，跟 MakePathKey 一致。模型的 overrides、
	// parent 检查会反复读同一个模型，所以解压过的内容缓存下来
	struct ModelCache : TextCache
	{
		std::map<std::string, int> mapIndex;	// 模型文件的键 -> zip 中的序号

		void Add(const std::string& strKey, int nIndex)
		{
			mapIndex[strKey] = nIndex;
		}

		// 这个模型文件在不在包里
		BOOL Has(const std::string& strKey) const
		{
			return mapIndex.find(strKey) != mapIndex.end();
		}

		// 取模型内容，不在包里或解压失败时返回空串
		const std::string& GetText(const std::string& strKey)
		{
			std::map<std::string, int>::const_iterator itIndex = mapIndex.find(strKey);
			return GetTextAt(strKey, itIndex == mapIndex.end() ? -1 : itIndex->second);
		}
	};

	// 扫一遍模型 json：收集里面所有字符串，顺便取出 "parent" 和 overrides 里 "model" 的值
	//
	// 模型文件里没有带转义的字符串，成对的引号就是完整的一个值，用不着拉个 json 库进来。
	// 流体标签这种没有 parent / model 键的文件也能用，那两个输出拿到的就是空的
	void ScanModelJson(const std::string& strText, std::vector<std::string>& arrStrings,
		std::string& strParent, std::vector<std::string>& arrOverrides)
	{
		size_t i = 0;
		while (i < strText.size())
		{
			if (strText[i] != '"')
			{
				++i;
				continue;
			}

			const size_t nEnd = strText.find('"', i + 1);
			if (nEnd == std::string::npos)
				return;

			const std::string strValue = strText.substr(i + 1, nEnd - i - 1);
			arrStrings.push_back(strValue);

			// "parent" / "model": 后面跟的字符串是模型引用，单独取出来
			if (strValue == "parent" || strValue == "model")
			{
				const size_t nRef = strText.find('"', nEnd + 1);
				if (nRef == std::string::npos)
					return;

				const size_t nRefEnd = strText.find('"', nRef + 1);
				if (nRefEnd == std::string::npos)
					return;

				const std::string strRef = strText.substr(nRef + 1, nRefEnd - nRef - 1);
				if (strValue == "parent")
					strParent = strRef;
				else
					arrOverrides.push_back(strRef);

				i = nRefEnd + 1;
				continue;
			}

			i = nEnd + 1;
		}
	}

	// 贴图引用是不是指向一张确实存在的贴图
	//
	// 引用没写命名空间时游戏按 minecraft 算，这里顺手把模型自己的命名空间也认一下，
	// 免得漏掉把贴图放在自己命名空间下的写法
	std::string FindTexture(const std::set<std::string>& setTextures, const std::string& strRef,
		const std::string& strDefaultNamespace)
	{
		std::string strNamespace = "minecraft";
		std::string strPath = strRef;

		const size_t nColon = strRef.find(':');
		if (nColon != std::string::npos)
		{
			strNamespace = strRef.substr(0, nColon);
			strPath = strRef.substr(nColon + 1);
		}

		const std::string strKey = MakeRefKey(strNamespace, strPath);
		if (setTextures.count(strKey) > 0)
			return strKey;

		const std::string strDefaultKey = MakeRefKey(strDefaultNamespace, strPath);
		if (nColon == std::string::npos && setTextures.count(strDefaultKey) > 0)
			return strDefaultKey;
		return std::string();
	}

	// 这个模型有没有引用到真实存在的贴图（自己没写就顺着 parent 往上找）
	//
	// 能拿在手上的东西都有贴图：air.json 是空模型，item/generated.json 这种模板只有 display，
	// 而 stone.json 只是转手指向 block/stone。正好用这一点把不是物品的模型筛掉
	std::string FindModelTexture(const std::string& strKey, const std::string& strNamespace,
		const std::set<std::string>& setTextures, ModelCache& models, std::set<std::string>& setVisited)
	{
		if (!setVisited.insert(strKey).second)	// 模型互相引用时别绕圈
			return std::string();

		const std::string& strText = models.GetText(strKey);
		if (strText.empty())
			return std::string();

		std::vector<std::string> arrStrings;
		std::vector<std::string> arrOverrides;
		std::string strParent;
		ScanModelJson(strText, arrStrings, strParent, arrOverrides);

		// parent 和 overrides 里的是模型引用（item/generated 这种），不是贴图，别拿来当证据
		std::set<std::string> setModelRefs(arrOverrides.begin(), arrOverrides.end());
		setModelRefs.insert(strParent);

		for (size_t i = 0; i < arrStrings.size(); ++i)
		{
			// 贴图路径都带 '/'，用不着看 gui_light 这类字符串
			if (arrStrings[i].find('/') == std::string::npos || setModelRefs.count(arrStrings[i]) > 0)
				continue;

			const std::string strTexture = FindTexture(setTextures, arrStrings[i], strNamespace);
			if (!strTexture.empty())
				return strTexture;
		}

		// parent 一般不写扩展名，最多往上找几层（setVisited 里每层加一个键，顺便当层数用）
		ModelPath parent;
		if (setVisited.size() >= 8 || !SplitModelRef(strParent, parent))
			return std::string();

		return FindModelTexture(MakePathKey(parent.strNamespace, parent.strKind, parent.strRelative),
			parent.strNamespace, setTextures, models, setVisited);
	}
}


int CKubeJSRecipeHelperDlg::ParseJarModels(LPCTSTR lpszJarPath, CString*& pNames)
{
	pNames = nullptr;

	mz_zip_archive zip;
	memset(&zip, 0, sizeof(zip));

	// jar 本质是 zip，miniz 的初始化接口使用 UTF-8 路径
	if (!mz_zip_reader_init_file(&zip, CT2A(lpszJarPath, CP_UTF8), 0))
		return -1;

	// ---------- 第一步：收集物品模型、贴图和语言文件 ----------
	//
	// 候选只来自 assets/<命名空间>/models/item/：能拿在手上的东西（方块物品也算）都有物品模型，
	// 而水、岩浆、各种砖墙这些拿不到的方块只有 models/block/ 里的模型
	std::vector<ModelPath> arrModels;
	ModelCache models;
	models.pZip = &zip;
	std::set<std::string> setTextures;		// 包里有哪些贴图
	std::map<std::string, std::string> mapTexturePaths;	// 贴图键 -> JAR 内的 PNG 路径
	std::vector<FluidPath> arrFluids;		// 候选流体（来自 textures/fluid 贴图）
	std::vector<int> arrFluidTags;			// 流体标签文件在 zip 里的序号
	LangCache lang;
	lang.pZip = &zip;

	const mz_uint nFiles = mz_zip_reader_get_num_files(&zip);
	for (mz_uint i = 0; i < nFiles; ++i)
	{
		mz_zip_archive_file_stat st;
		if (!mz_zip_reader_file_stat(&zip, i, &st) || st.m_is_directory)
			continue;

		ModelPath model;
		if (SplitModelPath(st.m_filename, model))
		{
			// block 模型也要记下来：物品模型会顺着 parent 找到它们
			models.Add(MakePathKey(model.strNamespace, model.strKind, model.strRelative), (int)i);
			if (model.strKind == "item")
				arrModels.push_back(model);
			continue;
		}

		std::string strTextureNamespace;
		std::string strTextureRelative;
		if (SplitTexturePath(st.m_filename, strTextureNamespace, strTextureRelative))
		{
			const std::string strTextureKey = MakeRefKey(strTextureNamespace, strTextureRelative);
			setTextures.insert(strTextureKey);
			mapTexturePaths[strTextureKey] = st.m_filename;

			// 流体：textures/fluid/<名称>_still.png / _flow.png
			std::string strFluidName;
			if (SplitFluidName(strTextureRelative, strFluidName))
			{
				FluidPath fluid;
				fluid.strNamespace = strTextureNamespace;
				fluid.strName = strFluidName;
				arrFluids.push_back(fluid);
			}
			continue;
		}

		// 流体标签的成员（minecraft:water 这种）写在文件内容里，先记下位置
		if (IsFluidTagPath(st.m_filename))
		{
			arrFluidTags.push_back((int)i);
			continue;
		}

		// 顺带记下语言文件，后面确认物品名和流体名时要用到
		std::string strLangNamespace;
		std::string strLangFile;
		if (SplitLangPath(st.m_filename, strLangNamespace, strLangFile))
			lang.Add(strLangNamespace, (int)i, strLangFile == "en_us.json");
	}

	// ---------- 第二步：先找出变体模型 ----------
	//
	// 拉弓的每一帧（bow_pulling_0）、指南针的每一圈（compass_16）、各种护甲纹饰……
	// 都是被别的模型的 overrides 引用的，玩家拿不到单独的这一件，不算物品
	std::set<std::string> setVariants;
	for (size_t i = 0; i < arrModels.size(); ++i)
	{
		const ModelPath& model = arrModels[i];
		const std::string strKey = MakePathKey(model.strNamespace, model.strKind, model.strRelative);

		std::vector<std::string> arrStrings;	// 这里只看模型引用，字符串列表用不上
		std::vector<std::string> arrOverrides;
		std::string strParent;
		ScanModelJson(models.GetText(strKey), arrStrings, strParent, arrOverrides);

		for (size_t k = 0; k < arrOverrides.size(); ++k)
		{
			ModelPath ref;
			if (!SplitModelRef(arrOverrides[k], ref))
				continue;

			const std::string strRefKey = MakePathKey(ref.strNamespace, ref.strKind, ref.strRelative);
			if (strRefKey != strKey)	// 引用自己（如 clock.json 里的 "model": "item/clock"）不算变体
				setVariants.insert(strRefKey);
		}
	}

	// ---------- 第三步：逐个物品模型推出物品 id ----------
	std::vector<std::string> arrNames;
	std::set<std::string> setNames;
	std::map<std::string, std::string> mapItemTextures;	// 物品 ID -> 贴图键
	std::map<std::string, unsigned> entryTypes;

	// 收录一个名字，按 “命名空间:名字” 去重
	auto AddName = [&arrNames, &setNames, &entryTypes](const std::string& strNamespace, const std::string& strName, unsigned kind = RecipeItem)
	{
		if (strNamespace.empty() || strName.empty())
			return;

		const std::string strFull = strNamespace + ":" + strName;
		entryTypes[strFull] |= kind;
		if (setNames.insert(strFull).second)
			arrNames.push_back(strFull);
	};

	auto AddModelName = [&AddName, &mapItemTextures, &setTextures, &models](
		const std::string& strNamespace, const std::string& strName, const std::string& strModelKey)
	{
		AddName(strNamespace, strName);
		const std::string strFull = strNamespace + ":" + strName;
		if (mapItemTextures.count(strFull) != 0)
			return;

		std::set<std::string> setVisited;
		const std::string strTexture = FindModelTexture(strModelKey, strNamespace,
			setTextures, models, setVisited);
		if (!strTexture.empty())
			mapItemTextures[strFull] = strTexture;
	};

	for (size_t i = 0; i < arrModels.size(); ++i)
	{
		const ModelPath& model = arrModels[i];
		const std::string strKey = MakePathKey(model.strNamespace, model.strKind, model.strRelative);

		// 直接放在 models/item 下的模型：文件名就是物品名
		const size_t nSlash = model.strRelative.find('/');
		if (nSlash == std::string::npos)
		{
			// 变体模型（拉弓的每一帧、指南针的每一圈……）不是能单独拿到的物品
			if (setVariants.count(strKey) > 0)
				continue;

			// 模型得真的引用到一张存在的贴图：air 这种空模型、item/generated 这种
			// 光有 display 的模板都没引用贴图，正好筛掉
			std::set<std::string> setVisited;
			const std::string strTexture = FindModelTexture(strKey, model.strNamespace,
				setTextures, models, setVisited);
			if (strTexture.empty())
				continue;

			const std::string strName = WithoutJsonExt(model.strRelative);
			AddName(model.strNamespace, strName);
			mapItemTextures[model.strNamespace + ":" + strName] = strTexture;
			continue;
		}

		const std::string strFolder = model.strRelative.substr(0, nSlash);	// 最外层的文件夹名

		// 物品：同级还有同名的扁平模型时，这个文件夹只是该物品的零件集合
		// （如 models/item/wrench/），物品名已经由那个扁平模型收录了
		if (models.Has(MakePathKey(model.strNamespace, "item", strFolder + ".json")))
			continue;

		// 文件夹里有 item.json 时（Create 等模组的自定义模型写法），文件夹名就是物品名
		if (models.Has(MakePathKey(model.strNamespace, "item", strFolder + "/item.json")))
		{
			AddModelName(model.strNamespace, strFolder,
				MakePathKey(model.strNamespace, "item", strFolder + "/item.json"));
			continue;
		}

		// 剩下的情况光看文件名分不出文件夹名和相对路径哪个才是物品 id，用语言文件确认：
		//   create 的 models/item/package/ 里那一堆零件  -> 语言键 item.create.package 存在，
		//                                                   而 item.create.package.xxx 都不存在，用文件夹名
		//   models/item/sawmill/sawmill_upgrade.json    -> 语言键
		//                                                   item.sophisticatedbackpacks.sawmill 不存在，
		//                                                   而 item.sophisticatedbackpacks.sawmill.sawmill_upgrade
		//                                                   存在，用相对路径
		// 方块做成的物品，语言键是 block. 开头，两种都查
		// 按约定路径收录时，文件夹本身也是物品 id 的一部分
		const std::string strRelativeName = WithoutJsonExt(model.strRelative);
		if (lang.Has(model.strNamespace))
		{
			if (lang.HasName(model.strNamespace, strFolder))
				AddModelName(model.strNamespace, strFolder, strKey);
			if (lang.HasName(model.strNamespace, strRelativeName))
				AddModelName(model.strNamespace, strRelativeName, strKey);
		}
		else
		{
			// 没有语言文件可查，就按物品模型的约定路径收录：
			// models/item/<id>.json 里的 <id> 就是物品 id
			AddModelName(model.strNamespace, strRelativeName, strKey);
		}
	}

	// ---------- 第四步：流体 ----------
	//
	// 流体没有物品模型，两处线索：
	//   1. 流体标签 data/<命名空间>/tags/fluids/**.json 的成员，命名空间写在值里
	//      （原版的 minecraft:water / minecraft:lava 就是这么来的）
	//   2. textures/fluid/<名称>_still.png / _flow.png 贴图。语言文件里有
	//      fluid.<命名空间>.<名称> 的就是模组自己注册的流体（create:tea）；没有的，多半是
	//      Forge 之类把流体挂在 minecraft 命名空间下、贴图由模组补的
	//      （Create 补的 milk 贴图对应 minecraft:milk）
	for (size_t i = 0; i < arrFluidTags.size(); ++i)
	{
		std::vector<std::string> arrStrings;
		std::vector<std::string> arrOverrides;
		std::string strParent;
		ScanModelJson(ExtractZipText(&zip, arrFluidTags[i]), arrStrings, strParent, arrOverrides);

		for (size_t k = 0; k < arrStrings.size(); ++k)
		{
			std::string strNamespace;
			std::string strName;
			if (SplitFluidId(arrStrings[k], strNamespace, strName))
			{
				AddName(strNamespace, strName, RecipeFluid);
				const std::string strBlockTexture = MakeRefKey(strNamespace,
					"block/" + strName + "_still");
				if (setTextures.count(strBlockTexture) > 0)
					mapItemTextures[strNamespace + ":" + strName] = strBlockTexture;
			}
		}
	}

	for (size_t i = 0; i < arrFluids.size(); ++i)
	{
		const std::string& strNamespace = arrFluids[i].strNamespace;
		const std::string& strName = arrFluids[i].strName;

		const std::string strIdNamespace =
			(strNamespace == "minecraft" || lang.HasKey(strNamespace, "fluid", strName))
			? strNamespace : "minecraft";
		AddName(strIdNamespace, strName, RecipeFluid);

		const std::string strStill = MakeRefKey(strNamespace, "fluid/" + strName + "_still");
		const std::string strFlow = MakeRefKey(strNamespace, "fluid/" + strName + "_flow");
		const std::string strId = strIdNamespace + ":" + strName;
		if (setTextures.count(strStill) > 0)
			mapItemTextures[strId] = strStill;
		else if (mapItemTextures.count(strId) == 0 && setTextures.count(strFlow) > 0)
			mapItemTextures[strId] = strFlow;
	}

	mz_zip_reader_end(&zip);

	const int nCount = (int)arrNames.size();
	if (nCount == 0)
		return 0;

	// 使用 new[] 分配 CString 数组，与调用方的 delete[] 配对
	// DEBUG_NEW 宏会破坏 nothrow new，此处临时还原
#pragma push_macro("new")
#undef new
	CString* pArr = new (std::nothrow) CString[nCount];
#pragma pop_macro("new")
	if (pArr == nullptr)
		return -1;

	// zip 内文件名均为 UTF-8，转成 CString 时按 UTF-8 解释
	for (int i = 0; i < nCount; ++i)
		pArr[i] = CString(CA2T(arrNames[i].c_str(), CP_UTF8));
	for (const auto& entry : entryTypes)
	{
		const CString id(CA2T(entry.first.c_str(), CP_UTF8));
		m_entryTypes[std::wstring(id.GetString())] |= entry.second;
	}

	for (const auto& itemTexture : mapItemTextures)
	{
		const auto itPath = mapTexturePaths.find(itemTexture.second);
		if (itPath != mapTexturePaths.end())
			m_itemTextures.Register(CString(CA2T(itemTexture.first.c_str(), CP_UTF8)),
				lpszJarPath, itPath->second);
	}

	pNames = pArr;
	return nCount;
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnclear()
{
	// TODO: 在此添加控件通知处理程序代码
	SetDlgItemText(IDC_EDITOUTPUT, _T(""));
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtndwnload()
{
	// TODO: 在此添加控件通知处理程序代码
	GetDlgItemText(IDC_EDITOUTPUT, m_strOutput);
	std::ofstream out("output.js", std::ios::app | std::ios::binary);
	out << CT2A(m_strOutput, CP_UTF8) << std::endl;
	MessageBox(_T("文件已输出至软件同目录下"));
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnother()
{
	CDlgOther dlg(this);
	dlg.SetItemSource(&m_arrNames);
	dlg.SetCatalog(&m_entryTypes);
	dlg.SetTextureStore(&m_itemTextures);
	if (dlg.DoModal() == IDOK)
		ShowRecipeScript(dlg.GetRecipeScript());
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnremove()
{
	CDlgRecipeChanges dlg(true,this);
	dlg.SetItemSource(&m_arrNames);
	dlg.SetCatalog(&m_entryTypes);
	dlg.SetTextureStore(&m_itemTextures);
	if(dlg.DoModal()==IDOK) ShowRecipeScript(dlg.GetRecipeScript());
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnmodify()
{
	CDlgRecipeChanges dlg(false,this);
	dlg.SetItemSource(&m_arrNames);
	dlg.SetCatalog(&m_entryTypes);
	dlg.SetTextureStore(&m_itemTextures);
	if(dlg.DoModal()==IDOK) ShowRecipeScript(dlg.GetRecipeScript());
}
