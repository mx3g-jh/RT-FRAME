# 新 Board 添加流程

参考已有实现：`boards/nxp/vmu_rt1170/`。

---

## 目录结构

```
boards/<vendor>/<board>/
├── <board>.yaml
├── <board>_<soc>_cm7_defconfig
├── <board>_<soc>_cm4_defconfig   （如有 CM4）
├── <board>.dts
├── <board>.dtsi                   （按原型需要提供）
├── board.cmake                    （可选，JLink 等烧录配置）
├── cm7/
│   └── main.cpp
└── cm4/
    └── main.cpp                   （如有 CM4）
```

实际需要提供哪些文件（`.dts`、`.dtsi`、`_defconfig`、`board.cmake` 等），以 `middlewares/zephyr/boards/` 下对应芯片原型为准。

---

## 步骤

**1. 找到 Zephyr 上游原型**

在 `middlewares/zephyr/boards/<vendor>/` 下找到与目标芯片最接近的已有 board 作为模板，将其完整复制到 `boards/<vendor>/<board>/`。

**2. 删除上游原型**

将 `middlewares/zephyr/boards/<vendor>/<原型目录>` 删除，并提交到 `middlewares/zephyr` submodule（mx3g-jh fork）。

这一步是必须的——`BOARD_ROOT` 会让 Zephyr 同时扫描 `rtframe/boards/` 和 `middlewares/zephyr/boards/`，同名 board 会触发 "Board defined multiple times" 错误。

**3. 按新板子修改文件**

- `<board>.dts` / `<board>.dtsi`：修改外设节点、引脚复用
- `<board>_defconfig`：调整 Kconfig 默认值（串口、时钟、内存布局等）
- `<board>.yaml`：修改 board name、arch、toolchain

**4. 确认工具链**

打开 `tools/toolchains.conf`，确认所需架构的工具链已启用（取消注释）：

```
arm-zephyr-eabi          # Cortex-M
# riscv64-zephyr-elf     # RISC-V（按需启用）
```

新架构需要重新运行 `bash tools/setup_env.sh` 下载对应工具链。

**5. 添加 target**

在 `targets/` 下新建目录，复制并修改 `CMakeLists.txt`：

```cmake
cmake_minimum_required(VERSION 3.20)

get_filename_component(RTFRAME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

include("${RTFRAME_ROOT}/cmake/toolchain.cmake")

set(BOARD <board>/<soc>/cm7)

include("${RTFRAME_ROOT}/cmake/zephyr.cmake")

project(rtframe_<board>_cm7 LANGUAGES C CXX ASM)

add_subdirectory("${RTFRAME_ROOT}/src" "${CMAKE_BINARY_DIR}/src")

target_sources(app PRIVATE "${BOARD_DIR}/cm7/main.cpp")
```

**6. 添加 Makefile 目标**

在根目录 `Makefile` 中添加：

```makefile
<board>_cm7:
	$(call cmake_build,targets/<board>_cm7,build/<board>_cm7)
```

**7. 验证**

```bash
make <board>_cm7
```

---

## DTS 注意事项

- rtframe 的 `boards/` 通过 `BOARD_ROOT` 优先于 `middlewares/zephyr/boards/`，同名 board 必须从上游删除
