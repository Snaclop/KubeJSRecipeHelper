
// KubeJSRecipeHelperDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "KubeJSRecipeHelper.h"
#include "KubeJSRecipeHelperDlg.h"
#include "afxdialogex.h"
#include "Language.h"
#include "CDlgRecipeChanges.h"
#include "CDlgVanilla.h"
#include "CDlgCreate.h"
#include "CDlgOther.h"
#include "ScriptExport.h"
#include <atlbase.h>
#include <filesystem>

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
	DDX_Control(pDX, IDC_COMBOLANGUAGE, m_comboLanguage);
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
	ON_CBN_SELCHANGE(IDC_COMBOLANGUAGE, &CKubeJSRecipeHelperDlg::OnCbnSelchangeCombolanguage)
END_MESSAGE_MAP()


// CKubeJSRecipeHelperDlg 消息处理程序

BOOL CKubeJSRecipeHelperDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// DLGINIT 中的选项是 ANSI 文本，使用 Unicode 重填以兼容不同系统代码页。
	m_comboLanguage.ResetContent();
	m_comboLanguage.AddString(_T("English"));
	m_comboLanguage.AddString(_T("简体中文"));
	m_comboLanguage.AddString(_T("繁體中文"));

	UiLanguage::Set(UiLanguage::SystemDefault());
	m_comboLanguage.SetCurSel(static_cast<int>(UiLanguage::Get()));
	UiLanguage::ApplyDialog(*this);

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
		int nIconWidth = GetSystemMetrics(SM_CXICON);
		int nIconHeight = GetSystemMetrics(SM_CYICON);
		CRect rcRect;
		GetClientRect(&rcRect);
		int nX = (rcRect.Width() - nIconWidth + 1) / 2;
		int nY = (rcRect.Height() - nIconHeight + 1) / 2;

		// 绘制图标
		dc.DrawIcon(nX, nY, m_hIcon);
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
	OpenRecipeDialog(dlg);
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtncreate()
{
	CDlgCreate dlg(this);
	OpenRecipeDialog(dlg);
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnimport()
{
	CFileDialog dlgFile(TRUE, _T("jar"), nullptr,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT,
		UiLanguage::Text(L"JAR 文件 (*.jar)|*.jar|所有文件 (*.*)|*.*||"), this);
	if (dlgFile.DoModal() != IDOK)
		return;

	// 直接枚举系统对话框的全部结果，避免固定文件名缓冲区截断多选列表。
	CComPtr<IShellItemArray> pSelectedFiles;
	pSelectedFiles.Attach(dlgFile.GetResults());
	DWORD uFileCount = 0;
	if (!pSelectedFiles || FAILED(pSelectedFiles->GetCount(&uFileCount)))
	{
		AfxMessageBox(UiLanguage::Text(L"无法读取所选文件列表。"), MB_ICONERROR);
		return;
	}

	CStringArray arrPaths;
	for (DWORD uFileIndex = 0; uFileIndex < uFileCount; ++uFileIndex)
	{
		CComPtr<IShellItem> pItem;
		CComHeapPtr<wchar_t> pPath;
		if (FAILED(pSelectedFiles->GetItemAt(uFileIndex, &pItem)) ||
			FAILED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pPath)))
		{
			AfxMessageBox(UiLanguage::Text(L"无法读取所选文件的路径。"), MB_ICONERROR);
			return;
		}
		arrPaths.Add(CString(pPath));
	}

	CString strFailures;
	int nFailedCount = 0;
	{
		CWaitCursor wait;
		// 按选择列表顺序解析，一个文件完成后才处理下一个。
		for (INT_PTR nIndex = 0; nIndex < arrPaths.GetSize(); ++nIndex)
		{
			CString strProgress;
			strProgress.Format(UiLanguage::Text(L"正在解析 %d/%d：\r\n%s"),
				(int)nIndex + 1, (int)arrPaths.GetSize(), arrPaths[nIndex].GetString());
			SetDlgItemText(IDC_EDITOUTPUT, strProgress);
			GetDlgItem(IDC_EDITOUTPUT)->UpdateWindow();

			JarImportResult result;
			std::wstring strError;
			if (!ImportJar(arrPaths[nIndex].GetString(), result, strError))
			{
				++nFailedCount;
				strFailures += arrPaths[nIndex] + _T("\r\n") + strError.c_str() + _T("\r\n");
			}
			else
			{
				const size_t nPreviousCount = m_importResult.arrNames.size();
				MergeJarImportResult(m_importResult, result);
				for (size_t nNameIndex = nPreviousCount; nNameIndex < m_importResult.arrNames.size(); ++nNameIndex)
					m_arrNames.Add(m_importResult.arrNames[nNameIndex].c_str());
				for (const auto& texture : result.mapTexturePaths)
					m_itemTextures.Register(texture.first.c_str(), arrPaths[nIndex], texture.second);
			}
		}
	}

	CString strSummary;
	strSummary.Format(UiLanguage::Text(L"导入完成：成功 %d 个 JAR，失败 %d 个。\r\n\r\n"),
		(int)arrPaths.GetSize() - nFailedCount, nFailedCount);
	RefreshNamesOutput(strSummary);
	if (nFailedCount > 0)
		AfxMessageBox(strSummary + UiLanguage::Text(L"以下文件解析失败，已继续处理其他文件：\r\n") + strFailures,
			MB_ICONWARNING);
}

