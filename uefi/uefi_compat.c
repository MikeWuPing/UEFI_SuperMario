/*
 * uefi_compat.c — Minimal C standard library implementation for UEFI.
 * Uses UEFI Boot Services (CopyMem, SetMem, AllocatePool, FreePool) where
 * possible, and provides standalone implementations for string functions.
 */

/* Disable MSVC intrinsics so we can define our own versions */
#ifdef _MSC_VER
#pragma function(memset, memcpy, memcmp, memmove)
#pragma function(strlen, strcmp, strncmp, strcpy, strncpy)
#endif

#include "uefi_compat.h"

/* ── Memory operations ─────────────────────────────────────────────────── */

void *memset(void *s, int c, size_t n) {
  if (gBS) {
    gBS->SetMem(s, n, (UINT8)c);
  } else {
    unsigned char *p = (unsigned char *)s;
    while (n--) { *p++ = (unsigned char)c; }
  }
  return s;
}

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
  if (gBS) {
    gBS->CopyMem(dest, (VOID *)src, n);
  } else {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) { *d++ = *s++; }
  }
  return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s2;
  for (size_t i = 0; i < n; i++) {
    if (p1[i] != p2[i]) {
      return (int)p1[i] - (int)p2[i];
    }
  }
  return 0;
}

void *memmove(void *dest, const void *src, size_t n) {
  if (dest < src) {
    return memcpy(dest, src, n);
  }
  unsigned char *d = (unsigned char *)dest;
  const unsigned char *s = (const unsigned char *)src;
  while (n--) { d[n] = s[n]; }
  return dest;
}

/* ── String operations ─────────────────────────────────────────────────── */

size_t strlen(const char *s) {
  const char *p = s;
  while (*p) { p++; }
  return (size_t)(p - s);
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s1 == *s2) { s1++; s2++; }
  return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  if (n == 0) { return 0; }
  while (--n && *s1 && *s1 == *s2) { s1++; s2++; }
  return (unsigned char)*s1 - (unsigned char)*s2;
}

char *strcpy(char *restrict dest, const char *restrict src) {
  char *d = dest;
  while ((*d++ = *src++)) {}
  return dest;
}

char *strncpy(char *restrict dest, const char *restrict src, size_t n) {
  size_t i;
  for (i = 0; i < n && src[i]; i++) {
    dest[i] = src[i];
  }
  for (; i < n; i++) {
    dest[i] = '\0';
  }
  return dest;
}

static int errno_value;
int *__uefi_errno(void) { return &errno_value; }

char *strerror(int errnum) {
  (void)errnum;
  return (char *)"UEFI error";
}

/* ── printf (outputs to ConOut via gST) ────────────────────────────────── */

int printf(const char *format, ...) {
  char buf[1024];
  va_list ap;
  va_start(ap, format);
  int n = vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);

  /* Convert ASCII to UCS-2 for UEFI ConOut */
  CHAR16 wbuf[1024];
  for (int i = 0; i <= n && i < (int)(sizeof(wbuf)/2 - 1); i++) {
    wbuf[i] = (CHAR16)(unsigned char)buf[i];
  }
  wbuf[n] = 0;

  if (gST && gST->ConOut) {
    gST->ConOut->OutputString(gST->ConOut, wbuf);
  }
  return n;
}

/* ── snprintf (minimal implementation) ─────────────────────────────────── */

static void reverse(char *s, int len) {
  for (int i = 0; i < len / 2; i++) {
    char c = s[i];
    s[i] = s[len - 1 - i];
    s[len - 1 - i] = c;
  }
}

static int uitoa(unsigned long long val, char *buf, int base) {
  if (val == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return 1;
  }
  int i = 0;
  while (val > 0) {
    unsigned long long r = val % base;
    buf[i++] = (r < 10) ? (char)('0' + r) : (char)('a' + r - 10);
    val /= base;
  }
  reverse(buf, i);
  buf[i] = '\0';
  return i;
}

