# 哪吒网络安全 IDE（NezhaIDE）

> 以 C++/Qt6 打造的安全研究集成开发环境，为渗透测试与逆向分析工作流而设计。

## 功能特性

- **代码编辑器**：多标签编辑、行号区、C++/Python/JavaScript 等语法高亮、光标位置与语言状态栏
- **资源管理器**：文件树浏览、新建文件/文件夹、重命名、在访达中显示
- **Git 面板**：变更列表、提交历史图（自绘）、差异查看、暂存/撤销暂存
- **HTTP 客户端**：GET/POST/PUT/PATCH/DELETE 等方法着色、请求头与参数表格、响应状态胶囊、正文/表头/Hex 三视图
- **Hydra 集成**：THC-Hydra 图形化前端，服务探测、用户名/密码来源管理、随机口令生成、实时日志
- **集成终端**：内置 PTY 终端，多标签、ANSI 256 色渲染
- **Hex 编辑器**：十六进制视图、反汇编（LIEF + Capstone）、区段/符号/导入/字符串分析、偏移跳转定位
- **数据视图**：SQLite/CSV 文件浏览与 SQL 查询，JSON 美化
- **RockYou 密码库**：一键下载并导入 1400 万条口令数据集
- **主题系统**：Default（亮色）/ Nezha Cyber（暗色）/ GitHub Light / Xcode 四套主题，统一设计 Token 体系
- **三语界面**：中文 / English / Deutsch

## 技术栈

- Qt 6（Widgets / Network / Svg）
- C++20 · CMake
- SQLite3 · tl-expected · LIEF · Capstone

## 构建

依赖：Qt 6 开发库、CMake ≥ 4.3、SQLite3、LIEF、Capstone。

```bash
cmake -B cmake-build -S .
cmake --build cmake-build --target NezhaIDE -j 8
```

macOS 下构建产物为 bundle：

```bash
./cmake-build/NezhaIDE.app/Contents/MacOS/NezhaIDE
```

### 自测

```bash
NEZHA_SELFTEST=1 NEZHA_SELFTEST_URL="https://example.com" \
  ./cmake-build/NezhaIDE.app/Contents/MacOS/NezhaIDE
```

截图输出至 `/tmp/nezha_*.png`；二进制分析自测用 `NEZHA_HEX_SELFTEST=1`。

## 项目结构

```
src/       核心逻辑（配置、服务、模型、工具、仓库）
  services/   主题系统、本地化、数据库
  tools/      Hydra 集成
views/     界面组件（编辑器、终端、Git、HTTP、Hex、活动栏、底部面板）
resources/ 主题与本地化定义（XML）
data/      运行时数据库
```

## 版权

Copyright © 2026 钟智强（哪吒网络安全）· 保留所有权利。

本软件仅用于授权的安全测试、逆向研究与教学用途。请遵守所在地法律法规，未经授权不得对未获许可的目标使用。
