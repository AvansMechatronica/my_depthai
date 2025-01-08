# Algemene informatie


## Pre-requistions ROS2-Humble
Volg de instructies voor het [installeren ROS-Humble](https://docs.ros.org/en/humble/Installation/Ubuntu-Install-Debs.html)

Kies: ros-humble-desktop

-- Of --

Clone deze repository(zie hieronder) en vervolgens
```bash
cd ~/ros2_industrial_ws/src/install
./install_ros_humble_desktop.bash
./install_colcon.bash
```

## Cloning de ROS2 Industrial workspace
```bash
mkdir -p /ros2_industrial_ws/src
cd ~/ros2_industrial_ws/src
git clone https://github.com/AvansMechatronica/ROS2_industrial.git
```

## Installeren van de benodigde ROS2 packages en software
```bash
cd ~/ros2_industrial_ws/src/ROS2_industrial/install
.install
```

## Bouwen van de ROS2 Industrial workspace
```bash
cd ~/ros2_industrial_ws/
colcon build --symlink-install
source install/setup.bash
echo "source ~/ros2_industrial_ws/install/setup.bash" >> ~/.bashrc
```
Opmerking: Gebruik de laatse regel slechts 1 maal.

## Aantekening voor windows gebruikers
You can use the WSL Ubuntu-22.04 distribution form the Microsoft Store. Use as development environment [Visual Studio Code](https://code.visualstudio.com/download). Alter installing add the WSL-plugin to Visual Studio Code. Open the distribution with <F1>WSL: Connect to WSL. Don't forget to install ROS-Humble into the distribution. Clone this repositry into te WSL distribution


## Verantwoording

Deze workshop is geinspireerd op [hello real world ros robot operating system](https://ocw.tudelft.nl/courses/hello-real-world-ros-robot-operating-system/) van de Technische Universiteit Delft/Nederland(TUD)