void CKubeJSRecipeHelperDlg::RefreshNamesOutput(const CString& strSummary)
{
	CString strOutput;
	strOutput.Format(UiLanguage::Text(L"共找到 %d 个 item：\r\n"), (int)m_arrNames.GetSize());
	strOutput = strSummary + strOutput;
	for (INT_PTR nIndex = 0; nIndex < m_arrNames.GetSize(); ++nIndex)
	{
		strOutput += m_arrNames[nIndex];
		strOutput += _T("\r\n");
	}
	SetDlgItemText(IDC_EDITOUTPUT, strOutput);
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnclear()
{
	// TODO: 在此添加控件通知处理程序代码
	SetDlgItemText(IDC_EDITOUTPUT, _T(""));
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtndwnload()
{
	CString strOutput;
	GetDlgItemText(IDC_EDITOUTPUT, strOutput);
	std::error_code error;
	const auto pathOutput = std::filesystem::absolute(L"output.js", error);
	const std::wstring strPath = error ? L"output.js" : pathOutput.wstring();
	const std::string strUtf8(CT2A(strOutput, CP_UTF8));
	CString strMessage;
	if (error || !AppendRecipeScript(strPath, strUtf8)) {
		strMessage.Format(UiLanguage::Text(L"无法写入文件：\r\n%s"), strPath.c_str());
		MessageBox(strMessage, nullptr, MB_ICONERROR);
		return;
	}
	strMessage.Format(UiLanguage::Text(L"脚本已追加至：\r\n%s"), strPath.c_str());
	MessageBox(strMessage, nullptr, MB_ICONINFORMATION);
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnother()
{
	CDlgOther dlg(this);
	OpenRecipeDialog(dlg);
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnremove()
{
	CDlgRecipeChanges dlg(true,this);
	OpenRecipeDialog(dlg);
}

void CKubeJSRecipeHelperDlg::OnBnClickedBtnmodify()
{
	CDlgRecipeChanges dlg(false,this);
	OpenRecipeDialog(dlg);
}

void CKubeJSRecipeHelperDlg::OnCbnSelchangeCombolanguage()
{
	const int nSelectedIndex = m_comboLanguage.GetCurSel();
	if (nSelectedIndex == CB_ERR) return;
	UiLanguage::Set(static_cast<UiLanguage::Language>(nSelectedIndex));
	UiLanguage::ApplyDialog(*this);
}
