# 算法模块说明

`algorithms/` 封装车牌检测、字符识别、训练与推理配置。

## 1. 目录结构

| 目录 | 作用 |
| --- | --- |
| `configs/` | 模型路径与运行参数（YAML） |
| `detector/` | YOLOv8 检测封装与结果数据类 |
| `recognizer/` | PaddleOCR 识别封装与结果数据类 |
| `training/` | YOLOv8 训练脚本与结果绘图 |
| `weights/` | 模型权重（含 `plate-models` 子仓库） |

## 2. 核心组件

- `LicensePlateDetector`：基于 Ultralytics YOLOv8。接口：`detect(image, image_id=None)`。
- `LicensePlateRecognizer`：基于 PaddleOCR PP-OCRv6_small_rec。接口：`recognize(image, image_id=None)`。
- `DetectionResult` / `DetectionBox`：检测层结果数据类，字段 `image_id`、`boxes`。
- `RecognitionResult`：识别层结果数据类，字段 `image_id`、`text`、`confidence`、`raw`。

结果统一以 `image_id` 关联输入图像，不依赖路径。

## 3. 配置

`algorithms/configs/model_config.yaml` 定义：

- 检测后端（默认 `onnxruntime`）、权重路径、置信度与 NMS 阈值。
- 识别后端（`paddleocr`）、模型目录、输入尺寸。

切换模型只需修改该文件，无需改动代码。

## 4. 训练

```bash
python algorithms/training/train.py --data data/processed/ccpd_green_yolo/dataset.yaml
```

详细参数与示例见 [algorithms/training/README.md](training/README.md)。

## 5. 模型子仓库

- 路径：`algorithms/weights/plate-models`
- 远端：`DoLovya/smart-license-plate-recognition-models`（私有）
- 初始化：`git submodule update --init --recursive`
- 版本策略：`algorithms/weights/plate-models/VERSIONING.md`
