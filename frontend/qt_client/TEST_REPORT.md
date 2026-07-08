# Qt 客户端界面功能测试报告

## 1. 测试范围

- 实时视频流显示区域
- 图像上传入口
- 摄像头设备枚举与切换
- 检测开始 / 停止控制
- 识别结果面板更新
- 识别结果导出
- 算法服务通信接口预留
- Qt 单元测试目标构建

## 2. 核心实现说明

- 主界面采用 `QWidget + .ui` 的布局方式开发。
- 视频流采集采用 `QThread + VideoStreamWorker` 分离，目标采集周期为 `40ms`，对应理论帧率 `25fps`。
- 摄像头设备自动枚举：
  - `Qt 5.15` 使用 `QCameraInfo`
  - `Qt 6` 使用 `QMediaDevices`
- 后端服务通信采用 `AlgorithmServiceClient`，默认启用 `Mock` 模式，同时预留 HTTP 接口。
- 识别结果导出采用 CSV 文件落盘。

## 3. 单元测试项

- `shouldInitializeCoreControls`
  - 验证上传按钮、开始按钮、停止按钮、摄像头下拉框、结果表格均已初始化
- `shouldToggleDetectionControls`
  - 验证开始 / 停止按钮状态切换与检测运行标志
- `shouldUpdateRecognitionPanelAfterSubmittingFrame`
  - 验证提交检测帧后识别结果面板可以被异步刷新
- `shouldExportRecognitionRecords`
  - 验证结果导出 CSV 文件成功落盘

## 4. 性能验证口径

- 视频流渲染目标：`>= 25fps`
- UI 操作响应目标：`<= 100ms`
- 当前代码通过以下方式保障指标：
  - 视频采集与 UI 渲染线程隔离
  - 检测帧推送节流为 `200ms` 一次，避免后端通信阻塞界面
  - 图像显示与结果刷新均基于 Qt 事件循环异步派发

## 5. 验证说明

- 本次提交已补齐完整 Qt 界面源文件、`.ui` 文件、资源配置文件、单元测试文件。
- 已在当前环境使用 `CMake + ctest` 完成一次构建与测试验证。
- 当前环境 Qt 版本：`Qt 6.9.3`
- 当前环境构建结果：`SmartLicensePlateQtClient` 与 `SmartLicensePlateQtClientTests` 均成功编译。
- 当前环境测试结果：`1/1` 测试任务通过，执行时间约 `1.44s`。
- 如果算法服务未启动，客户端默认走 `Mock Ready` 模式，仍可完成界面联调与交互测试。

## 6. 待实机确认项

- 真实摄像头采集链路需在具备摄像头设备权限的环境下做联调确认。
- 与后端车牌检测算法服务的真实 HTTP 对接需根据最终接口字段做联调验证。
- 若需严格按 `Qt 5.15` 交付，建议在 `Qt 5.15.x` 工具链下执行最终编译回归。
