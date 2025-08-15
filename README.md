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
- Unreal Engine 5.4+
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
   - Or modify model path in `yolo_folder_watcher_single.py`

4. **Open Project**:
   - Double-click `Drone.uproject`
   - Select corresponding Visual Studio version
   - Click "Compile" in UE5 editor

## Project Structure

```
UE5-Drone-Swarm/
├── Source/Drone/                    # C++ source code
│   ├── DroneSwarmTestActor/         # Main swarm controller
│   │   ├── DroneSwarmManagerComponent # Swarm management
│   │   └── GridMapComponent         # 3D grid environment
│   ├── DroneActor/                  # Individual drone logic
│   │   ├── AStarPathFinderComponent # A* path planning
│   │   ├── ObstacleScannerComponent # Obstacle detection
│   │   ├── PathModifierComponent    # Dynamic path adjustment
│   │   └── DroneImageCaptureComponent # Image capture
│   ├── TankSplineController/        # Tank movement controller
│   │   ├── TankSplineMovementComponent    # Spline following
│   │   └── TankDetectionReceiverComponent # YOLO result receiver
│   └── Drone.Build.cs               # Build configuration
├── Source/ThirdParty/               # Third-party libraries
│   ├── PCL/                         # Point Cloud Library
│   ├── FastDDS/                     # Fast DDS middleware
│   ├── Eigen/                       # Linear algebra library
│   └── Boost/                       # Boost C++ libraries
├── yolo_folder_watcher_single.py    # YOLO detection monitor
├── setup_libraries.bat              # Library configuration
└── Drone.uproject                   # UE5 project file
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
   python yolo_folder_watcher_single.py --drone_id 1
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
├── yolo_folder_watcher_single.py (YOLO Detection Monitor)
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

## Implementation Details

### A* Path Planning Algorithm

The system implements a sophisticated 3D A* path planning algorithm with the following features:

- **Spatiotemporal Conflict Resolution**: Uses a reservation table to prevent multiple drones from occupying the same space at the same time
- **Multi-heuristic Support**: Implements diagonal, Manhattan, and Euclidean distance heuristics
- **Path Smoothing**: Applies smoothing algorithms to reduce path jaggedness
- **Safety Margins**: Configurable safety distances and obstacle inflation
- **Dynamic Obstacle Avoidance**: Real-time path modification based on detected obstacles

### Grid Map System

The `GridMapComponent` provides a 3D occupancy grid with:

- **Configurable Resolution**: Adjustable grid cell size (default: 50cm)
- **Dynamic Obstacle Management**: Support for cylindrical obstacles with inflation
- **Automatic Updates**: Real-time grid updates when obstacles are added/removed
- **World-Grid Coordinate Conversion**: Seamless conversion between world and grid coordinates

### Swarm Management

The `DroneSwarmManagerComponent` handles:

- **Formation Control**: Configurable drone formations with spacing control
- **Target Assignment**: Random or fixed target position assignment
- **Beacon Navigation**: Optional beacon-based navigation system
- **Collision Avoidance**: Inter-drone collision prevention

### Image Capture and Processing

The `DroneImageCaptureComponent` provides:

- **Scene Capture**: Real-time scene capture from drone perspective
- **Depth Data**: Depth information extraction for 3D positioning
- **Automatic Saving**: Configurable capture intervals and save directories
- **Multi-drone Support**: Separate directories for each drone

### Tank Detection and Control

The tank tracking system includes:

- **UDP Communication**: Real-time detection result transmission
- **JSON Parsing**: Structured data format for easy integration
- **Spline Movement**: Smooth tank movement along predefined paths
- **Real-time Control**: Dynamic target position updates based on detections

## Configuration Files

### YOLO Configuration

The `yolo_folder_watcher_single.py` script supports:

- **Command Line Arguments**: Configurable drone ID, watch directory, model path
- **UDP Settings**: Configurable UE5 server IP and port
- **Detection Thresholds**: Confidence-based filtering (default: 0.4)
- **Multi-drone Support**: Individual drone folder monitoring

### Library Setup

The `setup_libraries.bat` script handles:

- **Third-party Library Installation**: Automatic copying from vcpkg
- **Directory Structure**: Creation of required include and lib directories
- **Dependency Management**: Boost, PCL, FastDDS, and Eigen setup
- **Error Handling**: Comprehensive error checking and reporting

## Performance Optimization

### Path Planning Optimization

- **Neighbor Node Optimization**: Efficient neighbor node generation
- **Reservation Table**: Static member for global conflict resolution
- **Path Caching**: Stored path reuse for similar queries
- **Early Termination**: Goal-reaching optimization

### Memory Management

- **Smart Pointer Usage**: Proper memory management for dynamic objects
- **Array Optimization**: Efficient data structures for large datasets
- **Component Lifecycle**: Proper cleanup and resource management

### Real-time Processing

- **Configurable Update Rates**: Adjustable tick rates for different components
- **Asynchronous Operations**: Non-blocking image capture and processing
- **Efficient Algorithms**: Optimized algorithms for real-time performance

## Troubleshooting

### Common Issues

1. **Library Loading Errors**:
   - Ensure vcpkg libraries are properly installed
   - Run `setup_libraries.bat` to copy required files
   - Check Visual Studio version compatibility

2. **YOLO Detection Issues**:
   - Verify model path in Python script
   - Check image save directory permissions
   - Ensure UDP port is not blocked by firewall

3. **Path Planning Failures**:
   - Verify grid map initialization
   - Check obstacle configuration
   - Ensure start/end positions are valid

4. **Performance Issues**:
   - Reduce grid resolution for large maps
   - Adjust drone spacing and formation
   - Optimize image capture intervals

### Debug Features

- **Path Visualization**: Visual path display with configurable duration
- **Reservation Table Display**: Spatiotemporal conflict visualization
- **Grid Map Debugging**: Obstacle and grid state visualization
- **Component Logging**: Comprehensive logging for troubleshooting

## Future Enhancements

### Planned Features

- **Machine Learning Integration**: Advanced obstacle classification
- **Multi-objective Optimization**: Energy-efficient path planning
- **Distributed Computing**: Multi-machine swarm coordination
- **Advanced Sensors**: LiDAR and radar integration
- **Cloud Integration**: Remote monitoring and control

### Research Applications

- **Autonomous Navigation**: Research in robotic path planning
- **Swarm Intelligence**: Multi-agent coordination studies
- **Computer Vision**: Real-time object detection research
- **Control Systems**: Advanced control algorithm development

## Contributing

This is a research project open for academic collaboration. Please:

1. Fork the repository
2. Create a feature branch
3. Implement your changes
4. Submit a pull request with detailed documentation

## License

This project is for research and educational purposes. Please ensure compliance with all applicable regulations before practical deployment.

## Contact

For research collaboration and technical support, please contact the development team.

---

**Note**: This is a research project. Please conduct thorough testing and validation before practical applications. The system is designed for simulation and research purposes and should not be used for actual drone operations without proper safety measures and regulatory compliance.
