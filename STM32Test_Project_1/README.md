# STM32 ClionProject

基于 CLion + CMake + Ninja + arm-none-eabi-gcc 的 STM32 开发仓库，使用 STM32CubeMX 生成初始化代码。

## 环境

| 工具 | 路径/版本 |
|------|----------|
| CMake | STM32CubeCLT 1.21.0 |
| 编译器 | arm-none-eabi-gcc 14.3.1 |
| 构建工具 | Ninja |
| CubeMX | 6.17.0 |
| HAL 库 | STM32Cube FW_F1 V1.8.7 |

## 项目列表

| 项目 | 芯片 | RTOS | 说明 |
|------|------|------|------|
| [STM32Test_Project_1](STM32Test_Project_1/) | STM32F103C8T6 | FreeRTOS CMSIS-RTOS v1 | DHT22 温湿度传感器 + USART1 串口打印 |

## 构建

每个子项目独立构建：

```bash
cd STM32Test_Project_1
cmake --preset Debug
cmake --build build/Debug
```

## CLion 配置

Settings → CMake → Profile 需清空 C/C++ Compiler 栏，在 CMake options 填：

```
-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake
```
