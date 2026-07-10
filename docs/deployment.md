# 部署说明

## 1. 一键启动（推荐）

```bash
bash deploy/scripts/start_all.sh
```

脚本会自动：

1. 检查 `http://127.0.0.1:8000/api/v1/health`，未启动则调用 `start_backend.sh`。
2. 配置并编译 `frontend/qt_client`。
3. 前台启动 `SmartLicensePlateQtClient`，退出时清理本次拉起的后端进程。

## 2. 仅后端

```bash
bash deploy/scripts/start_backend.sh
```

或手动：

```bash
cd backend && source .venv/bin/activate
uvicorn app.main:app --host 0.0.0.0 --port 8000
```

## 3. Docker

```bash
cp deploy/configs/.env.example deploy/configs/.env
docker compose -f deploy/docker/docker-compose.yml up --build -d
```

镜像默认监听 `8000` 端口，挂载 `algorithms/` 与 `data/` 到容器内。

## 4. systemd

模板：`deploy/systemd/license-plate-backend.service`。

部署时需替换：

- `WorkingDirectory`：实际后端目录
- `ExecStart`：实际虚拟环境下的 `uvicorn` 路径
- `EnvironmentFile`：实际 `.env` 路径

## 5. 环境变量

| 变量 | 含义 | 默认值 |
| --- | --- | --- |
| `APP_NAME` | 服务显示名 | `Smart License Plate Recognition Backend` |
| `APP_VERSION` | 服务版本 | `0.1.0` |
| `API_PREFIX` | API 前缀 | `/api/v1` |
| `HOST` | 监听地址 | `0.0.0.0` |
| `PORT` | 监听端口 | `8000` |
| `MODEL_CONFIG_PATH` | 模型配置 YAML | `../algorithms/configs/model_config.yaml` |
| `EXPORT_DIR` | 结果导出目录 | `../data/exports` |
| `SQLITE_URL` | SQLite 连接串 | `sqlite:///../data/sqlite/license_plate.db` |
