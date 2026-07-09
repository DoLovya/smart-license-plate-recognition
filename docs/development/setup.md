# 开发环境说明

## Python 环境

```bash
cd backend
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## 算法环境

```bash
cd algorithms
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Qt 客户端

- 安装 Qt 6.5+ 与 CMake 3.20+
- 进入 `frontend/qt_client/` 目录执行 CMake 构建

## 初始化子仓库

首次拉取项目后执行：

```bash
git submodule update --init --recursive
```

当前私有模型子仓库路径为 `algorithms/weights/plate-models`。
部署或协作时请固定主仓库提交，不要单独漂移子仓库分支。
