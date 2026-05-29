# 分层架构说明

rtframe 采用 PX4 启发的分层架构，基于 Zephyr RTOS 原生机制实现。分层是架构指导，不是编译强制隔离——业务复杂时可以跨层引用，但应尽量保持层间清晰。

---

## 架构总览

```
┌──────────────────────────────────────────────┐
│           应用层 (modules)                    │  src/modules/
│  业务逻辑，通过通信中间件与其他模块通信         │
├──────────────────────────────────────────────┤
│           通信中间件层 (middleware)            │  src/middleware/
│  uORB、zbus、MAVLink 等（各自独立，按需启用）  │
├──────────────────────────────────────────────┤
│           通用库层 (lib)                      │  src/lib/
│  算法、数据结构、工具库，与平台无关             │
├──────────────────────────────────────────────┤
│           驱动抽象层 (drivers)                │  src/drivers/
│  封装 Zephyr 驱动 API，屏蔽芯片差异           │
├──────────────────────────────────────────────┤
│           核心层 (core)                       │  src/core/
│  系统初始化、线程管理、错误处理基础设施         │
├──────────────────────────────────────────────┤
│           板级支持 (boards)                   │  boards/nxp/vmu_rt1170/
│  DTS、Kconfig、main 入口，CM7/CM4 各自独立    │
├──────────────────────────────────────────────┤
│           Zephyr RTOS + HAL                  │  middlewares/zephyr/
│                                              │  hardware/hal_nxp/
└──────────────────────────────────────────────┘
```

---

## 各层说明

### core — 核心层 (`src/core/`)

系统级基础设施：启动序列、看门狗、错误处理、日志初始化、系统时钟封装。

---

### drivers — 驱动抽象层 (`src/drivers/`)

封装 Zephyr 设备驱动 API，为上层提供统一接口。

包含：IMU、气压计、GPS、PWM 输出、CAN 等传感器/执行器的 Zephyr API 封装。

只通过 `zephyr/drivers/*.h` 访问硬件，不直接操作寄存器，不 include `fsl_*.h`。

---

### lib — 通用库层 (`src/lib/`)

与平台无关的算法和工具库：滤波器、PID、矩阵运算、环形缓冲区、单位换算等。

不依赖任何硬件接口，可在 SITL/单元测试中直接复用。

---

### middleware — 通信中间件层 (`src/middleware/`)

模块间通信机制，解耦 `modules` 层各组件。

| 子目录 | 协议 | Kconfig | 默认 |
| ------ | ---- | ------- | ---- |
| `uorb/` | PX4 风格 pub/sub | `CONFIG_RTFRAME_UORB` | 开 |
| `zbus/` | Zephyr 原生 pub/sub | `CONFIG_RTFRAME_ZBUS` | 开 |
| `mavlink/`（待添加） | MAVLink 串行协议 | `CONFIG_RTFRAME_MAVLINK` | 关 |

新增通信协议在此目录下新建子目录，提供独立 Kconfig，默认关闭。

---

### modules — 应用层 (`src/modules/`)

具体业务逻辑，每个模块通常运行在独立 Zephyr 线程中。

包含：姿态估计、飞控控制律、传感器融合、状态机、遥控解析等。

推荐通过 `middleware` 与其他模块通信，直接调用也允许（业务复杂时不强制解耦）。

---

### boards — 板级支持 (`boards/nxp/vmu_rt1170/`)

板级硬件描述和入口点：

```
boards/nxp/vmu_rt1170/
├── vmu_rt1170.yaml
├── vmu_rt1170_mimxrt1176_cm7_defconfig
├── vmu_rt1170_mimxrt1176_cm4_defconfig
├── vmu_rt1170.dts
├── cm7/
│   └── main.cpp    ← CM7 入口
└── cm4/
    └── main.cpp    ← CM4 入口
```

`main.cpp` 负责初始化各层、启动线程、进入 Zephyr 调度。这是 `target_sources(app PRIVATE ...)` 唯一合法出现的地方。

---

## 硬件访问原则

无论在哪一层，访问硬件必须通过 Zephyr 驱动 API：

```c
// ✅ 正确
const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(lpuart1));
uart_tx(dev, buf, len, SYS_FOREVER_US);

// ❌ 禁止
LPUART1->DATA = byte;   // 直接操作寄存器
fsl_lpuart_write(...);  // 直接调用 HAL
```

这是唯一的硬性约束，其余层间规则为架构建议。
