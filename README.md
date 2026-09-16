# misaka19754 N1 智能车

基于 Infineon AURIX TC264 的 N1 车模控制程序，使用磁编码器进行车轮反馈，包含底层驱动、姿态/传感器处理、路径控制和竞赛模式相关代码。

## 工程环境

- MCU：Infineon AURIX TC264
- IDE：AURIX Development Studio
- 编译器：TASKING TriCore

## 目录说明

- `code/`：车辆控制、算法与应用层代码
- `user/`：启动与用户入口代码
- `libraries/`：底层驱动及芯片支持库

作者：misaka19754
