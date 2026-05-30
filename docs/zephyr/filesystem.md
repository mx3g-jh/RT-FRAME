# 文件系统

本项目通过 SD 卡提供文件系统支持，驱动链路全部基于 Zephyr 原生机制，Shell 命令集由 Zephyr 内置模块提供。

---

## 全景

```
Shell 命令 (fs ls / fs read / ...)
        │
        ▼
  Zephyr fs API  (zephyr/fs/fs.h)
        │
        ▼
  FatFS (ELM FatFS)          CONFIG_FAT_FILESYSTEM_ELM=y
        │
        ▼
  SDMMC 子系统               CONFIG_SDMMC_SUBSYS=y
  (zephyr,sdmmc-disk)
        │
        ▼
  SDHC 驱动层                CONFIG_SDHC=y
        │
        ▼
  NXP i.MX USDHC 驱动        CONFIG_IMX_USDHC=y
  (drivers/sdhc/imx_usdhc.c)
        │
        ▼
  硬件：USDHC1 外设 + SD 卡
```

### Kconfig 配置（targets/cm7/prj.conf）

| 选项 | 说明 |
|------|------|
| `CONFIG_DISK_ACCESS=y` | 磁盘访问抽象层 |
| `CONFIG_SDHC=y` | SDHC 驱动框架 |
| `CONFIG_IMX_USDHC=y` | NXP i.MX USDHC 具体驱动 |
| `CONFIG_SDMMC_SUBSYS=y` | SDMMC 子系统（将 SDHC 暴露为磁盘设备） |
| `CONFIG_FILE_SYSTEM=y` | 文件系统框架 |
| `CONFIG_FAT_FILESYSTEM_ELM=y` | ELM FatFS 实现 |
| `CONFIG_FS_FATFS_MOUNT_MKFS=n` | 禁止挂载时自动格式化（防止意外擦除） |
| `CONFIG_FILE_SYSTEM_SHELL=y` | 内置 fs shell 命令集 |
| `CONFIG_FILE_SYSTEM_SHELL_MOUNT_COMMAND=y` | 启用 `fs mount` 命令 |

### DTS 节点（已配置，无需修改）

```dts
/* boards/nxp/vmu_rt1170/vmu_rt1170_mimxrt1176_cm7.dts */
&usdhc1 {
    status = "okay";
    no-1-8-v;
    pwr-gpios = <&gpio1 1 GPIO_ACTIVE_HIGH>;
    cd-gpios  = <&gpio3 31 (GPIO_ACTIVE_LOW | GPIO_PULL_DOWN)>;
    sdmmc {
        compatible = "zephyr,sdmmc-disk";
        disk-name = "SD";
        status = "okay";
    };
};
```

---

## Shell 命令速查

启用 `CONFIG_FILE_SYSTEM_SHELL=y` 后，串口 Shell 自动获得 `fs` 命令组。

| 命令 | 说明 | 示例 |
|------|------|------|
| `fs mount fatfs /SD:` | 挂载 FAT 文件系统 | — |
| `fs unmount /SD:` | 卸载 | — |
| `fs ls /SD:` | 列出目录 | `fs ls /SD:/logs` |
| `fs cd /SD:` | 切换目录 | — |
| `fs pwd` | 显示当前路径 | — |
| `fs mkdir /SD:/dir` | 创建目录 | — |
| `fs rm /SD:/file.txt` | 删除文件 | — |
| `fs read /SD:/file.txt` | 读取文件内容（hex + ASCII） | — |
| `fs write /SD:/file.txt 0x41 0x42` | 写入字节 | — |
| `fs stat /SD:/file.txt` | 查看文件信息 | — |

挂载点格式为 `/SD:`，冒号是 FatFS 约定，不可省略。

---

## 手动挂载（代码方式）

需要在代码中挂载时：

```c
#include <zephyr/fs/fs.h>

static struct fs_mount_t sd_mount = {
    .type      = FS_FATFS,
    .mnt_point = "/SD:",
    .flags     = 0,
};

int mount_sd(void)
{
    int ret = fs_mount(&sd_mount);
    if (ret != 0) {
        LOG_ERR("mount failed: %d", ret);
    }
    return ret;
}
```

---

## 文件读写示例

```c
#include <zephyr/fs/fs.h>

/* 写文件 */
struct fs_file_t f;
fs_file_t_init(&f);
fs_open(&f, "/SD:/log.txt", FS_O_CREATE | FS_O_WRITE);
fs_write(&f, "hello\n", 6);
fs_close(&f);

/* 读文件 */
fs_open(&f, "/SD:/log.txt", FS_O_READ);
char buf[64];
ssize_t n = fs_read(&f, buf, sizeof(buf));
fs_close(&f);
```

---

## 常见问题

**Q：`fs mount` 返回 -5（EIO）**
SD 卡未插入，或卡检测 GPIO 极性配置错误。检查 `cd-gpios` 定义。

**Q：`fs mount` 返回 -22（EINVAL）**
SD 卡格式不是 FAT32。需在 PC 上重新格式化为 FAT32（不支持 exFAT）。

**Q：Shell 没有 `fs` 命令**
确认 `CONFIG_FILE_SYSTEM_SHELL=y` 已加入 prj.conf，且 `CONFIG_SHELL=y` 也已启用。

**Q：路径写成 `/SD/` 报错**
FatFS 挂载点必须带冒号：`/SD:`，不是 `/SD/`。
