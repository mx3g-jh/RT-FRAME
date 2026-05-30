#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include "rtframe_core.h"
#include "task_register.h"

STRUCT_SECTION_START_EXTERN(task_entry);
STRUCT_SECTION_END_EXTERN(task_entry);

static const char *model_str(vwork::Model m)
{
	switch (m) {
	case vwork::Model::WORKQUEUE: return "workq";
	case vwork::Model::THREAD:    return "thread";
	}
	return "?";
}

/* 汇总打印所有已注册任务（启动时 + 可被 shell 复用）。 */
void task_list_print()
{
	printk("\n%-18s %-9s %-16s %8s %4s %5s %3s\n",
	       "TASK", "MODEL", "CONFIG", "PERIOD", "PRIO", "STACK", "LVL");
	STRUCT_SECTION_FOREACH(task_entry, e) {
		printk("%-18s %-9s %-16s %8u %4d %5u %3u\n",
		       e->name, model_str(e->model), e->cfg_name,
		       e->period_us, e->priority, e->stacksize, e->init_level);
	}
	printk("\n");
}

void task_init()
{
	/* 按 init_level 从低到高启动；同级内按 section 顺序 */
	for (int level = INIT_LEVEL_DRIVER; level <= INIT_LEVEL_APP; ++level) {
		STRUCT_SECTION_FOREACH(task_entry, e) {
			if (e->init_level == level) {
				bool ok = e->start_fn();
				if (!ok) {
					printk("[core] start FAIL: %s\n", e->name);
				}
			}
		}
	}
}

void rtframe_core_init()
{
	printk("[core] rtframe init\n");
	task_list_print();
	task_init();
	printk("[core] tasks started\n");
}

/* ---------- shell: rtframe task ---------- */
static int cmd_task(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	task_list_print();
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(rtframe_subcmds,
	SHELL_CMD(task, NULL, "List registered tasks", cmd_task),
	SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(rtframe, &rtframe_subcmds, "rtframe commands", NULL);
