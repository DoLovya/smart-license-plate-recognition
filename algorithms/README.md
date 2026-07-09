# 算法模块说明

该目录用于管理车牌检测、车牌字符识别、推理流水线和训练入口。

## 子模块

- `configs/`：模型路径、阈值和运行参数。
- `detector/`：车牌检测模型封装。
- `recognizer/`：字符识别模型封装。
- `pipelines/`：将检测与识别组合为统一业务流水线。
- `inference/`：命令行推理入口。
- `training/`：训练或微调入口。
- `weights/`：权重文件目录。通用本地权重默认不纳入版本管理，私有模型通过 `weights/plate-models` 子仓库接入。
- `checkpoints/`：训练中间产物目录，不纳入版本管理。

## 模型子仓库

- 子仓库路径：`algorithms/weights/plate-models`
- 远端仓库：`DoLovya/smart-license-plate-recognition-models`（私有）
- 初始化命令：`git submodule update --init --recursive`
- 版本策略：主仓库固定子仓库提交，模型仓库内部采用 `vX.Y.Z` 目录管理稳定发布件
