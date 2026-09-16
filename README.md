# n1-smart-car — TC264 N1 车模控制程序

第 21 届全国大学生智能汽车竞赛单车定向组的 N1 车模控制程序，磁编码器轮速反馈，基于 Infineon AURIX TC264（AURIX Development Studio + TASKING TriCore 编译器）。

## 系统组成

| 部分 | 说明 |
|---|---|
| 姿态解算 | 四元数三轴解算（`code/Imu.c`），陀螺仪/加速度计融合 |
| 惯导 | `code/Ins.c`，机械解除后的位置推算 |
| 控制结构 | 科目1：三环串级（角度-角速度-电流）；科目2：双环串级，处理 YAW 角 -180°/179° 跳变与最短路径转向 |
| 赛题任务 | `code/Subject_1/2/3.c` 对应三个科目的动作流程 |
| 人机交互 | `code/Menu.c` 屏幕菜单调参 + 按键 |

## 目录结构

```
├── code/                  # 应用层
│   ├── Agorithm.c         # 通用算法（滤波、PID 等）
│   ├── Imu.c / Ins.c      # 姿态解算 / 惯导
│   ├── Subject_1/2/3.c    # 各科目任务逻辑
│   ├── Menu.c             # 屏幕菜单
│   └── Common_peripherals.c  # 公共外设（电机/舵机/编码器/按键/屏幕/蜂鸣器/遥控）
├── user/                  # 启动与入口
├── libraries/             # 底层驱动与芯片支持库
├── .project / .cproject   # AURIX Development Studio 工程文件
└── AURIX修改工程名称.bat   # 工程改名脚本
```

## 版本记录

- **V2.0**：面向科目1的三环串级结构；四元数三轴解算；基础外设（舵机、电机、编码器、按键、屏幕、蜂鸣器、遥控器）
- **V2.1_Beta**：科目2双环串级，解决 YAW 角 ±180° 跳变下的稳定与最短路径问题；加入惯导；遥控控制

## 构建与烧录

1. 安装 AURIX Development Studio（TASKING 编译器免费版即可编译）
2. Import → AURIX Development Studio Project → 选择本目录
3. 若导入后工程名冲突，先运行 `AURIX修改工程名称.bat` 改名再导入
4. 编译后通过调试器烧录板卡

## 说明

- `libraries/` 中 `zf_*` 为逐飞科技 TC264 开源库组件，保持原样未改动
- `code/调试参数.txt` 记录实车调试参数，仅供参照
- 编译产物（`.o`、`.d`、`Debug/` 等）不入库
