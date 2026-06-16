# AGENTS.md - STM32 ClionProject

## 项目结构

每个 STM32Test_Project_N 是独立子项目，共享同一 git 仓库。

```
STM32_ClionProject/
├── STM32Test_Project_1/
│   ├── .gitignore
│   ├── README.md
│   ├── AGENTS.md
│   ├── CMakeLists.txt
│   ├── CMakePresets.json
│   ├── cmake/
│   │   ├── gcc-arm-none-eabi.cmake     # 工具链文件
│   │   └── stm32cubemx/CMakeLists.txt  # CubeMX 生成的 CMake
│   ├── Core/
│   │   ├── Inc/                        # CubeMX 生成的头文件
│   │   └── Src/                        # CubeMX 生成的源文件
│   ├── Drivers/
│   │   ├── BSP/                        # 板级支持包 (DHT22, LED)
│   │   ├── CMSIS/
│   │   ├── STM32F1xx_HAL_Driver/
│   │   └── SYSTEM/                     # 正点原子系统文件 (delay, sys, usart)
│   ├── Middlewares/
│   │   └── Third_Party/FreeRTOS/       # CubeMX 生成的 FreeRTOS
│   └── SMT32Test_Project_1.ioc        # CubeMX 配置文件
└── STM32Test_Project_2/  (未来)
```

## 构建命令

```bash
cd STM32Test_Project_1
cmake --preset Debug -S . -B build/Debug
cmake --build build/Debug
```

## CubeMX 生成代码后注意事项

1. `ProjectManager.KeepUserCode=true` — 用户代码在 `/* USER CODE BEGIN/END */` 间保留
2. 手动维护的文件在顶层 `CMakeLists.txt` 的 `target_sources` 中（ALIENTEK BSP/SYSTEM, DHT22）
3. 旧 `Drivers/SYSTEM/usart/usart.c` 中的冲突函数已用 `#if 0` 屏蔽
4. CLion profile: C/C++ Compiler 指向 STM32CubeCLT 的 arm-none-eabi-gcc/g++，CMake options 加 `-DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake`

## 编码约定

- **跨模块函数必须声明在头文件中**：一个 `.c` 中定义、其他 `.c` 中引用的函数，把函数原型写在对应头文件的 `/* USER CODE BEGIN EFP */` 区域（CubeMX 重生成时会保留），不要依赖 `.c` 内部的局部 `extern` 声明
  - 例如：`LED_Task`/`DHT22_Task` 定义在 `main.c`，被 `freertos.c` 调用 → 原型写在 `main.h`
  - CLion 导航依赖头文件声明，局部 `extern` 会导致无法跳转到定义

## FreeRTOS

- CMSIS-RTOS v1 封装（CubeMX 生成）
- 用户任务使用原生 FreeRTOS API（`xTaskCreate`, `vTaskDelay`）
- HAL 时基：TIM2（SysTick 留给 FreeRTOS）
- 配置：72MHz, Tick=1000Hz, Heap=8KB, 静态分配
