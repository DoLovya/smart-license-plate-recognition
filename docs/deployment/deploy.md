# 部署说明

## Docker 部署

```bash
docker compose -f deploy/docker/docker-compose.yml up --build -d
```

## 脚本部署

```bash
bash deploy/scripts/start_backend.sh
```

## 服务化部署

- 参考 `deploy/systemd/license-plate-backend.service`
- 替换工作目录、虚拟环境路径和环境变量文件路径
