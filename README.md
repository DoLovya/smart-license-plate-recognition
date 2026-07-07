# 智能车牌检测识别系统

智能车牌检测识别系统是一个面向后续工程化开发的多模块基础仓库，覆盖桌面端展示、Python 后端服务、深度学习算法、数据管理、部署配置和项目文档六大板块，适合作为后续迭代单图识别、批量处理、模型部署与性能优化的统一工程入口。

## 1. 项目目标

- 支持车牌检测、字符识别、结果可视化、批量处理与结果导出。
- 保持前端展示、后端编排、算法推理、数据管理和部署发布解耦。
- 采用适合团队协作的目录规范，便于后续扩展接口、模型与部署方式。

## 2. 项目结构

```text
smart-license-plate-recognition/
├── frontend/                    # 前端展示模块
│   └── qt_client/               # Qt 桌面端骨架
├── backend/                     # 后端服务模块
│   ├── app/
│   │   ├── api/                 # API 路由
│   │   ├── core/                # 配置与基础设施
│   │   ├── schemas/             # 请求/响应模型
│   │   ├── services/            # 业务服务
│   │   └── utils/               # 通用工具
│   └── requirements.txt
├── algorithms/                  # 算法模型模块
│   ├── configs/                 # 模型与推理配置
│   ├── detector/                # 车牌检测模块
│   ├── recognizer/              # 字符识别模块
│   ├── pipelines/               # 检测识别流水线
│   ├── inference/               # 推理入口
│   ├── training/                # 训练入口
│   ├── weights/                 # 模型权重目录
│   └── checkpoints/             # 训练检查点目录
├── data/                        # 数据存储模块
│   ├── raw/                     # 原始数据
│   ├── interim/                 # 临时处理中间产物
│   ├── processed/               # 处理后数据
│   ├── exports/                 # 导出结果
│   └── sqlite/                  # SQLite 初始化脚本与数据库文件
├── deploy/                      # 部署配置模块
│   ├── docker/                  # Docker 与 Compose 配置
│   ├── scripts/                 # 启停与任务脚本
│   ├── configs/                 # 环境变量模板
│   └── systemd/                 # 服务部署示例
├── docs/                        # 文档模块
│   ├── architecture/            # 架构设计文档
│   ├── development/             # 开发规范文档
│   ├── deployment/              # 部署说明文档
│   └── api/                     # 接口说明文档
├── tests/                       # 测试代码
│   ├── backend/
│   ├── algorithms/
│   └── integration/
├── tools/                       # 辅助脚本
├── .gitignore
└── 智能车牌检测识别系统-项目规划.md
```

## 3. 模块说明

### `frontend`

- 承载桌面端展示逻辑，当前提供 `Qt Widgets` 项目骨架。
- 后续可扩展图片加载、识别结果面板、参数配置、批量任务进度与日志面板。

### `backend`

- 提供统一服务入口，负责文件接收、任务编排、结果组织与状态查询。
- 当前采用 `FastAPI` 作为基础服务框架，便于后续接入桌面端、本地脚本或 HTTP 调试。

### `algorithms`

- 管理检测模型、识别模型、推理流水线、训练入口与配置文件。
- 当前骨架包含检测器、识别器、流水线和 CLI 推理入口，便于后续接入 ONNX Runtime、PyTorch 或 TensorRT。

### `data`

- 管理原始数据、处理中间产物、导出结果和 SQLite 初始化脚本。
- 约定数据与代码分层，避免算法临时产物污染源码目录。

### `deploy`

- 保存 Docker、脚本、环境变量模板与服务化部署样例。
- 当前可用于本地容器调试、服务启动和后续 Linux 服务化部署。

### `docs`

- 保存架构、开发、部署和 API 文档，确保代码结构与文档说明同步维护。

## 4. 环境依赖

### 基础依赖

- Python `3.10+`
- pip `23+`
- CMake `3.20+`
- Qt `6.5+`
- OpenCV `4.8+`

### Python 依赖

- 后端服务：`FastAPI`、`Uvicorn`、`Pydantic`、`python-multipart`
- 算法模块：`PyTorch`、`ONNX Runtime`、`NumPy`、`OpenCV`
- 测试工具：`pytest`

### 可选依赖

- Docker `24+`
- Docker Compose Plugin

## 5. 本地启动

### 5.1 启动后端服务

```bash
cd backend
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
uvicorn app.main:app --reload --host 0.0.0.0 --port 8000
```

服务启动后可访问 `http://127.0.0.1:8000/docs` 查看接口文档。

### 5.2 运行算法推理骨架

```bash
cd algorithms
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python inference/run_inference.py --image ../data/raw/demo.jpg
```

### 5.3 编译 Qt 客户端骨架

```bash
cd frontend/qt_client
cmake -S . -B build
cmake --build build
```

## 6. 部署流程

### 6.1 环境变量准备

```bash
cp deploy/configs/.env.example deploy/configs/.env
```

根据实际模型路径、端口和数据目录调整配置。

### 6.2 Docker 部署后端

```bash
docker compose -f deploy/docker/docker-compose.yml up --build -d
```

### 6.3 脚本方式启动

```bash
bash deploy/scripts/start_backend.sh
```

### 6.4 服务化部署

- 参考 `deploy/systemd/license-plate-backend.service`
- 将工作目录、Python 虚拟环境路径和环境变量文件替换为实际部署值

## 7. 后续开发建议

- 在 `algorithms/weights/` 中落地公开模型权重，统一通过配置文件加载。
- 在 `backend/app/services/` 中接入真实推理流程与批处理调度逻辑。
- 在 `frontend/qt_client/` 中实现图像画布、结果表格与日志面板。
- 在 `tests/` 中逐步补齐 API、流水线和集成测试。
