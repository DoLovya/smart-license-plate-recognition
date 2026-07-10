# 文档内容校验清单

> 用途：任何文档改动后，按本清单逐条核对。每条都应对应一个可验证的事实或文件位置。

## 1. 项目入口

- [ ] 根 `README.md` 的“快速开始”可直接 `bash deploy/scripts/start_all.sh` 启动后端 + Qt 客户端。
- [ ] `README.md` 中的技术栈与 `backend/requirements.txt`、`algorithms/requirements.txt`、`frontend/qt_client/CMakeLists.txt` 一致。
- [ ] 仓库结构图与实际目录一致（无 `pipelines/`、`inference/`、`checkpoints/`、`sqlite/` 等空目录被误列）。

## 2. 架构

- [ ] `docs/architecture.md` 模块职责与代码一致。
- [ ] 字段命名：算法结果以 `image_id` 关联，不再出现 `source` 作为结果关联键。

## 3. 开发环境

- [ ] `docs/development.md` 中 Python、Qt、CMake 版本与实际工具链一致。
- [ ] `pytest` 与 `ctest` 命令在当前环境可执行。

## 4. 部署

- [ ] `docs/deployment.md` 中：
  - `start_all.sh` 描述与 `deploy/scripts/start_all.sh` 实现一致。
  - Docker 命令与 `deploy/docker/docker-compose.yml` 一致。
  - 环境变量表与 `deploy/configs/.env.example` 一致。

## 5. API

- [ ] `docs/api-overview.md` 中路由与 `backend/app/api/routes.py` 一致。
- [ ] `RecognitionResponse` 字段与 `backend/app/schemas/recognition.py` 一致。
- [ ] 占位实现说明与 `backend/app/services/recognition_service.py` 一致。

## 6. 算法

- [ ] `algorithms/README.md` 描述的接口与 `algorithms/detector/license_plate_detector.py`、`algorithms/recognizer/license_plate_recognizer.py` 一致。
- [ ] 训练参数与 `algorithms/training/train.py` 一致。
- [ ] `algorithms/configs/model_config.yaml` 字段说明准确。

## 7. 数据

- [ ] `data/README.md` 目录与 `data/` 实际子目录一致。
- [ ] Ultralytics 数据布局说明与 `tools/convert_ccpd_to_yolo.py` 输出一致。

## 8. Qt 客户端

- [ ] `frontend/qt_client/TEST_REPORT.md` 用例与 `tests/tst_MainWindow.cpp` 一致。
- [ ] 性能口径与代码节流（`200ms`）、采集周期（`40ms`）一致。

## 9. 文档一致性

- [ ] `docs/CHANGELOG.md` 最近一次更新覆盖本次改动。
- [ ] `docs/DOC_REFACTOR_REPORT.md` 中的修改项与实际文件 diff 一致。
- [ ] 所有外链（`/docs/`、`/api/v1/...`）可在本地启动后访问。
