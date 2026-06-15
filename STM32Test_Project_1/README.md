# STM32Test_Project_1

STM32F103C8T6 + FreeRTOS + DHT22 温湿度传感器，USART1 串口打印。

## 环境

| 工具 | 路径/版本 |
|------|----------|
| CMake | STM32CubeCLT 1.21.0 |
| 编译器 | arm-none-eabi-gcc 14.3.1 |
| 构建工具 | Ninja |
| CubeMX | 6.17.0 |
| HAL 库 | STM32Cube FW_F1 V1.8.7 |

## 构建

```bash
cmake --preset Debug
cmake --build build/Debug
```

## CLion 配置

Settings → Build, Execution, Deployment → CMake → 对应的 Profile：

| 字段 | 值 |
|------|-----|
| CMake | `{STM32CubeCLT}\CMake\bin\cmake.exe` |
| Build tool | `{STM32CubeCLT}\Ninja\bin\ninja.exe` |
| C Compiler | `{STM32CubeCLT}\GNU-tools-for-STM32\bin\arm-none-eabi-gcc.exe` |
| C++ Compiler | `{STM32CubeCLT}\GNU-tools-for-STM32\bin\arm-none-eabi-c++.exe` |
| **CMake options** | `-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake` |

C/C++ Compiler 不能留空，必须指向 CubeCLT 的 arm-none-eabi-gcc/g++。`-mcpu=cortex-m3` 等架构标志由工具链文件注入。
