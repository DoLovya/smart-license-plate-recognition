# 文档变更记录

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
