// CDlgShapeless.h: 头文件
//

#pragma once
#include "CDlgCraftBase.h"


// CDlgShapeless 对话框：无序合成
//
// 界面（九宫格 + 输出格）跟有序合成共用 CDlgCraftBase，这里只负责生成
// KubeJS 无序合成脚本（event.shapeless）。无序合成只看材料的种类和数量，
// 材料摆在哪个格子都一样。
class CDlgShapeless : public CDlgCraftBase
{
	DECLARE_DYNAMIC(CDlgShapeless)

public:
	CDlgShapeless(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDlgShapeless();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLGSHAPELESS };
#endif

protected:
	// 按九宫格的内容生成 KubeJS 无序合成脚本
	virtual CString BuildRecipeScript() const;
	// 无序合成的材料一样受工作台的九格限制，超了先提醒
	virtual BOOL ValidateSlots(CString& strError) const;
	virtual LPCTSTR GetHintText() const;

	DECLARE_MESSAGE_MAP()
};
