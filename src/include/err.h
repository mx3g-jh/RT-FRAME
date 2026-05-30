/**
 * @file err.h — Zephyr port
 */

#pragma once

#include <log.h>
#include <visibility.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>

__BEGIN_DECLS

#define EXIT(eval) exit(eval)

#define err(eval, ...)  do { PX4_ERR(__VA_ARGS__); EXIT(eval); } while(0)
#define errx(eval, ...) do { PX4_ERR(__VA_ARGS__); EXIT(eval); } while(0)
#define warn(...)       PX4_WARN(__VA_ARGS__)
#define warnx(...)      PX4_WARN(__VA_ARGS__)

__END_DECLS
