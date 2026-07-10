# 开发环境说明

## 1. 基础环境

| 依赖 | 版本 |
| --- | --- |
| Python | 3.10+ |
| pip | 23+ |
| Qt | 5.15 或 6.5+ |
| CMake | 3.16+ |
| OpenCV | 4.10+（Python 端） |

## 2. 后端

```bash
cd backend
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
uvicorn app.main:app --host 0.0.0.0 --port 8000
```

入口文件：`backend/app/main.py`。环境变量模板：`deploy/configs/.env.example`。

## 3. 算法层

```bash
cd algorithms
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

主要依赖：Ultralytics、PaddleOCR、PyTorch、OpenCV、ONNX Runtime。

## 4. Qt 客户端

```bash
cd frontend/qt_client
cmake -S . -B build
cmake --build build
./build/SmartLicensePlateQtClient
```

CMake 会优先查找 Qt5（5.15），找不到时回退到 Qt6。Qt 客户端当前仅依赖 `Widgets`、`Network`、`Test`，用于静态图片检测流程。

## 5. 模型子仓库

```bash
git submodule update --init --recursive
```

子仓库路径：`algorithms/weights/plate-models`。部署环境必须固定主仓库 commit，不得让子仓库分支漂移。

## 6. 测试

```bash
pytest                              # Python 测试（tests/）
ctest --test-dir frontend/qt_client/build --output-on-failure   # Qt 测试
```

## 7. 常见问题

- **导入 `algorithms` 失败**：从项目根目录运行脚本，或设置 `PYTHONPATH=.`。
- **找不到识别模型目录**：确认 `algorithms/weights/plate-models/recognizer/paddleocr/pp-ocrv6-small/v1.0.0/` 存在。
- **训练报 “No labels found”**：检查 `dataset.yaml` 的 `images/` 与 `labels/` 目录结构是否完全对应。
