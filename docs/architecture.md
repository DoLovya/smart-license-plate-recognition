# 项目结构说明

仓库按职责划分三大主模块加四个支撑模块，依赖单向流动：`frontend` 与 `backend` 调用 `algorithms`，`data` / `deploy` / `docs` / `tools` 为横向支撑。

## 模块概览

| 目录 | 职责 | 主要产物 |
| --- | --- | --- |
| `frontend/qt_client/` | 桌面端图像展示、结果面板、CSV 导出 | `SmartLicensePlateQtClient`、单元测试 |
| `backend/` | HTTP 服务、请求/响应模型、配置加载 | FastAPI 应用、OpenAPI 文档 |
| `algorithms/` | 检测与识别模型封装、训练脚本、推理配置 | `LicensePlateDetector` / `LicensePlateRecognizer`、模型权重 |
| `data/` | 原始与处理后数据、导出结果 | 训练数据集、CSV / JSON 导出 |
| `deploy/` | Docker、脚本、systemd、环境变量模板 | `docker-compose.yml`、启停脚本 |
| `docs/` | 架构、开发、部署、API、变更与重构报告 | 本目录 |
| `tests/` | 后端、算法、集成测试 | pytest 用例 |
| `tools/` | 数据转换、辅助脚本 | CCPD 转换等 |

## 调用关系

```text
Qt 客户端 ──HTTP──▶ FastAPI 后端 ──调用──▶ algorithms/ 检测 + 识别
   │                                              │
   └──直接读取──▶ data/ 中的图片/导出             └──加载 weights/ 中的模型
```

- Qt 客户端仅支持静态图片检测，必须通过 HTTP 调用后端接口执行识别。
- 后端通过 `app.services.recognition_service` 调用算法层（占位实现，详见 [api-overview.md](api-overview.md)）。
- 模型以 git submodule 形式接入，固定子仓库提交。

## 关键约束

- 路径以 `Path(__file__)` 解析，跨平台行为一致。
- 算法层不耦合业务字段，识别与检测结果均通过 `image_id` 关联。
- 训练数据按 `images/{train,val,test}` + `labels/{train,val,test}` 组织。
- 通用模型权重默认不入库，私有模型通过子仓库管理。
