# Qt 客户端功能验证记录

## 1. 验证范围

- 静态图片导入与合法性校验
- “开始检测”真实触发后端 HTTP 检测
- 识别结果面板与历史表刷新
- CSV 导出落盘
- CMake + ctest 单元测试

## 2. 单元测试项

| 用例 | 用途 |
| --- | --- |
| `shouldInitializeCoreControls` | 验证关键控件已创建 |
| `shouldRejectInvalidImportedImage` | 验证非法文件不会启用检测流程 |
| `shouldTriggerBackendRecognitionAfterLoadingImage` | 验证导入合法图片后真实发起 `/api/v1/recognize` 请求并刷新结果面板 |
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
| UI 操作响应 | ≤ 100 ms |
| 单张图片校验 | ≤ 100 ms |
| 检测请求触发 | 单次点击触发一次 HTTP 上传 |

## 5. 联调说明

- 客户端已移除 Demo / Mock / 摄像头路径，仅支持静态图片检测。
- 本地测试通过 `QTcpServer` 模拟后端，验证前端实际执行 multipart 图片上传。
- 真实环境需保证后端 `/api/v1/recognize` 可访问并接受字段名为 `file` 的图片上传。
