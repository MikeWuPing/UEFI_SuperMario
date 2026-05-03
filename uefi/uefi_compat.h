/*
 * uefi_compat.h — C standard library shims for the freestanding UEFI environment.
 * Provides the minimal set of functions that the smbcore game library needs.
 */
#ifndef UEFI_COMPAT_H
#define UEFI_COMPAT_H

#include "uefi_types.h"
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Memory operations ─────────────────────────────────────────────────── */

void *memset(void *s, int c, size_t n);
void *memcpy(void *restrict dest, const void *restrict src, size_t n);
int   memcmp(const void *s1, const void *s2, size_t n);
void *memmove(void *dest, const void *src, size_t n);

/* ── String operations ─────────────────────────────────────────────────── */

size_t strlen(const char *s);
int    strcmp(const char *s1, const char *s2);
int    strncmp(const char *s1, const char *s2, size_t n);
char  *strcpy(char *restrict dest, const char *restrict src);
char  *strncpy(char *restrict dest, const char *restrict src, size_t n);
char  *strerror(int errnum);

/* ── printf family (used for console output via ConOut) ────────────────── */

int printf(const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);

/* ── stdlib ────────────────────────────────────────────────────────────── */

#ifndef assert
#define assert(expr) ((void)0)  /* game core uses assert; disable in UEFI */
#endif

#ifndef errno
#define errno (*__uefi_errno())
int *__uefi_errno(void);
#endif

int sscanf(const char *str, const char *format, ...);

/* ── File I/O stubs (not used by game core, needed for linking only) ──── */

/* These are only used by the SDL frontend. The UEFI frontend uses
 * EFI_FILE_PROTOCOL directly and does not call these. */

#ifdef __cplusplus
}
#endif

#endif /* UEFI_COMPAT_H */
