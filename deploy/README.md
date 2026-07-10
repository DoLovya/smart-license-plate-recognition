# 部署模块说明

## 1. 目录结构

| 路径 | 作用 |
| --- | --- |
| `deploy/configs/.env.example` | 后端环境变量模板 |
| `deploy/docker/Dockerfile.backend` | 后端镜像 |
| `deploy/docker/docker-compose.yml` | 一键启动服务 |
| `deploy/scripts/start_backend.sh` | 启动后端 |
| `deploy/scripts/start_all.sh` | 一键启动后端 + Qt 客户端 |
| `deploy/scripts/run_algorithm_pipeline.sh` | 算法推理脚本 |
| `deploy/systemd/license-plate-backend.service` | systemd 单元模板 |

## 2. 常用脚本

```bash
# 启动后端
bash deploy/scripts/start_backend.sh

# 一键启动后端 + Qt
bash deploy/scripts/start_all.sh

# 单图算法推理
bash deploy/scripts/run_algorithm_pipeline.sh data/raw/demo.jpg
```

## 3. 环境变量

见 `deploy/configs/.env.example`。生产部署时复制为 `.env` 并按需修改。

## 4. Docker

```bash
cp deploy/configs/.env.example deploy/configs/.env
docker compose -f deploy/docker/docker-compose.yml up --build -d
```

## 5. systemd

替换 `deploy/systemd/license-plate-backend.service` 中的：

- `WorkingDirectory`
- `ExecStart`（虚拟环境下的 `uvicorn` 路径）
- `EnvironmentFile`

执行 `systemctl daemon-reload && systemctl enable --now license-plate-backend` 即可。
