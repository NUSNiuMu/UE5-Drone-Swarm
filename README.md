# UE5-Drone-Swarm
UE5-based intelligent multi-drone swarm system for path planning, collision avoidance, and coordinated control.

## Project Overview

This project is a comprehensive drone swarm control system developed in Unreal Engine 5, implementing multi-drone cooperative path planning, obstacle avoidance, image capture, real-time tank tracking, and YOLO object detection. The system features a component-based architecture supporting real-time path planning, swarm management, computer vision analysis, and visual debugging capabilities.

The project demonstrates how advanced robotics algorithms can be integrated with game engine technology to create sophisticated autonomous drone systems for research and development purposes.

## Key Features

- **Real-time Path Planning**: 3D path planning based on A* algorithm with spatiotemporal conflict resolution
- **Swarm Management**: Multi-drone cooperative control and coordination
- **Obstacle Detection**: Dynamic obstacle scanning and avoidance
- **Image Capture**: Drone perspective image acquisition and processing
- **Tank Tracking**: Real-time YOLO-based object detection and tracking
- **Tank Control**: Tank movement control along spline paths
- **Visual Debugging**: Path visualization, reservation table display
- **Third-party Integration**: PCL, Eigen, FastDDS, OpenCV, YOLO integration

## System Requirements

### Software
- Windows 10/11
- Visual Studio 2019/2022
- Unreal Engine 5.0+
- Python 3.8+
- vcpkg package manager

### Hardware
- CUDA-capable NVIDIA GPU (recommended for YOLO inference)
- 8GB+ GPU memory for UE5
- 16GB+ system memory
- High-speed SSD for real-time image processing

## Installation

1. **Install Python Dependencies**:
   ```bash
   pip install ultralytics opencv-python numpy
   ```

2. **Install vcpkg and Libraries**:
   ```bash
   # Clone vcpkg
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   ./bootstrap-vcpkg.bat
   
   # Install required libraries
   vcpkg install eigen3:x64-windows
   vcpkg install pcl:x64-windows
   vcpkg install fastdds:x64-windows
   
   # Run library setup script
   setup_libraries.bat
   ```

3. **Configure YOLO Model**:
   - Place trained YOLO model in `runs/detect/train/weights/best.pt`
   - Or modify model path in `yolo_folder_watcher.py`

4. **Open Project**:
   - Double-click `Drone.uproject`
   - Select corresponding Visual Studio version
   - Click "Compile" in UE5 editor

## Project Structure

```
UE5-Drone-Swarm/
├── DroneSwarmTestActor/           # Main swarm controller
│   ├── DroneSwarmManagerComponent # Swarm management
│   └── GridMapComponent          # 3D grid environment
├── DroneActor/                    # Individual drone logic
│   ├── AStarPathFinderComponent  # A* path planning
│   ├── ObstacleScannerComponent  # Obstacle detection
│   ├── PathModifierComponent     # Dynamic path adjustment
│   └── DroneImageCaptureComponent # Image capture
├── TankSplineController/          # Tank movement controller
│   ├── TankSplineMovementComponent    # Spline following
│   └── TankDetectionReceiverComponent # YOLO result receiver
├── yolo_folder_watcher.py        # YOLO detection monitor
├── setup_libraries.bat           # Library configuration
└── Drone.uproject                # UE5 project file
```

## Usage

### Basic Setup

1. **Create Test Scene**:
   ```bash
   # Add DroneSwarmTestActor to scene
   # Configure drone parameters (NumDrones, DroneSpacing, DroneSpeed)
   # Set formation and target configurations
   ```

2. **Configure Tank Tracking**:
   ```bash
   # Add TankSplineController to scene
   # Set movement and rotation speeds
   # Enable auto-start movement
   ```

3. **Start YOLO Detection**:
   ```bash
   python yolo_folder_watcher.py
   ```

### Training and Testing

```bash
# Start swarm test
StartSwarmTest()

# Monitor drone movement and path planning
# Observe tank tracking and detection results
# Analyze performance metrics
```

### Advanced Configuration

#### Path Planning Parameters
- `PathSmoothingFactor`: Path smoothness (0.0-1.0)
- `DroneRadius`: Drone safety radius
- `SafetyFactor`: Safety distance multiplier (1.0-3.0)
- `PathPointSpacing`: Distance between path points

#### Grid Map Parameters
- `GridResolution`: Grid cell size
- `MapMargin`: Map boundary margin
- `InflationRadius`: Obstacle inflation radius

#### Swarm Behavior Parameters
- `bUseBeaconSystem`: Enable beacon-based navigation
- `StartBeaconTagPrefix`: Starting beacon identifier
- `EndBeaconTagPrefix`: Target beacon identifier

#### Tank Control Parameters
- `TankMovementSpeed`: Tank movement velocity
- `RotationSpeed`: Rotation interpolation speed
- `bLoopMovement`: Enable continuous movement
- `bFollowSplineDirection`: Follow spline orientation

## System Architecture

### Core Component Relationships

```
DroneSwarmTestActor (Main Controller)
├── DroneSwarmManagerComponent (Swarm Manager)
├── GridMapComponent (Grid Environment)
└── Multiple DroneActor Instances
    ├── AStarPathFinderComponent (Path Planning)
    ├── ObstacleScannerComponent (Obstacle Detection)
    ├── PathModifierComponent (Path Modification)
    └── DroneImageCaptureComponent (Image Capture)

TankSplineController (Tank Controller)
├── TankSplineMovementComponent (Movement Control)
└── TankDetectionReceiverComponent (Detection Receiver)

External Systems:
├── yolo_folder_watcher.py (YOLO Detection Monitor)
└── Image Processing Pipeline
```

### Tank Tracking Workflow

1. **Image Capture Phase**:
   ```
   Drone Flight → Image Capture → Save to Folder
   ```

2. **YOLO Detection Phase**:
   ```
   Folder Monitoring → Image Reading → YOLO Inference → Result Filtering → UDP Transmission
   ```

3. **UE5 Processing Phase**:
   ```
   UDP Reception → JSON Parsing → Depth Calculation → Target Update → Path Replanning
   ```

4. **Tank Movement Control**:
   ```
   Target Position Update → Spline Movement → Real-time Position Control → Smooth Motion
   ```



**Note**: This is a research project. Please conduct thorough testing and validation before practical applications.
