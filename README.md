# KubeJSRecipeHelper

KubeJSRecipeHelper 是一个 Windows 桌面程序，用图形界面生成 KubeJS 配方脚本。程序可以从 Minecraft 或模组的 JAR 文件中读取可选的物品、方块和流体 ID，在配方对话框中选择原料与产物，并预览生成的 JavaScript。

## 功能

- 生成有序合成、无序合成、锻造、熔炉、高炉、烟熏炉和切石机配方。
- 生成机械动力（Create）的处理配方，包括冲压、研磨、粉碎、切削、混合搅拌、塑形、注液、分液、动力合成和序列装配等方法。
- 从 JAR 文件中的物品与方块模型、流体标签等资源收集 ID；存在对应贴图时，在配方格子中显示预览。
- 在主窗口预览脚本，并将其追加写入 `output.js`。

本程序只生成脚本文本，不会修改游戏实例或自动安装配方。生成的脚本应根据所使用的 KubeJS 与模组版本进行检查。

## 构建要求

- Windows。
- Visual Studio，安装“使用 C++ 的桌面开发”及 MFC 组件。
- Windows 10 SDK。项目当前配置使用 MSVC `v145` 平台工具集和动态链接的 MFC；如果本机没有该工具集，需要在项目属性中调整平台工具集。

## 构建与运行

1. 在 Visual Studio 中打开仓库根目录的 `KubeJSRecipeHelper.slnx`。
2. 选择 `Debug` 或 `Release` 配置，以及 `x64` 或 `Win32` 平台。
3. 构建并运行 `KubeJSRecipeHelper` 项目。

项目包含 `miniz` 源码，用于读取 JAR 文件。运行程序不需要单独安装 `miniz`。

## 使用方法

1. 点击“导入 .jar 文件”，选择包含所需物品或流体资源的 JAR。可以多次导入；识别出的 ID 会加入当前会话的可选列表。
2. 选择配方类型，点击格子选择原料与产物。机械动力对话框还可以设置相应的处理参数。
3. 点击配方对话框中的“确定”，在主窗口的输出框查看脚本。
4. 复制输出框内容，或点击“一键输出”。后者会以追加方式写入程序**当前工作目录**中的 `output.js`；再次输出不会覆盖已有内容。
5. 检查并整理生成的脚本，将其放入游戏实例的 `kubejs/server_scripts/` 目录。配方脚本的放置位置参见 [KubeJS 官方文档](https://kubejs.com/wiki/folder-structure/server-scripts)。

JAR 解析依赖文件中的资源结构，未被识别的项目不会出现在选择列表中。输出前请确认物品 ID、数量、概率及生成的脚本符合目标游戏环境。

## 测试

`tests/CreateRecipeModelTests.cpp` 测试机械动力配方的生成与输入校验，并写出用于脚本检查的样例文件。`tests/ValidateCreateScripts.js` 使用 Node.js 解析这些样例，检查生成脚本的结构。

在已配置 MSVC 的开发者命令行中，从仓库根目录执行：

```bat
cl /nologo /EHsc /std:c++17 /DCREATE_MODEL_STANDALONE tests\CreateRecipeModelTests.cpp KubeJSRecipeHelper\CreateRecipeModel.cpp /Fe:CreateRecipeModelTests.exe
CreateRecipeModelTests.exe create-fixtures.js
node tests\ValidateCreateScripts.js create-fixtures.js
```

上述命令需要 Node.js，并会在当前目录生成测试可执行文件、目标文件和 `create-fixtures.js`。测试完成后可删除这些生成文件。

## 项目结构

- `KubeJSRecipeHelper/`：MFC 程序、配方对话框、JAR 解析及脚本生成代码。
- `KubeJSRecipeHelper/CreateRecipeModel.*`：机械动力配方的数据模型、校验和脚本生成。
- `tests/`：机械动力配方模型及生成脚本的测试。

## 参与贡献

欢迎通过 GitHub Issues 报告问题或提出建议。提交代码变更时，请说明复现步骤或使用场景，并尽可能附上相关测试结果。对配方生成逻辑的修改应同时检查生成脚本的内容。

## 许可证

仓库目前未包含项目许可证文件。
