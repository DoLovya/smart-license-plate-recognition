# 接口说明

后端基于 FastAPI，OpenAPI 文档：`http://127.0.0.1:8000/docs`。

## 1. 路由概览

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| GET | `/` | 服务根，返回运行信息 |
| GET | `/api/v1/health` | 健康检查 |
| POST | `/api/v1/recognize` | 上传图片，触发识别 |

## 2. 健康检查

```http
GET /api/v1/health
```

响应：

```json
{ "status": "ok", "service": "backend" }
```

## 3. 识别接口

```http
POST /api/v1/recognize
Content-Type: multipart/form-data

file=<图片文件>
```

响应 `RecognitionResponse`：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `image_id` | string | 图像 ID，未提供时使用上传文件名 |
| `plate_text` | string | 车牌文本，占位返回 `PENDING` |
| `confidence` | float | 车牌置信度 |
| `boxes` | list[DetectionBox] | 检测框列表 |
| `elapsed_ms` | float | 处理耗时（毫秒） |

`DetectionBox` 字段：`x1, y1, x2, y2, confidence`。

> 当前 `recognition_service.recognize` 为占位实现，尚未接入真实检测 + 识别流水线。

## 4. 错误与状态

- 后端未启动：客户端走 Mock 模式，仍可完成界面联调。
- 字段对齐：与 Qt 客户端的 `RecognitionRecord.imageId` 字段对应。
