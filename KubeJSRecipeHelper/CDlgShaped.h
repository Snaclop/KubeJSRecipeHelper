// CDlgShaped.h: 头文件
//

#pragma once
#include "CDlgCraftBase.h"


// CDlgShaped 对话框：有序合成
//
// 九宫格和输出格的界面部分在 CDlgCraftBase 里，这里只负责按九宫格的形状
// 生成 KubeJS 有序合成脚本（event.shaped）。
class CDlgShaped : public CDlgCraftBase
{
	DECLARE_DYNAMIC(CDlgShaped)

public:
	CDlgShaped(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDlgShaped();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLGSHAPED };
#endif

protected:
	// 按九宫格的内容生成 KubeJS 有序合成脚本
	virtual CString BuildRecipeScript() const;

	DECLARE_MESSAGE_MAP()
};
