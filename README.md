# UE5-Drone-Swarm
UE5-based intelligent multi-drone swarm system for path planning, collision avoidance, and coordinated control.
# 无人机群控制系统 (Drone Swarm Control System)

## 项目概述

这是一个基于虚幻引擎5 (Unreal Engine 5) 开发的综合无人机群控制系统，实现了多无人机协同路径规划、障碍物避让、图像捕获、坦克实时追踪、YOLO目标检测等功能。项目采用组件化架构设计，支持实时路径规划、群组管理、计算机视觉分析和可视化调试。

## 技术特性

- **实时路径规划**: 基于A*算法的3D路径规划
- **群组管理**: 支持多无人机协同控制
- **障碍物检测**: 动态障碍物扫描和避让
- **图像捕获**: 无人机视角图像采集
- **坦克追踪**: 基于YOLO的实时目标检测和追踪
- **坦克控制**: 坦克沿样条线移动控制
- **可视化调试**: 路径可视化、预约表显示
- **第三方库集成**: PCL、Eigen、FastDDS、OpenCV、YOLO等

## 系统架构

### 核心组件关系图

```
DroneSwarmTestActor (主控制器)
├── DroneSwarmManagerComponent (群组管理器)
├── GridMapComponent (网格地图)
└── 多个 DroneActor (无人机实例)
    ├── AStarPathFinderComponent (路径规划)
    ├── ObstacleScannerComponent (障碍扫描)
    ├── PathModifierComponent (路径修改)
    └── DroneImageCaptureComponent (图像捕获)

TankSplineController (坦克控制器)
├── TankSplineMovementComponent (坦克移动控制)
└── TankDetectionReceiverComponent (坦克检测接收器)

外部系统:
├── yolo_folder_watcher.py (YOLO检测监控)
└── 图像处理管道
```

### 组件详细说明

#### 1. DroneSwarmTestActor (主控制器)
- **功能**: 整个无人机系统的入口点，负责初始化和管理无人机群
- **主要职责**:
  - 生成无人机实例
  - 配置群组参数（数量、间距、速度等）
  - 管理起始队形和目标区域
  - 协调整个系统的运行
- **关键参数**:
  - `NumDrones`: 无人机数量
  - `DroneSpacing`: 无人机间距
  - `DroneSpeed`: 移动速度
  - `FormationConfig`: 起始队形配置
  - `TargetConfig`: 目标区域配置

#### 2. DroneSwarmManagerComponent (群组管理器)
- **功能**: 管理多个无人机的协同行为
- **主要职责**:
  - 无人机群组状态管理
  - 任务分配和协调
  - 群组行为控制
  - 冲突避免和协调

#### 3. GridMapComponent (网格地图)
- **功能**: 3D网格化环境表示
- **主要职责**:
  - 环境地图初始化
  - 障碍物标记和管理
  - 坐标转换（世界坐标 ↔ 网格坐标）
  - 碰撞检测支持
- **关键参数**:
  - `GridResolution`: 网格分辨率
  - `MapMargin`: 地图外扩边距
  - `InflationRadius`: 障碍物膨胀半径

#### 4. DroneActor (无人机主体)
- **功能**: 单个无人机的核心逻辑
- **主要职责**:
  - 无人机状态管理
  - 运动控制
  - 组件协调
  - 路径执行
- **包含组件**:
  - 路径规划组件
  - 障碍扫描组件
  - 路径修改组件
  - 图像捕获组件

#### 5. AStarPathFinderComponent (路径规划)
- **功能**: A*算法的3D路径规划实现
- **主要职责**:
  - 起点到终点的路径计算
  - 时空冲突检测和避免
  - 路径平滑和优化
  - 预约表管理
- **关键参数**:
  - `PathSmoothingFactor`: 路径平滑度 (0.0-1.0)
  - `DroneRadius`: 无人机半径
  - `SafetyFactor`: 安全距离系数 (1.0-3.0)
  - `PathPointSpacing`: 路径点间距

#### 6. ObstacleScannerComponent (障碍扫描)
- **功能**: 实时障碍物检测
- **主要职责**:
  - 环境感知
  - 动态障碍物识别
  - 安全距离计算
  - 实时碰撞检测

#### 7. PathModifierComponent (路径修改)
- **功能**: 动态路径调整
- **主要职责**:
  - 实时路径优化
  - 障碍物避让路径生成
  - 路径冲突解决
  - 动态重规划

#### 8. DroneImageCaptureComponent (图像捕获)
- **功能**: 无人机视角图像采集
- **主要职责**:
  - 实时图像捕获
  - 图像处理和存储
  - 视觉信息分析
  - 深度信息计算

