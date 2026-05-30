#pragma once

/*
 * 由 board 的 main() 调用，初始化 rtframe 核心，
 * 然后调用用户实现的 task_init()。
 */
void rtframe_core_init();

/*
 * 用户在自己的模块里实现。在此创建并 start 各个 VWork 周期任务。
 * 调用时机：rtframe_core_init() 内部，系统已就绪。
 */
void task_init();
