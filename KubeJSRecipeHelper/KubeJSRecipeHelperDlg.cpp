
// KubeJSRecipeHelperDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "KubeJSRecipeHelper.h"
#include "KubeJSRecipeHelperDlg.h"
#include "afxdialogex.h"
#include "miniz.h"

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


void CKubeJSRecipeHelperDlg::OnBnClickedBtnshaped()
{
	// TODO: 在此添加控件通知处理程序代码
	CDlgShaped dlg;
	dlg.DoModal();
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnshapeless()
{
	// TODO: 在此添加控件通知处理程序代码
	CDlgShapeless dlg;
	dlg.DoModal();
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnsmith()
{
	// TODO: 在此添加控件通知处理程序代码
	CDlgSmith dlg;
	dlg.DoModal();
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtncook()
{
	// TODO: 在此添加控件通知处理程序代码
	CDlgCook dlg;
	dlg.DoModal();
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnstonecut()
{
	// TODO: 在此添加控件通知处理程序代码
	CDlgStonecut dlg;
	dlg.DoModal();
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtncreate()
{
	// TODO: 在此添加控件通知处理程序代码
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

	// 解析选中的 jar 文件，获取 blockstates 目录下的所有文件名
	CString* pNames = nullptr;
	int nCount = ParseJarBlockstates(strJarPath, pNames);
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
	strOutput.Format(_T("共找到 %d 个 blockstate 文件：\r\n"), (int)m_arrNames.GetSize());
	for (INT_PTR i = 0; i < m_arrNames.GetSize(); ++i)
	{
		strOutput += m_arrNames[i];
		strOutput += _T("\r\n");
	}
	SetDlgItemText(IDC_EDITOUTPUT, strOutput);
}

int CKubeJSRecipeHelperDlg::ParseJarBlockstates(LPCTSTR lpszJarPath, CString*& pNames)
{
	pNames = nullptr;

	mz_zip_archive zip;
	memset(&zip, 0, sizeof(zip));

	// jar 本质是 zip，miniz 的初始化接口使用 UTF-8 路径
	if (!mz_zip_reader_init_file(&zip, CT2A(lpszJarPath, CP_UTF8), 0))
		return -1;

	const char szPrefix[] = "assets/";
	const size_t nPrefixLen = sizeof(szPrefix) - 1;
	const char szDirName[] = "blockstates/";
	const size_t nDirNameLen = sizeof(szDirName) - 1;

	// assets 下同一级有多个目录，但只有一个包含 blockstates，找到后记录前缀
	char szBlockstatesPrefix[260] = { 0 };
	char szNamespace[260] = { 0 };  // blockstates 上级目录名（命名空间）
	size_t nNamespaceLen = 0;
	int nCapacity = 16;      // 动态数组容量
	int nCount = 0;          // 已找到的名称个数
	// 使用 new[] 分配 CString 数组，与调用方的 delete[] 配对
	// DEBUG_NEW 宏会破坏 nothrow new，此处临时还原
#pragma push_macro("new")
#undef new
	CString* pArr = new (std::nothrow) CString[nCapacity];
#pragma pop_macro("new")
	if (pArr == nullptr)
	{
		mz_zip_reader_end(&zip);
		return -1;
	}

	mz_uint nFiles = mz_zip_reader_get_num_files(&zip);
	for (mz_uint i = 0; i < nFiles; ++i)
	{
		mz_zip_archive_file_stat st;
		if (!mz_zip_reader_file_stat(&zip, i, &st) || st.m_is_directory)
			continue;

		const char* pszName = st.m_filename;
		size_t nLen = strlen(pszName);

		if (szBlockstatesPrefix[0] == '\0')
		{
			// 查找 assets/<目录>/blockstates/ 中的文件，记录该唯一前缀
			if (nLen > nPrefixLen + 1 + nDirNameLen && strncmp(pszName, szPrefix, nPrefixLen) == 0)
			{
				const char* pszMid = pszName + nPrefixLen;
				const char* pszSlash = strchr(pszMid, '/');
				if (pszSlash != nullptr && strncmp(pszSlash + 1, szDirName, nDirNameLen) == 0)
				{
					size_t nPfxLen = (pszSlash + 1 - pszName) + nDirNameLen;
					memcpy(szBlockstatesPrefix, pszName, nPfxLen);
					szBlockstatesPrefix[nPfxLen] = '\0';
					// 记录命名空间（assets/ 与 /blockstates/ 之间的目录名）
					nNamespaceLen = pszSlash - pszMid;
					memcpy(szNamespace, pszMid, nNamespaceLen);
					szNamespace[nNamespaceLen] = '\0';
				}
				else
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}
		else
		{
			// 前缀已确定，只保留该目录下的文件
			size_t nPfxLen = strlen(szBlockstatesPrefix);
			if (nLen <= nPfxLen + 1 || strncmp(pszName, szBlockstatesPrefix, nPfxLen) != 0)
				continue;
		}

		// 只取 blockstates 目录下的文件本身（不含子目录内容）
		const char* pszFile = pszName + strlen(szBlockstatesPrefix);
		if (strchr(pszFile, '/') != nullptr)
			continue;

		// 容量不足时扩容一倍
		if (nCount == nCapacity)
		{
			nCapacity *= 2;
#pragma push_macro("new")
#undef new
			CString* pNew = new (std::nothrow) CString[nCapacity];
#pragma pop_macro("new")
			if (pNew == nullptr)
			{
				delete[] pArr;
				mz_zip_reader_end(&zip);
				return -1;
			}
			// 逐个拷贝已有元素，再释放旧数组
			for (int k = 0; k < nCount; ++k)
				pNew[k] = pArr[k];
			delete[] pArr;
			pArr = pNew;
		}

		// 去掉 .json 扩展名
		CStringA strFile(pszFile);
		int nDot = strFile.ReverseFind('.');
		if (nDot > 0 && strFile.Mid(nDot) == ".json")
			strFile = strFile.Left(nDot);

		// 拼成 “命名空间:文件名” 的形式（zip 内文件名均为 UTF-8）
		CStringA strFull;
		strFull.Format("%s:%s", szNamespace, strFile.GetString());
		pArr[nCount] = CString(CA2T(strFull, CP_UTF8));
		++nCount;
	}

	mz_zip_reader_end(&zip);

	if (nCount == 0)
	{
		delete[] pArr;
		return 0;
	}

	pNames = pArr;
	return nCount;
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnclear()
{
	// TODO: 在此添加控件通知处理程序代码
	SetDlgItemText(IDC_EDITOUTPUT, _T(""));
}