#### 9. TankSplineController (坦克控制器)
- **功能**: 坦克移动和检测的主控制器
- **主要职责**:
  - 坦克和样条线的自动查找
  - 坦克移动组件的配置
  - 移动参数的设置和控制
  - 系统状态管理
- **关键功能**:
  - 自动查找场景中的坦克Actor
  - 自动查找样条线组件
  - 坦克移动的启动、停止、重置控制

#### 10. TankSplineMovementComponent (坦克移动控制)
- **功能**: 控制坦克沿样条线移动
- **主要职责**:
  - 样条线跟随移动
  - 平滑的旋转插值
  - 循环或单次移动控制
  - 速度和时间控制
- **关键参数**:
  - `MovementSpeed`: 移动速度
  - `RotationSpeed`: 旋转速度
  - `bLoopMovement`: 是否循环移动
  - `bFollowSplineDirection`: 是否跟随样条线方向

#### 11. TankDetectionReceiverComponent (坦克检测接收器)
- **功能**: 接收和处理YOLO检测结果
- **主要职责**:
  - UDP消息接收和解析
  - JSON数据解析
  - 检测结果处理
  - 无人机目标更新
- **关键特性**:
  - 异步消息处理
  - 多线程UDP监听
  - 自动深度信息计算
  - 实时目标位置更新

#### 12. yolo_folder_watcher.py (YOLO检测监控)
- **功能**: 外部Python脚本，监控图像文件夹并进行YOLO检测
- **主要职责**:
  - 实时文件夹监控
  - YOLO模型推理
  - 检测结果处理
  - UDP消息发送
- **关键特性**:
  - 多线程文件夹监控
  - 置信度过滤
  - 连续检测验证
  - 结果可视化保存

## 坦克追踪系统工作流程

### 1. 图像捕获阶段
```
无人机飞行 → 图像捕获 → 保存到指定文件夹
```

### 2. YOLO检测阶段
```
文件夹监控 → 图像读取 → YOLO推理 → 结果过滤 → UDP发送
```

### 3. UE5接收处理阶段
```
UDP接收 → JSON解析 → 深度计算 → 目标更新 → 路径重规划
```

### 4. 坦克移动控制阶段
```
目标位置更新 → 样条线移动 → 实时位置控制 → 平滑运动
```

## 安装和配置

### 系统要求
- Windows 10/11
- Visual Studio 2019/2022
- Unreal Engine 5.0+
- Python 3.8+
- vcpkg包管理器

### Python环境配置

1. **安装Python依赖**:
```bash
pip install ultralytics opencv-python numpy
```

2. **配置YOLO模型**:
- 将训练好的YOLO模型放在 `runs/detect/train/weights/best.pt`
- 或修改 `yolo_folder_watcher.py` 中的模型路径

### 依赖库安装

