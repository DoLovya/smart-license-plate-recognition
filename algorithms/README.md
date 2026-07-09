# 算法模块说明

该目录用于管理车牌检测、车牌字符识别、训练脚本，以及后续推理流水线相关代码。

## 当前目录结构

- `configs/`：模型路径、阈值和运行参数配置。
- `detector/`：车牌检测模型封装，当前包含：
  - `LicensePlateDetector`：YOLO 检测器封装。
  - `DetectionBox` / `DetectionResult`：标准化检测结果数据类。
- `recognizer/`：车牌字符识别模型封装，当前包含：
  - `LicensePlateRecognizer`：基于 PaddleOCR `TextRecognition` 的识别封装。
  - `RecognitionResult`：标准化识别结果数据类。
- `training/`：训练、结果绘图和训练说明文档。
- `weights/`：权重文件目录。通用本地权重默认不纳入版本管理，私有模型通过 `weights/plate-models` 子仓库接入。
- `requirements.txt`：算法侧依赖清单。

## 规划中的目录

- `pipelines/`：将检测与识别组合为统一业务流水线。
- `inference/`：命令行推理入口。
- `checkpoints/`：训练中间产物目录，不纳入版本管理。

## 模型子仓库

- 子仓库路径：`algorithms/weights/plate-models`
- 远端仓库：`DoLovya/smart-license-plate-recognition-models`（私有）
- 初始化命令：`git submodule update --init --recursive`
- 版本策略：主仓库固定子仓库提交，模型仓库内部采用 `vX.Y.Z` 目录管理稳定发布件

## 当前实现说明

- 检测模块与识别模块保持分层，避免把算法实现直接耦合进业务流程。
- 检测输出使用 `DetectionResult` 统一结构；识别输出使用 `RecognitionResult` 统一结构。
- 当前识别模块仅封装 OCR 能力，不包含车牌裁剪、检测串联或完整流水线编排逻辑。
