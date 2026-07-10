# 文档变更记录

## 0.4.2 / 2026-07-10 — Qt 客户端切换为纯静态图片检测

### 变更

- 删除 Qt 客户端中的 Demo / Mock 回退逻辑，`开始检测` 仅通过真实后端接口 `/api/v1/recognize` 执行静态图片检测。
- 移除摄像头枚举、视频采集线程、实时帧推送与 Qt Multimedia 依赖，客户端不再包含摄像头流程。
- 将预览控件由 `VideoDisplayWidget` 重构为 `ImagePreviewWidget`，界面文案与交互统一收敛为静态图片场景。
- 为图片导入与开始检测补充格式、尺寸、完整性校验，仅允许合法 PNG/JPG/JPEG/BMP 图片进入检测流程。
- 更新 Qt 客户端测试：新增非法图片拦截与真实 HTTP 上传验证，移除 Mock / 视频流相关测试说明。

## 0.4.1 / 2026-07-10 — Qt 客户端目录分层

### 变更

- 重构 `frontend/qt_client/src/` 与 `include/`：按职责拆分为 `app/`、`api/`、`services/`、`widgets/`、`workers/`、`core/`，替代原有平铺结构。
- 更新 `frontend/qt_client/CMakeLists.txt`：按模块分组维护源文件列表，测试目标复用同一套分层清单。
- 同步修正 Qt 客户端源码与测试中的头文件引用路径，保持目录结构与依赖关系一致。
- 验证通过：`cmake -S frontend/qt_client -B frontend/qt_client/build`、`cmake --build frontend/qt_client/build`、`ctest --test-dir frontend/qt_client/build --output-on-failure`。

## 0.4.0 / 2026-07-10 — docs 目录扁平化

### 变更

- 合并 5 个单文件子目录（`api/`、`architecture/`、`deployment/`、`development/`、`references/`）到 `docs/` 根目录。
- 文件名规整：`api/overview.md` → `api-overview.md`，`architecture/project-structure.md` → `architecture.md`，`deployment/deploy.md` → `deployment.md`，`development/setup.md` → `development.md`，`references/references.md` → `references.md`。
- 同步更新 `README.md`、`CHECKLIST.md`、`CHANGELOG.md`、`DOC_REFACTOR_REPORT.md` 中的所有引用路径。
- 新增本节，更新 [DOC_REFACTOR_REPORT.md](DOC_REFACTOR_REPORT.md) 中“目录迁移”小节。

## 0.3.0 / 2026-07-10 — 文档系统性重构

### 变更

- 重写根 `README.md`：聚焦功能特性、技术栈、快速开始、模型版本与文档索引。
- 重写 `docs/architecture.md`：用表格+调用关系图替代纯文字段落。
- 重写 `docs/development.md`：补全依赖表、测试命令、常见问题。
- 重写 `docs/deployment.md`：补全 Docker、systemd、环境变量说明。
- 重写 `docs/api-overview.md`：与 `backend/app/api/routes.py`、`schemas/recognition.py` 字段对齐。
- 重写 `docs/references.md`：精简为单表 + 补充约定。
- 重写 `algorithms/README.md`：与算法层实现对齐（`image_id`、`raw` 字段、PP-OCRv6_small_rec）。
- 重写 `algorithms/training/README.md`：补全默认参数表、绘图命令、运行前准备。
- 重写 `data/README.md`：补全目录与是否入版本、数据流转图、训练数据布局。
- 新增 `deploy/README.md`：替代 README 中重复的部署段落。
- 新增 `frontend/qt_client/TEST_REPORT.md`（保持原位并重写）。
- 新增 `docs/CHECKLIST.md`：内容校验清单。
- 新增 `docs/CHANGELOG.md`：本文档。
- 新增 `docs/DOC_REFACTOR_REPORT.md`：本次重构对比报告。

### 删除

- 旧 `智能车牌检测识别系统-项目规划.md` 中“项目价值 / 简历接入 / 风险与应对”等与代码无关的章节已沉入项目记忆，本次不再维护该文件。
- 旧 `algorithms/training/README.md` 中 Windows 路径 `d:\Code.Personal\...` 与 `algorithms/training/runs/detect/runs/train-2/...` 硬编码示例已替换为相对路径。
- 旧 `docs/api-overview.md` 中虚构的“批量识别 / 历史结果查询”接口段落已删除。

## 0.2.x — 历史

由早期 PR 维护，无对外文档变更记录。