1. **安装vcpkg**:
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
```

2. **安装必要的库**:
```bash
vcpkg install eigen3:x64-windows
vcpkg install pcl:x64-windows
vcpkg install fastdds:x64-windows
```

3. **运行库文件配置脚本**:
```bash
setup_libraries.bat
```

### 项目构建

1. **打开项目**:
   - 双击 `Drone.uproject`
   - 选择对应的Visual Studio版本

2. **编译项目**:
   - 在UE5编辑器中点击"编译"按钮
   - 或在Visual Studio中构建解决方案

## 使用方法

### 基本设置

1. **创建测试场景**:
   - 在UE5编辑器中创建新关卡
   - 添加 `DroneSwarmTestActor` 到场景中
   - 添加 `TankSplineController` 到场景中

2. **配置无人机参数**:
   - 设置无人机数量 (`NumDrones`)
   - 配置无人机间距 (`DroneSpacing`)
   - 设置移动速度 (`DroneSpeed`)
   - 定义起始队形 (`FormationConfig`)

3. **配置坦克追踪系统**:
   - 设置坦克移动速度 (`TankMovementSpeed`)
   - 配置旋转速度 (`RotationSpeed`)
   - 启用自动开始移动 (`bAutoStartMovement`)

4. **配置YOLO检测**:
   - 修改 `yolo_folder_watcher.py` 中的路径配置
   - 设置监控文件夹路径 (`WATCH_ROOT`)
   - 配置UE5的IP和端口

### 运行测试

1. **启动无人机群组测试**:
   - 调用 `StartSwarmTest()` 函数
   - 系统将自动生成无人机并开始路径规划

2. **启动坦克追踪系统**:
   - 运行 `python yolo_folder_watcher.py`
   - 系统将开始监控图像文件夹
   - 检测结果将自动发送到UE5

3. **监控运行状态**:
   - 观察无人机运动轨迹
   - 查看坦克移动状态
   - 监控检测结果和路径更新

### 高级配置

#### 路径规划参数
- `PathSmoothingFactor`: 路径平滑度 (0.0-1.0)
- `DroneRadius`: 无人机半径
- `SafetyFactor`: 安全距离系数 (1.0-3.0)
- `PathPointSpacing`: 路径点间距

#### 网格地图参数
- `GridResolution`: 网格分辨率
- `MapMargin`: 地图外扩边距
- `InflationRadius`: 障碍物膨胀半径

#### 群组行为参数
- `bUseBeaconSystem`: 是否使用信标系统
- `StartBeaconTagPrefix`: 起点信标标签前缀
- `EndBeaconTagPrefix`: 终点信标标签前缀

#### 坦克控制参数
- `TankMovementSpeed`: 坦克移动速度
- `RotationSpeed`: 旋转插值速度
- `bLoopMovement`: 是否循环移动
- `bFollowSplineDirection`: 是否跟随样条线方向

#### YOLO检测参数
- `WATCH_ROOT`: 监控的根文件夹路径
- `YOLO_MODEL_PATH`: YOLO模型文件路径
- `UE5_IP`: UE5应用的IP地址
- `UE5_PORT`: UE5应用的UDP端口

## 开发指南

### 添加新功能

1. **创建新组件**:
   - 继承自 `UActorComponent`
   - 实现必要的接口函数
   - 在相应的Actor中集成

2. **扩展路径规划**:
   - 修改 `AStarPathFinderComponent`
   - 添加新的启发式函数
   - 实现自定义路径优化算法

3. **增强障碍检测**:
   - 扩展 `ObstacleScannerComponent`
   - 添加新的传感器类型
   - 实现更复杂的碰撞检测

4. **扩展目标检测**:
   - 修改 `yolo_folder_watcher.py`
   - 添加新的检测类别
   - 实现自定义后处理逻辑

### 调试和测试

1. **路径可视化**:
   - 使用 `VisualizePath()` 函数
   - 设置 `PathDisplayDuration` 参数
   - 观察路径线条和节点

2. **预约表监控**:
   - 调用 `VisualizeReservationTable()`
   - 使用 `GetReservationTableString()` 获取文本信息
   - 分析时空冲突情况

3. **坦克追踪调试**:
   - 检查UDP消息接收
   - 验证检测结果解析
   - 监控目标位置更新

4. **YOLO检测调试**:
   - 查看检测结果图片
   - 检查UDP消息发送
   - 验证模型推理结果

### 性能分析

1. **路径规划性能**:
   - 监控路径规划耗时
   - 分析内存使用情况
   - 优化算法效率

2. **图像处理性能**:
   - 监控YOLO推理速度
   - 优化图像预处理
   - 调整检测频率

3. **网络通信性能**:
   - 监控UDP消息延迟
   - 优化消息格式
   - 调整发送频率

## 故障排除

### 常见问题

1. **编译错误**:
   - 检查第三方库是否正确安装
   - 确认UE5版本兼容性
   - 验证库文件路径设置

2. **运行时错误**:
   - 检查网格地图初始化
   - 验证无人机参数设置
   - 确认场景中无障碍物冲突

3. **坦克追踪问题**:
   - 检查UDP端口是否被占用
   - 验证Python脚本是否正常运行
   - 确认图像文件夹路径正确

4. **YOLO检测问题**:
   - 检查模型文件路径
   - 验证Python依赖是否正确安装
   - 确认图像格式支持

5. **性能问题**:
   - 调整网格分辨率
   - 优化路径规划参数
   - 减少同时运行的无人机数量
   - 调整YOLO检测频率

### 调试技巧

1. **启用详细日志**:
   - 在UE5编辑器中设置日志级别
   - 使用 `UE_LOG` 输出调试信息
   - 在Python脚本中添加print语句

2. **可视化调试**:
   - 启用路径可视化
   - 显示网格地图边界
   - 标记关键位置点
   - 保存检测结果图片

3. **网络调试**:
   - 使用网络抓包工具
   - 检查UDP消息格式
   - 验证IP和端口配置

## 贡献指南

欢迎提交问题报告和功能请求！请遵循以下步骤：

1. Fork项目仓库
2. 创建功能分支
3. 提交更改
4. 创建Pull Request

## 许可证

本项目采用MIT许可证 - 详见LICENSE文件

## 联系方式

如有问题或建议，请通过以下方式联系：
- 提交GitHub Issue
- 发送邮件至项目维护者

---

**注意**: 这是一个研究项目，请在实际应用中进行充分测试和验证。
