# 训练脚本说明

## 1. 启动训练

```bash
python algorithms/training/train.py
```

默认参数：

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `--data` | `data/processed/ccpd_green_yolo/dataset.yaml` | 数据集配置 |
| `--model` | `yolov8n.pt` | Ultralytics 模型名或权重路径 |
| `--epochs` | 300 | 训练轮次 |
| `--batch` | 4 | 批大小 |
| `--workers` | 4 | 数据加载线程数 |
| `--imgsz` | 640 | 图像尺寸 |
| `--device` | `auto` | `auto` / `cpu` / `0` / `0,1` |
| `--project` | `algorithms/training/runs` | 输出根目录 |
| `--name` | `train` | 运行名 |
| `--amp` | 开启 | 混合精度，`--no-amp` 关闭 |

输出位置由 Ultralytics 根据 `--project` 与 `--name` 决定。

## 2. 常用示例

```bash
# 指定数据集与模型
python algorithms/training/train.py --data data/processed/ccpd_green_yolo/dataset.yaml --model yolov8s.pt

# 调整训练规模
python algorithms/training/train.py --epochs 100 --batch 8 --imgsz 640

# 指定设备
python algorithms/training/train.py --device cpu
python algorithms/training/train.py --device 0
```

## 3. 结果绘图

```bash
# 查看可用列
python algorithms/training/plot_results.py --csv algorithms/training/runs/detect/runs/train-2/results.csv --list-columns

# 绘制指定曲线
python algorithms/training/plot_results.py --csv algorithms/training/runs/detect/runs/train-2/results.csv --columns train/box_loss train/cls_loss val/cls_loss
```

不传 `--columns` 时默认绘制：box/cls/dfl 损失，precision、recall、mAP50、mAP50-95。

## 4. 运行前准备

1. 准备 `dataset.yaml`，结构必须为 `images/{train,val,test}` + `labels/{train,val,test}`。
2. 安装训练依赖：`pip install ultralytics torch`。
3. 使用 GPU 时确认 PyTorch 与 CUDA 匹配。

> 训练脚本会打印 PyTorch / CUDA / 设备信息。
