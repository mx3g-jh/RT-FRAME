/**
 * @file visibility.h — Zephyr port
 */

#pragma once

#ifdef __EXPORT
#  undef __EXPORT
#endif
#define __EXPORT

#ifdef __PRIVATE
#  undef __PRIVATE
#endif
#define __PRIVATE

#ifdef __cplusplus
#  define __BEGIN_DECLS  extern "C" {
#  define __END_DECLS    }
#else
#  define __BEGIN_DECLS
#  define __END_DECLS
#endif