static int itoa(long long val, char *buf, int base) {
  if (val < 0) {
    buf[0] = '-';
    return 1 + uitoa((unsigned long long)(-val), buf + 1, base);
  }
  return uitoa((unsigned long long)val, buf, base);
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
  if (size == 0) { return 0; }
  size_t pos = 0;
  const char *f = format;

  while (*f && pos < size - 1) {
    if (*f != '%') {
      str[pos++] = *f++;
      continue;
    }
    f++;
    if (*f == '%') {
      str[pos++] = '%';
      f++;
      continue;
    }
    if (*f == 's') {
      const char *s = va_arg(ap, const char *);
      if (!s) { s = "(null)"; }
      while (*s && pos < size - 1) { str[pos++] = *s++; }
      f++;
      continue;
    }
    if (*f == 'c') {
      str[pos++] = (char)va_arg(ap, int);
      f++;
      continue;
    }
    if (*f == 'd' || *f == 'i') {
      char numbuf[32];
      int len = itoa((long long)va_arg(ap, int), numbuf, 10);
      for (int i = 0; i < len && pos < size - 1; i++) {
        str[pos++] = numbuf[i];
      }
      f++;
      continue;
    }
    if (*f == 'u') {
      char numbuf[32];
      int len = uitoa((unsigned long long)va_arg(ap, unsigned int), numbuf, 10);
      for (int i = 0; i < len && pos < size - 1; i++) {
        str[pos++] = numbuf[i];
      }
      f++;
      continue;
    }
    if (*f == 'x' || *f == 'X') {
      char numbuf[32];
      int len = uitoa((unsigned long long)va_arg(ap, unsigned int), numbuf, 16);
      for (int i = 0; i < len && pos < size - 1; i++) {
        str[pos++] = numbuf[i];
      }
      f++;
      continue;
    }
    if (*f == 'p') {
      char numbuf[32];
      memcpy(numbuf, "0x", 2);
      int len = uitoa((unsigned long long)(UINTN)va_arg(ap, void *), numbuf + 2, 16);
      for (int i = 0; i < len + 2 && pos < size - 1; i++) {
        str[pos++] = numbuf[i];
      }
      f++;
      continue;
    }
    /* Unknown format specifier, copy literally */
    str[pos++] = '%';
    if (pos < size - 1) { str[pos++] = *f++; }
  }
  str[pos] = '\0';
  return (int)pos;
}

int snprintf(char *str, size_t size, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int n = vsnprintf(str, size, format, ap);
  va_end(ap);
  return n;
}

/* ── sscanf (minimal, supports %u and strings) ─────────────────────────── */

int sscanf(const char *str, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int count = 0;
  const char *f = format;
  const char *s = str;

  while (*f) {
    if (*f == ' ') {
      while (*s == ' ') { s++; }
      f++;
      continue;
    }
    if (*f == '%') {
      f++;
      if (*f == 'u') {
        unsigned int *up = va_arg(ap, unsigned int *);
        *up = 0;
        while (*s >= '0' && *s <= '9') {
          *up = *up * 10 + (unsigned int)(*s - '0');
          s++;
        }
        count++;
        f++;
        continue;
      }
      if (*f == 'd') {
        int *ip = va_arg(ap, int *);
        int sign = 1;
        if (*s == '-') { sign = -1; s++; }
        *ip = 0;
        while (*s >= '0' && *s <= '9') {
          *ip = *ip * 10 + (*s - '0');
          s++;
        }
        *ip *= sign;
        count++;
        f++;
        continue;
      }
      if (*f == 's') {
        char *buf = va_arg(ap, char *);
        while (*s && *s != ' ' && *s != '\n') {
          *buf++ = *s++;
        }
        *buf = '\0';
        count++;
        f++;
        continue;
      }
    }
    if (*f == *s) {
      f++;
      s++;
    } else {
      break;
    }
  }
  va_end(ap);
  return count;
}

/* ── SMB2J stubs (not compiled in SMB1-only mode) ──────────────────────── */

void SMB2J_Reset(void) {}
void SMB2J_NMI(void) {}
