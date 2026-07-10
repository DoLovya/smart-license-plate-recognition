# 数据模块说明

## 1. 目录约定

| 目录 | 用途 | 是否入版本 |
| --- | --- | --- |
| `raw/` | 原始图片与采集数据 | 否 |
| `interim/` | 清洗、增强等中间产物 | 否 |
| `processed/` | 训练 / 评估前的标准化数据 | 选择性 |
| `exports/` | 识别结果导出（CSV / JSON） | 否 |
| `sqlite/` | SQLite 脚本与数据库文件 | 否 |

## 2. 数据流转

```text
raw/  ── tools/convert_ccpd_to_yolo.py ──▶  processed/  ── train.py ──▶ weights/
                                                              │
                                                              └─▶ exports/
```

## 3. 训练数据布局

Ultralytics 要求：

```text
processed/<dataset>/
├── dataset.yaml
├── images/{train,val,test}/
└── labels/{train,val,test}/
```

`dataset.yaml` 建议使用绝对路径，并通过 `Path(__file__)` 解析根目录，避免跨平台路径不一致。
