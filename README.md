# 智能车牌检测识别系统

桌面端 + 后端 + 算法三层架构的车牌检测与字符识别系统。Qt Widgets 客户端负责图像与结果展示，FastAPI 后端负责服务编排，Python 算法层封装 YOLOv8 检测器与 PaddleOCR 识别器。

## 1. 功能特性

- 单张图片导入与结果展示（车牌文本、置信度、图像 ID、抓拍时间）。
- 摄像头/视频流采集、缩略图绘制与结果回写。
- 检测 + 识别结果 CSV 导出。
- 一键脚本串联后端启动与 Qt 客户端构建运行。
- 算法层支持替换检测与识别模型，配置集中在 `algorithms/configs/model_config.yaml`。

## 2. 技术栈

- 客户端：Qt 5.15 / Qt 6（Widgets、Network、Multimedia 可选），CMake 3.16+，C++17。
- 后端：FastAPI、Uvicorn、Pydantic v2、pydantic-settings。
- 算法：Ultralytics YOLOv8、PaddleOCR、OpenCV、ONNX Runtime、PyTorch。
- 部署：Docker Compose、systemd、bash 脚本。

## 3. 仓库结构

```text
smart-license-plate-recognition/
├── frontend/qt_client/         # Qt 桌面端
├── backend/                    # FastAPI 后端
├── algorithms/                 # 检测、识别、训练、推理
├── data/                       # 原始与处理后数据
├── deploy/                     # Docker / 脚本 / systemd
├── docs/                       # 架构、开发、部署、API、变更、报告
├── tests/                      # 后端、算法、集成测试
└── tools/                      # 数据准备等辅助脚本
```

各模块独立职责见 [docs/architecture.md](docs/architecture.md)。

## 4. 环境要求

- Python 3.10+
- Qt 5.15 或 Qt 6.5+；CMake 3.16+
- Docker 24+（可选，用于容器化部署）
- NVIDIA 驱动 + CUDA（可选，用于 GPU 训练与推理）

## 5. 快速开始

```bash
# 1. 克隆并初始化子仓库（私有模型）
git submodule update --init --recursive

# 2. 一键启动后端 + Qt 客户端
bash deploy/scripts/start_all.sh
```

如需手动分步执行，可参考 [docs/development.md](docs/development.md) 与 [docs/deployment.md](docs/deployment.md)。

## 6. 模型版本

- 模型子仓库：`algorithms/weights/plate-models`（远端 `DoLovya/smart-license-plate-recognition-models`，私有）。
- 当前检测发布件：`detector/yolo/ccpd-green/v1.0.0/best.pt`。
- 当前识别发布件：`recognizer/paddleocr/pp-ocrv6-small/v1.0.0/`。
- 详细策略见 `algorithms/weights/plate-models/VERSIONING.md`。

## 7. 文档索引

| 主题 | 路径 |
| --- | --- |
| 架构 | [docs/architecture.md](docs/architecture.md) |
| 开发环境 | [docs/development.md](docs/development.md) |
| 部署 | [docs/deployment.md](docs/deployment.md) |
| API | [docs/api-overview.md](docs/api-overview.md) |
| 参考文献 | [docs/references.md](docs/references.md) |
| 内容校验 | [docs/CHECKLIST.md](docs/CHECKLIST.md) |
| 文档变更 | [docs/CHANGELOG.md](docs/CHANGELOG.md) |
| 文档重构报告 | [docs/DOC_REFACTOR_REPORT.md](docs/DOC_REFACTOR_REPORT.md) |

## 8. 版本与更新

- 项目版本：0.1.0
- 文档主入口：本文档
- 文档最近一次重构：见 [docs/CHANGELOG.md](docs/CHANGELOG.md)
