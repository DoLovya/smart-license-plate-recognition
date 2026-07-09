# 训练脚本使用说明

`train.py` 已改造成命令行参数程序，训练结果默认输出到当前目录下的 `runs/`，也就是：

```text
d:\Code.Personal\smart-license-plate-recognition\algorithms\training\runs
```

这样无论你从项目根目录还是其他目录启动脚本，结果都会稳定落在 `algorithms/training/runs` 下。

## 默认运行

在项目根目录执行：

```bash
python algorithms/training/train.py
```

默认参数：

- `--data`：`data/processed/ccpd_green_yolo/dataset.yaml`
- `--model`：`yolov8n.pt`
- `--epochs`：`300`
- `--batch`：`4`
- `--workers`：`4`
- `--imgsz`：`640`
- `--device`：`auto`
- `--project`：`algorithms/training/runs`
- `--name`：`train`
- `--amp`：开启

## 查看帮助

```bash
python algorithms/training/train.py --help
```

查看绘图脚本帮助：

```bash
python algorithms/training/plot_results.py --help
```

## 常用示例

使用默认配置训练：

```bash
python algorithms/training/train.py
```

指定数据集配置和模型：

```bash
python algorithms/training/train.py --data data/processed/ccpd_green_yolo/dataset.yaml --model yolov8s.pt
```

调整训练轮次、批大小和图像尺寸：

```bash
python algorithms/training/train.py --epochs 100 --batch 8 --imgsz 640
```

指定设备：

```bash
python algorithms/training/train.py --device auto
python algorithms/training/train.py --device cpu
python algorithms/training/train.py --device 0
```

自定义输出目录和运行名：

```bash
python algorithms/training/train.py --project algorithms/training/runs --name train-exp-01
```

允许复用已有运行目录：

```bash
python algorithms/training/train.py --name train --exist-ok
```

关闭混合精度：

```bash
python algorithms/training/train.py --no-amp
```

## 参数说明

- `--data`：`dataset.yaml` 路径
- `--model`：Ultralytics 模型名或本地权重路径
- `--epochs`：训练轮次
- `--batch`：批大小
- `--workers`：数据加载线程数
- `--imgsz`：输入图像尺寸
- `--device`：训练设备，支持 `auto`、`cpu`、`0`、`0,1`
- `--project`：训练结果根目录
- `--name`：当前训练任务名称
- `--exist-ok`：已有同名目录时允许继续写入
- `--amp` / `--no-amp`：开启或关闭混合精度

## 输出目录示例

默认情况下，训练结果会类似生成到：

```text
algorithms/training/runs/
  train/
  train2/
  train-exp-01/
```

具体目录命名由 Ultralytics 根据 `--project` 和 `--name` 决定。

## 运行前准备

1. 先准备好数据集配置文件：

```text
data/processed/ccpd_green_yolo/dataset.yaml
```

2. 确保已安装训练依赖，例如：

```bash
pip install ultralytics torch
```

3. 如果使用 GPU，确认 PyTorch 与 CUDA 环境可用。

## 说明

- 脚本会在训练开始前打印当前 PyTorch、CUDA 和设备信息。
- 当 `--device auto` 时：
  - 检测到 CUDA 则自动使用第一个 GPU
  - 否则回退到 CPU

## 基于 results.csv 绘图

训练完成后，Ultralytics 会生成 `results.csv`。你可以用 `plot_results.py` 按列名绘制训练曲线。

例如，针对下面这个文件：

```text
algorithms/training/runs/detect/runs/train-2/results.csv
```

先查看有哪些可用列：

```bash
python algorithms/training/plot_results.py --csv algorithms/training/runs/detect/runs/train-2/results.csv --list-columns
```

绘制你关心的几条曲线：

```bash
python algorithms/training/plot_results.py --csv algorithms/training/runs/detect/runs/train-2/results.csv --columns train/box_loss train/cls_loss val/cls_loss
```

上面的命令会默认把图片输出到：

```text
algorithms/training/runs/detect/runs/train-2/plots/
```

并生成：

- `train_box_loss.png`
- `train_cls_loss.png`
- `val_cls_loss.png`
- `combined_metrics.png`

也可以自定义输出目录：

```bash
python algorithms/training/plot_results.py --csv algorithms/training/runs/detect/runs/train-2/results.csv --columns train/box_loss train/cls_loss val/cls_loss --output-dir algorithms/training/runs/detect/runs/train-2/custom-plots
```

如果你想一次画常用指标，不传 `--columns` 即可，脚本会默认绘制：

- `train/box_loss`
- `train/cls_loss`
- `train/dfl_loss`
- `val/box_loss`
- `val/cls_loss`
- `val/dfl_loss`
- `metrics/precision(B)`
- `metrics/recall(B)`
- `metrics/mAP50(B)`
- `metrics/mAP50-95(B)`
