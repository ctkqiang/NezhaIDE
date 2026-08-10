# NezhaIDE — Codex 规则

## 目录结构

项目目录结构固定，所有新文件必须按现有布局放置：

```
NezhaIDE/
├── CMakeLists.txt
├── main.cc
├── src/
│   ├── model/        — 数据模型、结构体、枚举
│   ├── services/     — 业务逻辑、应用服务
│   ├── utilities/    — 共享工具（日志、指标等）
│   └── views/        — UI 组件、widget、QML
```

新增代码按架构角色放入对应子目录，禁止创建其他目录结构。

## 文件命名

- 源文件: `snake_case.cc`
- 头文件: `snake_case.h`
- 禁止 `.cpp`、禁止 `.hpp`

## 代码风格

### 命名空间
- 项目根命名空间: `NezhaIDE`
- 子命名空间对应目录结构: `NezhaIDE::Utilities`、`NezhaIDE::Services`、`NezhaIDE::Model`、`NezhaIDE::Views`
- 使用嵌套命名空间语法 (`namespace A::B { }`)，不用嵌套块

### 注释
- 禁止装饰性注释，禁止分隔符模式：
  - 禁止 `// ----------`
  - 禁止 `// ==========`
  - 禁止 `///=-----step`
  - 禁止 `// ~~~~~~~~~~`
  - 禁止任何形式的注释框或视觉分隔符
- 只写解释 WHY 的有意义注释，不写 WHAT
- 不写多段文档字符串
- 不在注释中引用当前任务、修复或调用者

### 头文件
- 顶部使用 `#pragma once`
- 辅助使用 `#ifndef`/`#define`/`#endif` 守卫

### 通用
- 使用 C++26 特性
- 优先使用 `enum class`、`std::unique_ptr`、`std::expected`、`std::string_view`
- 禁止裸拥有指针、禁止 `system()`、禁止全局可变状态
- CMakeLists.txt: 新增 `.cc` 和 `.h` 文件必须显式加入 `add_executable`

## UI 设计规范

### Apple 设计原则
- **清晰**：充足留白（8px 基础间距）、圆角（6-8px）、SF 风格无衬线字体
- **层级**：微妙阴影 (`box-shadow`) 和扁平边框 (`1px solid`) 区分深度
- **统一**：卡片式面板、padding/margin 节奏 (4/8/12/16px)

### ByteDance 设计模式
- **紧凑高效**：面板头部 32px 高、树节点行高 28px、信息密度高但不拥挤
- **功能可见**：可操作元素有 hover 态 (`rgba(0,0,0,0.04)`) 和 active 态
- **色彩克制**：主色 `#3370FF`（飞书蓝）、图标色 `#646A73`（飞书灰）、背景 `#F5F6F7`

### 组件化规则
- 每个独立 UI 模块一个 `.h` + `.cc` + `.ui` 三元组
- `.ui` 定义 Widget 树和布局，`.cc` 写交互逻辑，`.h` 暴露信号/槽接口
- Widget 类放在 `NezhaIDE::Views` 命名空间
- 页面内组件不允许直接依赖其他 View，通过信号/槽解耦

### 样式规则
- 禁止内联 `setStyleSheet` 字符串拼接
- 样式统一写入对应 `.ui` 文件的 `<styleSheet>` 标签或集中的 QSS 文件
- 颜色使用 `#RRGGBB` 格式，禁止 `rgb()` 函数

## 构建
- 构建目录: `cmake-build-debug`
- 构建命令: `cmake --build cmake-build-debug --target NezhaIDE -j 6`
- 每次修改后必须构建验证