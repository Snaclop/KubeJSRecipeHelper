
// KubeJSRecipeHelperDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "KubeJSRecipeHelper.h"
#include "KubeJSRecipeHelperDlg.h"
#include "afxdialogex.h"
#include "miniz.h"
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
	ON_BN_CLICKED(IDC_BTNSHAPED, &CKubeJSRecipeHelperDlg::OnBnClickedBtnshaped)
	ON_BN_CLICKED(IDC_BTNSHAPELESS, &CKubeJSRecipeHelperDlg::OnBnClickedBtnshapeless)
	ON_BN_CLICKED(IDC_BTNSMITH, &CKubeJSRecipeHelperDlg::OnBnClickedBtnsmith)
	ON_BN_CLICKED(IDC_BTNCOOK, &CKubeJSRecipeHelperDlg::OnBnClickedBtncook)
	ON_BN_CLICKED(IDC_BTNSTONECUT, &CKubeJSRecipeHelperDlg::OnBnClickedBtnstonecut)
	ON_BN_CLICKED(IDC_BTNCREATE, &CKubeJSRecipeHelperDlg::OnBnClickedBtncreate)
	ON_BN_CLICKED(IDC_BTNIMPORT, &CKubeJSRecipeHelperDlg::OnBnClickedBtnimport)
	ON_BN_CLICKED(IDC_BTNCLEAR, &CKubeJSRecipeHelperDlg::OnBnClickedBtnclear)
	ON_BN_CLICKED(IDC_BTNDWNLOAD, &CKubeJSRecipeHelperDlg::OnBnClickedBtndwnload)
END_MESSAGE_MAP()


// CKubeJSRecipeHelperDlg 消息处理程序

BOOL CKubeJSRecipeHelperDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

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

