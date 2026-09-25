# KubeJSRecipeHelper - 项目文档

## 📋 目录
1. [项目概览](#项目概览)
2. [快速开始](#快速开始)
3. [系统环境要求](#系统环境要求)
4. [项目架构](#项目架构)
5. [项目结构](#项目结构)
6. [模块说明](#模块说明)
7. [编译和构建](#编译和构建)
8. [运行和调试](#运行和调试)
9. [常见功能流程](#常见功能流程)
10. [代码规范](#代码规范)
11. [扩展开发指南](#扩展开发指南)
12. [常见问题](#常见问题)

---

## 项目概览

### 项目信息
- **项目名称**：KubeJSRecipeHelper
- **项目描述**：一个 MFC 桌面应用，用于解析和处理 KubeJS 配方（Recipe）数据
- **应用类型**：MFC (Microsoft Foundation Classes) C++ 桌面应用
- **开发语言**：C++
- **用户界面框架**：MFC Dialog  
- **目标平台**：Windows (Win32/x64)

### 核心功能
该工具提供以下配方类型的导入和处理功能：

| 功能 | 说明 | 对话框类 |
|------|------|---------|
| 有型配方 | 工作台配方 | `CDlgShaped` |
| 无型配方 | 流体工作台 | `CDlgShapeless` |
| 锻造台 | 工作台配方 | `CDlgSmith` |
| 烹饪 | 炉火配方 | `CDlgCook` |
| 石头切割 | 特殊加工 | `CDlgStonecut` |
| 导入/创建 | JAR 导入解析 | `CDlgCreate` |

### 技术特点
- ✅ 支持多平台编译 (Win32 / x64)
- ✅ 支持 Debug / Release 构建
- ✅ 使用预编译头 (PCH) 加快编译速度
- ✅ 启用 SDL 安全检查
- ✅ Unicode 字符集支持
- ✅ 动态 MFC 库链接

---

## 快速开始

### 安装依赖
本项目不需要额外的第三方库下载，仅需按照系统环境要求配置编钓环境。

### 第一次开发
1. 使用 Visual Studio 2019 或 2022 打开 `KubeJSRecipeHelper.sln`
2. 在菜单中选择 **Build → Build Solution** 进行首次编译
3. 编译成功后，按 **F5** 或点击 **Debug → Start Debugging** 运行应用

### 构建应用
```bash
# 使用 Visual Studio 构建（推荐）
# 打开解决方案文件，使用 Visual Studio IDE 构建

# 或使用命令行构建
msbuild KubeJSRecipeHelper.sln /p:Configuration=Release /p:Platform=x64
```

---

## 系统环境要求

### 硬件要求
- **处理器**：x86/x64 兼容处理器
- **内存**：最少 2GB RAM
- **磁盘**：最少 100MB 可用空间

### 开发环境要求
| 项目 | 要求 | 说明 |
|------|------|------|
| **操作系统** | Windows 7 SP1 或更高版本 | 支持 Windows 10/11 |
| **编译器** | Visual C++ v145 (MSVC 14.0) | 对应 Visual Studio 2019/2022 |
| **Visual Studio** | VS 2019 或 VS 2022 Community/Professional | 需安装 Desktop C++ 工作负载 |
| **Windows SDK** | Windows 10 SDK 或更高版本 | 自动选择最高可用版本 |
| **.NET Framework** | 4.7.2 或更高版本 | 用于 Visual Studio 本身 |
| **字符集** | Unicode | 必需 |
| **MSVC** | 14.0 或更高版本 | 默认包含在 VS 2019+ 中 |

### 编译器配置
```
编译器：Visual C++ v145 (MSVC 14.0)
目标平台：Windows 10.0 SDK
字符集：Unicode (支持多字节字符)
MFC 库：Dynamic (共享 MFCXXXXU.DLL)
```

### 运行时环境要求
- Windows Vista 或更高版本
- Visual C++ Redistributable for Visual Studio 2019+
  - 下载地址：https://support.microsoft.com/en-us/help/2977003

---

## 项目架构

### 应用程序架构图

```
┌─────────────────────────────────────────────────────────┐
│               KubeJSRecipeHelper 应用                    │
├─────────────────────────────────────────────────────────┤
│  CKubeJSRecipeHelperApp (应用类)                        │
│    ↓ 初始化 InitInstance()                              │
├─────────────────────────────────────────────────────────┤
│  CKubeJSRecipeHelperDlg (主窗口对话框)                  │
│  ├─ [有型配方]  ─→ CDlgShaped Dialog                    │
│  ├─ [无型配方]  ─→ CDlgShapeless Dialog                 │
│  ├─ [锻造台]    ─→ CDlgSmith Dialog                     │
│  ├─ [烹饪]      ─→ CDlgCook Dialog                      │
│  ├─ [石头切割]  ─→ CDlgStonecut Dialog                  │
│  ├─ [导入/创建] ─→ CDlgCreate Dialog                    │
│  └─ [导入]      ─→ 文件对话框 + JAR 解析               │
├─────────────────────────────────────────────────────────┤
│  [可选的业务逻辑层]                                     │
│  JAR 文件解析模块 / 配方数据处理模块                     │
└─────────────────────────────────────────────────────────┘
```

### 消息流程

```
用户交互
  ↓
主对话框按钮事件 (OnBnClicked*)
  ↓
打开子对话框 (DoModal)
  ↓
子对话框处理 / JAR 文件选择
  ↓
返回主对话框
```

---

## 项目结构

### 完整项目树

```
KubeJSRecipeHelper/
│
├── 📄 KubeJSRecipeHelper.vcxproj     ← 项目文件
├── 📄 KubeJSRecipeHelper.sln         ← 解决方案文件
│
├── 📁 KubeJSRecipeHelper/            ← 项目源代码目录
│   │
│   ├── 📄 KubeJSRecipeHelper.h       ← 应用程序主头文件
│   ├── 📄 KubeJSRecipeHelper.cpp     ← 应用程序实现
│   ├── 📄 KubeJSRecipeHelperDlg.h    ← 主窗口头文件
│   ├── 📄 KubeJSRecipeHelperDlg.cpp  ← 主窗口实现
│   │
│   ├── 📄 CDlgShaped.h               ← 有型配方对话框头文件
│   ├── 📄 CDlgShaped.cpp             ← 有型配方对话框实现
│   ├── 📄 CDlgShapeless.h            ← 无型配方对话框头文件
│   ├── 📄 CDlgShapeless.cpp          ← 无型配方对话框实现
│   ├── 📄 CDlgSmith.h                ← 锻造台对话框头文件
│   ├── 📄 CDlgSmith.cpp              ← 锻造台对话框实现
│   ├── 📄 CDlgCook.h                 ← 烹饪对话框头文件
│   ├── 📄 CDlgCook.cpp               ← 烹饪对话框实现
│   ├── 📄 CDlgStonecut.h             ← 石头切割对话框头文件
│   ├── 📄 CDlgStonecut.cpp           ← 石头切割对话框实现
│   ├── 📄 CDlgCreate.h               ← 导入/创建对话框头文件
│   ├── 📄 CDlgCreate.cpp             ← 导入/创建对话框实现
│   │
│   ├── 📄 pch.h                      ← 预编译头文件
│   ├── 📄 pch.cpp                    ← 预编译头实现
│   ├── 📄 framework.h                ← MFC 框架包含文件
│   ├── 📄 targetver.h                ← 目标平台版本定义
│   ├── 📄 Resource.h                 ← 资源定义
│   │
│   ├── 📄 KubeJSRecipeHelper.rc      ← 资源脚本文件
│   │
│   ├── 📁 res/                       ← 资源文件夹
│   │   ├── 📄 KubeJSRecipeHelper.ico ← 应用程序图标
│   │   └── 📄 KubeJSRecipeHelper.rc2 ← 资源配置
│
└── 📄 Project_Documentation.md        ← 本文档
```

### 关键文件说明

| 文件/目录 | 说明 |
|---------|------|
| `KubeJSRecipeHelper.vcxproj` | Visual Studio 项目配置文件 |
| `KubeJSRecipeHelper.h` | 应用主类定义 (CKubeJSRecipeHelperApp) |
| `KubeJSRecipeHelperDlg.h/.cpp` | 主窗口类实现 |
| `CDlgXxx.h/.cpp` | 各功能模块的对话框类 |
| `framework.h` | MFC 框架头文件包含 |
| `pch.h / pch.cpp` | 预编译头（加快编译速度） |
| `Resource.h` | 资源常量定义 |
| `KubeJSRecipeHelper.rc` | 资源脚本（对话框、菜单、图标等） |

---

## 模块说明

### 1. 应用程序入口 (CKubeJSRecipeHelperApp)

**文件**：KubeJSRecipeHelper.h / KubeJSRecipeHelper.cpp

**职责**：
- 应用程序初始化 (InitInstance)
- Windows 消息分发
- 重启管理器支持

**主要方法**：
```cpp
BOOL InitInstance()   // 应用初始化，创建主窗口
```

**特点**：
- 启用重启管理器 (`AFX_RESTART_MANAGER_SUPPORT_RESTART`)
- 初始化公共控件
- 启用 Shell 管理器和控件容器支持

### 2. 主窗口对话框 (CKubeJSRecipeHelperDlg)

**文件**：KubeJSRecipeHelperDlg.h / KubeJSRecipeHelperDlg.cpp

**职责**：
- 主用户界面
- 导航菜单（7 个按钮）
- 文件导入功能

**主要方法**：
```cpp
BOOL OnInitDialog()                       // 初始化对话框
void OnBnClickedBtnshaped()              // 有型配方
void OnBnClickedBtnshapeless()           // 无型配方
void OnBnClickedBtnsmith()               // 锻造台
void OnBnClickedBtncook()                // 烹饪
void OnBnClickedBtnstonecut()            // 石头切割
void OnBnClickedBtncreate()              // 创建/导入
void OnBnClickedBtnimport()              // 导入 JAR 文件
```

**功能流程**：

```
1. 用户点击菜单按钮
   ↓
2. 触发对应的 OnBnClicked* 事件处理函数
   ↓
3. 创建子对话框对象
   ↓
4. 调用 DoModal() 显示模态对话框
   ↓
5. 等待用户交互
   ↓
6. 对话框关闭，返回
```

**JAR 导入流程**：

```cpp
void OnBnClickedBtnimport()
{
	// 1. 创建文件对话框，筛选 .jar 文件
	CFileDialog dlgFile(TRUE,                          // 打开文件
					   _T("jar"),                       // 默认扩展名
					   nullptr,                         // 默认文件名
					   OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
					   _T("JAR 文件 (*.jar)|*.jar|所有文件 (*.*)|*.*||"),
					   this);

	// 2. 显示对话框，等待用户选择
	if (dlgFile.DoModal() != IDOK)
		return;

	// 3. 获取选中文件路径
	CString strJarPath = dlgFile.GetPathName();

	// 4. TODO: 使用 strJarPath 进行 JAR 文件解析
}
```

### 3. 配方对话框模块

#### 3.1 有型配方 (CDlgShaped)
- **文件**：CDlgShaped.h / CDlgShaped.cpp
- **功能**：处理工作台形状配方
- **状态**：框架完成，具体逻辑待实现

#### 3.2 无型配方 (CDlgShapeless)
- **文件**：CDlgShapeless.h / CDlgShapeless.cpp
- **功能**：处理无形状配方（流体工作台等）
- **状态**：框架完成，具体逻辑待实现

#### 3.3 锻造台 (CDlgSmith)
- **文件**：CDlgSmith.h / CDlgSmith.cpp
- **功能**：处理锻造台配方
- **状态**：框架完成，具体逻辑待实现

#### 3.4 烹饪 (CDlgCook)
- **文件**：CDlgCook.h / CDlgCook.cpp
- **功能**：处理炉火烹饪配方
- **状态**：框架完成，具体逻辑待实现

#### 3.5 石头切割 (CDlgStonecut)
- **文件**：CDlgStonecut.h / CDlgStonecut.cpp
- **功能**：处理石头切割机配方
- **状态**：框架完成，具体逻辑待实现

#### 3.6 导入/创建 (CDlgCreate)
- **文件**：CDlgCreate.h / CDlgCreate.cpp
- **功能**：创建新配方或导入配方
- **状态**：框架完成，具体逻辑待实现

**对话框学习代码示例**：
```cpp
class CDlgShaped : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgShaped)

public:
	CDlgShaped(CWnd* pParent = nullptr);  // 构造
	virtual ~CDlgShaped();                 // 析构

	enum { IDD = IDD_DLGSHAPED };          // 对话框 ID

protected:
	virtual void DoDataExchange(CDataExchange* pDX);  // 数据交换

	DECLARE_MESSAGE_MAP()
};
```

### 4. 框架和支持文件

#### framework.h
**内容**：
- `afxwin.h` - MFC 核心组件
- `afxext.h` - MFC 扩展
- `afxdisp.h` - MFC 自动化类
- `afxdtctl.h` - Internet Explorer 4 公共控件
- `afxcmn.h` - Windows 公共控件
- `afxcontrolbars.h` - 功能区和控制条

**用途**：集中包含所有必需的 MFC 头文件

#### pch.h / pch.cpp
**用途**：
- 预编译头，加快编译速度
- 包含频繁使用的稳定头文件
- 减少重复编译开销

**最佳实践**：
- 将稳定的头文件放入 pch.h
- 不要频繁修改 pch.h
- 所有源文件都应包含 pch.h

#### targetver.h
**内容**：Windows SDK 目标版本定义

```cpp
#include <SDKDDKVer.h>
// 自动定义 _WIN32_WINNT 为最高可用版本
```

#### Resource.h
**内容**：资源常量定义
```cpp
#define IDD_DLGSHAPED        101
#define IDD_DLGSHAPELESS     102
// ... 其他资源 ID
```

---

## 编译和构建

### 编译配置

项目支持 4 种编译配置：

| 配置 | 平台 | 用途 |
|------|------|------|
| Debug \| Win32 | 32 位 | 开发调试 |
| Debug \| x64 | 64 位 | 开发调试 |
| Release \| Win32 | 32 位 | 发布部署 |
| Release \| x64 | 64 位 | 发布部署 |

### 编译选项

#### Debug 配置
- 预编译头：使用 (Use)
- 优化：禁用 (/Od)
- 运行时库：多线程调试 DLL (/MDd)
- 链接增量：是 (true)
- 调试信息：程序数据库 (/Zi)

#### Release 配置
- 预编译头：使用 (Use)
- 优化：最大化速度 (/O2，依赖于配置)
- 运行时库：多线程 DLL (/MD)
- 链接增量：否 (false)
- COMDAT 折叠：启用
- 引用优化：启用
- 函数级链接：启用
- 内在函数：启用

### 在 Visual Studio 中构建

#### 方法 1：使用菜单
1. 打开 `KubeJSRecipeHelper.sln`
2. 选择 **Build** → **Build Solution** (快捷键: Ctrl+Shift+B)
3. 或选择 **Build** → **Rebuild Solution** 清空重建

#### 方法 2：使用快捷键
- **Ctrl+Shift+B** - 构建解决方案
- **F7** - 构建项目
- **Shift+F7** - 重建项目

#### 方法 3：选择特定配置
1. 在工具栏中选择配置 (Debug/Release)
2. 选择平台 (Win32/x64)
3. Ctrl+Shift+B 构建

### 命令行构建

```bash
# 使用 msbuild 构建
msbuild KubeJSRecipeHelper.sln /p:Configuration=Release /p:Platform=x64

# 清空和重建
msbuild KubeJSRecipeHelper.sln /p:Configuration=Release /p:Platform=x64 /t:Clean,Build

# 仅构建特定项目
msbuild KubeJSRecipeHelper.vcxproj /p:Configuration=Debug /p:Platform=Win32
```

### 输出文件位置

```
${ProjectDir}/
├── Debug/                       ← Debug 配置输出
│   ├── KubeJSRecipeHelper.exe
│   ├── KubeJSRecipeHelper.pdb
│   └── ... (其他调试文件)
└── Release/                     ← Release 配置输出
	├── KubeJSRecipeHelper.exe
	└── ... (优化后的文件)
```

---

## 运行和调试

### 直接运行

1. 编译完成后，按 **Ctrl+Shift+B** 确保构建成功
2. 找到输出的 `.exe` 文件：
   - Debug 版本：`Debug/KubeJSRecipeHelper.exe`
   - Release 版本：`Release/KubeJSRecipeHelper.exe`
3. 双击运行

### 调试运行

1. 设置断点（在代码行左边点击红圆）
2. 按 **F5** 或选择 **Debug → Start Debugging**
3. 应用启动后，执行到断点时暂停
4. 使用调试工具栏或快捷键：
   - **F10** - 单步跳过
   - **F11** - 单步进入
   - **Shift+F11** - 单步退出
   - **F5** - 继续运行

### 调试窗口

| 窗口 | 快捷键 | 用途 |
|------|--------|------|
| 输出 | Ctrl+Alt+O | 查看编译和调试输出 |
| 监视 | Ctrl+Alt+W | 监视变量值 |
| 局部变量 | Ctrl+Alt+V | 查看局部变量 |
| 调用堆栈 | Ctrl+Alt+C | 查看函数调用栈 |
| 断点 | Ctrl+Alt+B | 管理断点 |
| 线程 | Ctrl+Alt+H | 查看线程信息 |

### 常见调试技巧

#### 1. 条件断点
- 右键点击断点 → 筛选器
- 设置条件表达式

#### 2. 内存查看
- Debug → Windows → Memory → Memory 1/2/3/4
- 输入变量地址查看内存

#### 3. 输出调试信息
```cpp
// 使用 OutputDebugString 输出到 Visual Studio 输出窗口
OutputDebugString(_T("调试信息\n"));
```

#### 4. 运行时异常捕获
- Debug → Exceptions (Ctrl+Alt+E)
- 选择异常类型和处理方式

---

## 常见功能流程

### 流程 1：启动应用

```
用户双击应用.exe
  ↓
进入 CKubeJSRecipeHelperApp::InitInstance()
  ↓
初始化公共控件 (InitCommonControlsEx)
  ↓
创建 Shell 管理器
  ↓
启用 MFC 控件容器
  ↓
创建主对话框 CKubeJSRecipeHelperDlg
  ↓
显示主对话框 (DoModal)
  ↓
进入消息循环 (WndProc)
  ↓
等待用户交互
```

### 流程 2：用户点击配方按钮

```
用户点击 [有型配方] 按钮
  ↓
触发 OnBnClickedBtnshaped() 事件
  ↓
创建 CDlgShaped 对象
  ↓
调用 DoModal() 显示对话框
  ↓
对话框进入消息循环
  ↓
用户操作对话框 / 点击确定/cancel 按钮
  ↓
对话框关闭
  ↓
返回主对话框
```

### 流程 3：用户导入 JAR 文件

```
用户点击 [导入] 按钮
  ↓
触发 OnBnClickedBtnimport() 事件
  ↓
创建 CFileDialog 文件对话框
  ↓
设置筛选器为 *.jar 文件
  ↓
显示文件选择对话框
  ↓
用户选择 JAR 文件 → 点击确定
  ↓
获取文件路径 (GetPathName)
  ↓
关闭对话框
  ↓
返回主对话框
  ↓
[TODO] 使用 strJarPath 进行 JAR 解析
```

---

## 代码规范

### 命名规范

#### 类命名
- **MFC 类**：`CXxx` 前缀（如 `CKubeJSRecipeHelperApp`）
- **对话框**：`CDlgXxx` 前缀（如 `CDlgShaped`）
- **自定义类**：`CXxx` 前缀

```cpp
class CMyCustomClass { };
```

#### 变量命名
- **成员变量**：`m_` 前缀
  ```cpp
  class CMyClass {
	  HICON m_hIcon;
	  CString m_strName;
  };
  ```

- **本地变量**：小驼峰命名
  ```cpp
  CString strJarPath;
  int nCount = 0;
  ```

- **指针**：`p` 前缀
  ```cpp
  CWnd* pParent;
  ```

#### 方法命名
- **事件处理**：`OnXxx` 前缀
  ```cpp
  void OnBnClickedBtnshaped();
  void OnInitDialog();
  void OnPaint();
  ```

- **消息处理**：`On消息类型` 格式
  ```cpp
  void OnWmPaint();
  void OnBnClicked();
  ```

- **其他方法**：动词开头，小驼峰
  ```cpp
  void InitializeControls();
  CString GetFilePath();
  ```

#### 常量和定义
- **宏定义**：大写 + 下划线
  ```cpp
  #define MAX_PATH_LENGTH 260
  #define BATCH_SIZE 100
  ```

- **常量**：`const` + 小驼峰
  ```cpp
  const int DEFAULT_TIMEOUT = 5000;
  ```

### 代码风格

#### 缩进
- 使用 4 个空格（不使用 Tab）
- Visual Studio 默认设置即可

#### 括号风格
```cpp
// 函数定义：K&R 风格
void MyFunction()
{
	if (condition)
	{
		// 代码
	}
	else
	{
		// 代码
	}
}

// 消息映射：MFC 标准
BEGIN_MESSAGE_MAP(CMyClass, CDialogEx)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_BUTTON, &CMyClass::OnBnClicked)
END_MESSAGE_MAP()
```

#### 注释规范
```cpp
// 单行注释用 //

/*
 * 多行注释用 /* */
 * 每行末尾输入 *
 */

// TODO: 待实现功能
// FIXME: 已知缺陷
// NOTE: 重要提示
// HACK: 临时解决方案
```

#### 文件头注释
```cpp
// MyFile.cpp: 功能描述
//
// 创建日期：2024-01-15
// 作者：Your Name
// 描述：该文件实现 xxx 功能
```

### 最佳实践

#### 1. 使用 Unicode
```cpp
// ✅ 好
CString str = _T("你好 World");
LPCTSTR pszStr = _T("字符串");

// ❌ 不好
CString str = "你好";  // 易乱码
```

#### 2. 资源管理
```cpp
// ✅ 好：自动释放
{
	CString str;  // 函数结束时自动释放
	// 使用 str
}

// ❌ 不好：手动管理
LPCTSTR pszStr = new TCHAR[100];
// ... 容易忘记 delete
```

#### 3. 错误处理
```cpp
// ✅ 好
CFileDialog dlg(TRUE);
if (dlg.DoModal() != IDOK)
	return;

// ❌ 坏：忽略返回值
dlg.DoModal();
CString strPath = dlg.GetPathName();  // 可能失败
```

#### 4. 指针使用
```cpp
// ✅ 好
CWnd* pParent = GetParent();
if (pParent != nullptr)
{
	pParent->ShowWindow(SW_HIDE);
}

// ❌ 坏
CWnd* pParent = GetParent();
pParent->ShowWindow(SW_HIDE);  // 可能崩溃
```

---

## 扩展开发指南

### 添加新的配方类型

#### 第 1 步：创建新的对话框类

创建文件 `CDlgXxx.h`：
```cpp
#pragma once
#include "afxdialogex.h"

// CDlgXxx 对话框
class CDlgXxx : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgXxx)

public:
	CDlgXxx(CWnd* pParent = nullptr);   // 标准构造
	virtual ~CDlgXxx();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DLGXXX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
```

创建文件 `CDlgXxx.cpp`：
```cpp
#include "pch.h"
#include "KubeJSRecipeHelper.h"
#include "afxdialogex.h"
#include "CDlgXxx.h"

IMPLEMENT_DYNAMIC(CDlgXxx, CDialogEx)

CDlgXxx::CDlgXxx(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLGXXX, pParent)
{
}

CDlgXxx::~CDlgXxx()
{
}

void CDlgXxx::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	// TODO: 添加 DDX/DDV 映射
}

BEGIN_MESSAGE_MAP(CDlgXxx, CDialogEx)
	// TODO: 添加消息映射
END_MESSAGE_MAP()

// CDlgXxx 消息处理程序
```

#### 第 2 步：在主对话框中添加按钮事件

编辑 `KubeJSRecipeHelperDlg.h`：
```cpp
class CKubeJSRecipeHelperDlg : public CDialogEx
{
	// ... 其他成员 ...
public:
	afx_msg void OnBnClickedBtnxxx();  // 添加新的事件处理声明
};
```

编辑 `KubeJSRecipeHelperDlg.cpp`：
```cpp
#include "CDlgXxx.h"

BEGIN_MESSAGE_MAP(CKubeJSRecipeHelperDlg, CDialogEx)
	// ... 其他消息映射 ...
	ON_BN_CLICKED(IDC_BTNXXX, &CKubeJSRecipeHelperDlg::OnBnClickedBtnxxx)
END_MESSAGE_MAP()

void CKubeJSRecipeHelperDlg::OnBnClickedBtnxxx()
{
	CDlgXxx dlg;
	dlg.DoModal();
}
```

#### 第 3 步：在资源文件中添加对话框

1. 打开 `KubeJSRecipeHelper.rc`
2. 创建新的对话框资源 `IDD_DLGXXX`
3. 添加对应的按钮控件

#### 第 4 步：在项目文件中注册新文件

编辑 `KubeJSRecipeHelper.vcxproj`：
```xml
<ItemGroup>
	<ClInclude Include="CDlgXxx.h" />
</ItemGroup>
<ItemGroup>
	<ClCompile Include="CDlgXxx.cpp" />
</ItemGroup>
```

### 添加新的功能模块

#### 示例：添加配方解析模块

创建 `RecipeParser.h`：
```cpp
#pragma once

class CRecipeParser
{
public:
	CRecipeParser();
	virtual ~CRecipeParser();

	// 核心方法
	BOOL ParseJarFile(const CString& strJarPath);
	BOOL ParseRecipe(const CString& strRecipePath);

	// 数据访问
	CString GetRecipeName() const;

private:
	CString m_strName;
};
```

创建 `RecipeParser.cpp`：
```cpp
#include "pch.h"
#include "RecipeParser.h"

CRecipeParser::CRecipeParser()
{
}

CRecipeParser::~CRecipeParser()
{
}

BOOL CRecipeParser::ParseJarFile(const CString& strJarPath)
{
	// TODO: 实现 JAR 文件解析逻辑
	return TRUE;
}

// 其他方法实现...
```

### 添加单元测试

创建测试项目 `KubeJSRecipeHelper_Tests.vcxproj`（使用 Google Test 或 CppUnit）：

```cpp
#include <gtest/gtest.h>
#include "RecipeParser.h"

TEST(RecipeParserTest, ParseSimpleRecipe)
{
	CRecipeParser parser;
	EXPECT_TRUE(parser.ParseJarFile(_T("test.jar")));
	EXPECT_TRUE(parser.GetRecipeName().GetLength() > 0);
}
```

### 性能优化建议

1. **使用预编译头** - 已配置为 pch.h
2. **异步文件 I/O** - 大文件使用线程
3. **缓存解析结果** - 避免重复解析
4. **UI 响应** - 耗时操作使用工作线程
5. **内存管理** - 及时释放大对象

---

## 常见问题

### Q1: 编译时提示找不到头文件？

**A**: 检查以下几点：
1. 项目包含路径配置（项目属性 → VC++ 目录 → 包含目录）
2. 预编译头是否正确配置
3. pch.h 是否被所有源文件包含

```cpp
// 每个源文件开头都应该有
#include "pch.h"
```

### Q2: 调试模式下运行缓慢？

**A**: 这是正常现象，Debug 模式包含大量调试信息。
- 发布版本：选择 Release 配置编译
- 使用菜单：Build → Configuration Manager → 选择 Release
- 或使用快捷键切换

### Q3: 如何打包发布应用？

**A**: 需要以下文件：
1. KubeJSRecipeHelper.exe（Release 版本）
2. 必要的 DLL 文件：
   - 如果使用动态 MFC：MFCXXXU.DLL（需要 VC++ 运行库）
3. 第三方库（如果有）

**发布步骤**：
```bash
1. 编译 Release|x64 版本
2. 收集所有必需的 DLL
3. 创建安装程序或绿色版本
4. 包含必需的配置文件
5. 创建快捷方式和文档
```

### Q4: 模态对话框如何传递数据？

**A**: 使用公共成员变量或 GetDlgItemText：

```cpp
// 在对话框头文件中
class CDlgShaped : public CDialogEx
{
	// ...
public:
	CString m_strResult;  // 输出数据
};

// 在主窗口中
CDlgShaped dlg;
if (dlg.DoModal() == IDOK)
{
	CString strResult = dlg.m_strResult;  // 获取结果
}
```

### Q5: 如何实现异步操作？

**A**: 使用 Windows 线程或 MFC 线程类：

```cpp
// 创建工作线程
AfxBeginThread(ThreadProc, nullptr);

// 线程函数
UINT ThreadProc(LPVOID pParam)
{
	// 执行耗时操作
	return 0;
}

// 通知主线程
PostMessage(WM_CUSTOM_MESSAGE, wParam, lParam);
```

### Q6: 如何处理异常？

**A**: 使用 try-catch 或 MFC 异常类：

```cpp
try
{
	CFile file;
	file.Open(_T("file.txt"), CFile::modeRead);
	// 文件操作
	file.Close();
}
catch (CFileException* e)
{
	TCHAR szCause[256];
	e->GetErrorMessage(szCause, 256);
	MessageBox(szCause, _T("错误"));
	e->Delete();
}
```

### Q7: 如何调试消息映射？

**A**: 使用 Message Tracing：
1. Debug → Windows → Spy++
2. 选择目标窗口
3. 查看消息流

### Q8: 项目文件无法打开怎么办？

**A**: 
1. 检查 SDK 安装：Visual C++ 工具集需要 v145
2. 修复 Visual Studio：Repair Visual Studio Installation
3. 重新创建项目文件

### Q9: 如何禁用警告？

**A**: 在 .cpp 开头添加：
```cpp
#pragma warning(disable: 4996)  // 禁用特定警告
```

或在项目属性中配置：
- 项目属性 → C/C++ → 高级 → 禁用特定警告

### Q10: 如何与数据库集成？

**A**: 使用以下库：
1. **ODBC** - Windows 原生支持
2. **OLE DB** - Microsoft 数据访问
3. **SQLite** - 轻量级嵌入式数据库

示例（SQLite）：
```cpp
// 需要 sqlite3.h 和 sqlite3.lib
#include <sqlite3.h>

sqlite3* db;
sqlite3_open("recipe.db", &db);
// 执行 SQL 操作
sqlite3_close(db);
```

---

## 相关资源

### MFC 文档
- [Microsoft MFC 官方文档](https://docs.microsoft.com/zh-cn/cpp/mfc/mfc-desktop-applications)
- [MFC 对话框编程](https://docs.microsoft.com/zh-cn/cpp/mfc/dialog-boxes)
- [MFC 消息映射](https://docs.microsoft.com/zh-cn/cpp/mfc/message-maps)

### 开发工具
- [Visual Studio 2022 下载](https://visualstudio.microsoft.com/zh-hans/)
- [Windows SDK 下载](https://developer.microsoft.com/zh-cn/windows/downloads/windows-sdk/)
- [Spy++ 工具](https://docs.microsoft.com/en-us/visualstudio/debugger/general-debugging-options-dialog-box)

### C++ 标准
- [C++17 参考](https://en.cppreference.com/w/cpp/17)
- [Windows API 参考](https://docs.microsoft.com/en-us/windows/win32/api/)

---

## 更新历史

| 版本 | 日期 | 说明 |
|------|------|------|
| 1.0 | 2024-01-15 | 初始创建 |

---

## 贡献指南

### 报告问题
1. 检查 Issues 中是否已有相同问题
2. 提供详细的重现步骤
3. 附加错误日志和环境信息

### 提交改进
1. Fork 项目
2. 创建特性分支：`git checkout -b feature/xxx`
3. 提交改动：`git commit -m "Add feature xxx"`
4. 推送分支：`git push origin feature/xxx`
5. 提交 Pull Request

### 编码标准
- 遵循本文档的代码规范
- 添加必要的注释
- 包含单元测试
- 执行代码检查 (Code Review)

---

## 许可证

本项目的许可证信息请参考项目根目录的 `LICENSE` 文件。

---

**最后更新**：2024-01-15  
**维护者**：Project Team  
**问题反馈**：请在 GitHub Issues 中报告

---

**文档完成！**

本文档涵盖了项目的所有关键信息，包括架构、构建、调试、最佳实践和常见问题。
下次开发时，可以快速参考此文档了解项目结构和开发流程。

