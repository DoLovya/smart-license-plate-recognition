# Qt 客户端功能验证记录

## 1. 验证范围

- 控件初始化与检测启停
- 识别结果面板与历史表刷新
- CSV 导出落盘
- 后端 HTTP 接口预留（Mock 模式可独立运行）
- CMake + ctest 单元测试

## 2. 单元测试项

| 用例 | 用途 |
| --- | --- |
| `shouldInitializeCoreControls` | 验证关键控件已创建 |
| `shouldToggleDetectionControls` | 验证启停按钮状态切换 |
| `shouldUpdateRecognitionPanelAfterSubmittingFrame` | 验证提交帧后异步刷新结果面板 |
| `shouldExportRecognitionRecords` | 验证 CSV 导出 |

## 3. 运行方式

```bash
cmake -S frontend/qt_client -B frontend/qt_client/build
cmake --build frontend/qt_client/build
ctest --test-dir frontend/qt_client/build --output-on-failure
```

## 4. 性能口径

| 指标 | 目标 |
| --- | --- |
| 视频流渲染 | ≥ 25 fps |
| UI 操作响应 | ≤ 100 ms |
| 检测帧推送节流 | 200 ms |

## 5. 联调说明

- 未连接后端时默认走 Mock 模式，界面可联调。
- 真实摄像头 / 真实 HTTP 字段需在目标环境回归。
- Qt 5.15 与 Qt 6 双链路支持，CMake 自动选择。