void CKubeJSRecipeHelperDlg::OnBnClickedBtnshaped()
{
	// 有序合成：把已导入的物品列表交给九宫格，点击方格时从中选择物品
	CDlgShaped dlg;
	dlg.SetItemSource(&m_arrNames);
	if (dlg.DoModal() != IDOK)
		return;

	ShowRecipeScript(dlg.GetRecipeScript());
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnshapeless()
{
	// 无序合成：界面跟有序合成一样，只是生成 event.shapeless
	CDlgShapeless dlg;
	dlg.SetItemSource(&m_arrNames);
	if (dlg.DoModal() != IDOK)
		return;

	ShowRecipeScript(dlg.GetRecipeScript());
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnsmith()
{
	// TODO: 在此添加控件通知处理程序代码，锻造台
	CDlgSmith dlg;
	dlg.DoModal();
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtncook()
{
	// TODO: 在此添加控件通知处理程序代码，熔炉
	CDlgCook dlg;
	dlg.DoModal();
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnstonecut()
{
	// TODO: 在此添加控件通知处理程序代码，切石机
	CDlgStonecut dlg;
	dlg.DoModal();
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtncreate()
{
	// TODO: 在此添加控件通知处理程序代码，机械动力
	CDlgCreate dlg;
	dlg.DoModal();
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnimport()
{
	// TODO: 在此添加控件通知处理程序代码
	// 导入jar文件并解析
	CFileDialog dlgFile(TRUE, _T("jar"), nullptr,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		_T("JAR 文件 (*.jar)|*.jar|所有文件 (*.*)|*.*||"), this);
	if (dlgFile.DoModal() != IDOK)
		return;

	CString strJarPath = dlgFile.GetPathName();

	// 解析选中的 jar 文件，收集 models 目录下 item / block 里的物品、方块 id
	CString* pNames = nullptr;
	int nCount = ParseJarModels(strJarPath, pNames);
	if (nCount < 0)
	{
		AfxMessageBox(_T("JAR 文件解析失败！"), MB_ICONERROR);
		return;
	}

	// 追加到累积列表（保留之前导入的数据）
	for (int i = 0; i < nCount; ++i)
		m_arrNames.Add(pNames[i]);

	delete[] pNames;

	// 刷新显示全部累积结果
	RefreshNamesOutput();
}

void CKubeJSRecipeHelperDlg::RefreshNamesOutput()
{
	CString strOutput;
	strOutput.Format(_T("共找到 %d 个 item：\r\n"), (int)m_arrNames.GetSize());
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

	// 判断某个模型文件在不在这个 jar 里的键
	std::string MakePathKey(const std::string& strNamespace, const std::string& strKind, const std::string& strRelative)
	{
		return strNamespace + "/" + strKind + "/" + strRelative;
	}

	// 语言文件缓存
	//
	// 物品 id 里的 '/' 在语言文件里写成 '.'（id 为 sophisticatedbackpacks:sawmill/sawmill_upgrade
	// 的物品，语言键是 item.sophisticatedbackpacks.sawmill.sawmill_upgrade），所以反过来可以用
	// 语言键是否存在，确认某个名字到底是不是真正的物品 id。
	struct LangCache
	{
		mz_zip_archive* pZip;

		std::map<std::string, int>  mapIndex;	// 命名空间 -> 语言文件在 zip 中的序号
		std::map<std::string, BOOL> mapEnUs;	// 命名空间 -> 当前用的是不是 en_us.json
		std::map<std::string, std::string> mapText;	// 命名空间 -> 语言文件内容

		LangCache() : pZip(nullptr) {}

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
			std::map<std::string, std::string>::iterator it = mapText.find(strNamespace);
			if (it != mapText.end())
				return it->second;

			std::string strText;
			std::map<std::string, int>::const_iterator itIndex = mapIndex.find(strNamespace);
			if (pZip != nullptr && itIndex != mapIndex.end())
			{
				size_t nSize = 0;
				void* pData = mz_zip_reader_extract_to_heap(pZip, (mz_uint)itIndex->second, &nSize, 0);
				if (pData != nullptr)
				{
					strText.assign(static_cast<const char*>(pData), nSize);
					mz_free(pData);
				}
			}

			return mapText.insert(std::make_pair(strNamespace, strText)).first->second;
		}
	};
}


int CKubeJSRecipeHelperDlg::ParseJarModels(LPCTSTR lpszJarPath, CString*& pNames)
{
	pNames = nullptr;

	mz_zip_archive zip;
	memset(&zip, 0, sizeof(zip));

	// jar 本质是 zip，miniz 的初始化接口使用 UTF-8 路径
	if (!mz_zip_reader_init_file(&zip, CT2A(lpszJarPath, CP_UTF8), 0))
		return -1;

	// ---------- 第一步：收集 assets/<命名空间>/models/(item|block)/ 下的所有模型文件 ----------
	std::vector<ModelPath> arrModels;
	std::set<std::string> setPaths;		// 用来判断某个模型文件是否存在
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
			arrModels.push_back(model);
			setPaths.insert(MakePathKey(model.strNamespace, model.strKind, model.strRelative));
			continue;
		}

		// 顺带记下语言文件，后面确认物品名时要用到
		std::string strLangNamespace;
		std::string strLangFile;
		if (SplitLangPath(st.m_filename, strLangNamespace, strLangFile))
			lang.Add(strLangNamespace, (int)i, strLangFile == "en_us.json");
	}

	// ---------- 第二步：逐个模型推出物品 / 方块 id ----------
	std::vector<std::string> arrNames;
	std::set<std::string> setNames;

	// 收录一个名字，按 “命名空间:名字” 去重
	auto AddName = [&arrNames, &setNames](const std::string& strNamespace, const std::string& strName)
	{
		if (strNamespace.empty() || strName.empty())
			return;

		const std::string strFull = strNamespace + ":" + strName;
		if (setNames.insert(strFull).second)
			arrNames.push_back(strFull);
	};

	for (size_t i = 0; i < arrModels.size(); ++i)
	{
		const ModelPath& model = arrModels[i];

		// 直接放在 models/item 或 models/block 下的模型：文件名就是物品 / 方块名。
		// 不过这里混着大量零件模型（方块的门、墙、台阶零件，箭的拉弓动画帧，护甲纹饰变体……），
		// 有语言文件时就用语言键把它们筛掉
		const size_t nSlash = model.strRelative.find('/');
		if (nSlash == std::string::npos)
		{
			const std::string strName = WithoutJsonExt(model.strRelative);
			if (!lang.Has(model.strNamespace) || lang.HasName(model.strNamespace, strName))
				AddName(model.strNamespace, strName);
			continue;
		}

		const std::string strFolder = model.strRelative.substr(0, nSlash);	// 最外层的文件夹名

		if (model.strKind == "block")
		{
			// 方块的模型由 blockstates 指定，models/block/<文件夹>/ 里放的是一个方块的多套模型
			// （Create 等模组都是这种写法），所以文件夹名就是方块名
			AddName(model.strNamespace, strFolder);
			continue;
		}

		// 物品：同级还有同名的扁平模型时，这个文件夹只是该物品的零件集合
		// （如 models/item/wrench/），物品名已经由那个扁平模型收录了
		if (setPaths.count(MakePathKey(model.strNamespace, "item", strFolder + ".json")) > 0)
			continue;

		// 文件夹里有 item.json 时（Create 等模组的自定义模型写法），文件夹名就是物品名
		if (setPaths.count(MakePathKey(model.strNamespace, "item", strFolder + "/item.json")) > 0)
		{
			AddName(model.strNamespace, strFolder);
			continue;
		}

		// 剩下的情况光看文件名分不出文件夹名和相对路径哪个才是物品 id，用语言文件确认：
		//   create 的 models/item/package/ 里那一堆零件  -> 语言键 item.create.package 存在，
		//                                                   而 item.create.package.xxx 都不存在，用文件夹名
		//   models/item/sawmill/sawmill_upgrade.json    -> 语言键
		//                                                   item.sophisticatedbackpacks.sawmill 不存在，
		//                                                   而 item.sophisticatedbackpacks.sawmill.sawmill_upgrade
		//                                                   存在，用相对路径
		// 按约定路径收录时，文件夹本身也是物品 id 的一部分
		const std::string strRelativeName = WithoutJsonExt(model.strRelative);
		if (lang.Has(model.strNamespace))
		{
			if (lang.HasKey(model.strNamespace, "item", strFolder))
				AddName(model.strNamespace, strFolder);
			if (lang.HasKey(model.strNamespace, "item", strRelativeName))
				AddName(model.strNamespace, strRelativeName);
		}
		else
		{
			// 没有语言文件可查，就按物品模型的约定路径收录：
			// models/item/<id>.json 里的 <id> 就是物品 id
			AddName(model.strNamespace, strRelativeName);
		}
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
