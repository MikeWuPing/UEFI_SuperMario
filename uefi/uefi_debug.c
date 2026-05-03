/*
 * uefi_debug.c — Serial port debug output via COM1 (I/O port 0x3F8).
 */

#include "uefi_debug.h"
#include "uefi_types.h"

#ifdef _MSC_VER
#include <intrin.h>
#endif

#define COM1_BASE 0x3F8

static int serial_ready(void) {
#ifdef _MSC_VER
  return (__inbyte(COM1_BASE + 5) & 0x20) != 0;
#else
  return 1;
#endif
}

static void serial_write(unsigned char c) {
#ifdef _MSC_VER
  __outbyte(COM1_BASE, c);
#else
  (void)c;
#endif
}

void debug_init(void) {
  /* Initialize COM1 serial port to 115200 8N1 */
#ifdef _MSC_VER
  __outbyte(COM1_BASE + 1, 0x00);  /* Disable interrupts */
  __outbyte(COM1_BASE + 3, 0x80);  /* Enable DLAB */
  __outbyte(COM1_BASE + 0, 0x01);  /* Baud low (115200: 115200 = 115200 / 1) */
  __outbyte(COM1_BASE + 1, 0x00);  /* Baud high */
  __outbyte(COM1_BASE + 3, 0x03);  /* 8N1, disable DLAB */
  __outbyte(COM1_BASE + 2, 0xC7);  /* Enable FIFO, clear, 14-byte threshold */
#endif
}

void debug_putc(char c) {
  if (c == '\n') {
    while (!serial_ready()) {}
    serial_write('\r');
  }
  while (!serial_ready()) {}
  serial_write((unsigned char)c);
}

void debug_print(const char *s) {
  while (*s) {
    debug_putc(*s++);
  }
}

void debug_print_u64(unsigned long long v) {
  char buf[32];
  int i = 0;
  if (v == 0) {
    debug_putc('0');
    return;
  }
  while (v > 0) {
    buf[i++] = (char)('0' + (v % 10));
    v /= 10;
  }
  while (i > 0) {
    debug_putc(buf[--i]);
  }
}

void debug_print_hex(unsigned long long v) {
  debug_print("0x");
  char buf[16];
  int i = 0;
  int leading = 1;
  for (int shift = 60; shift >= 0; shift -= 4) {
    int nibble = (int)((v >> shift) & 0xF);
    if (nibble == 0 && leading && shift > 0) continue;
    leading = 0;
    debug_putc((char)(nibble < 10 ? '0' + nibble : 'a' + nibble - 10));
  }
  if (leading) debug_putc('0');
}

void debug_print_efi_status(const char *msg, unsigned long long status) {
  debug_print(msg);
  debug_print(": ");
  if (status == 0) {
    debug_print("OK\n");
  } else {
    debug_print("FAILED (");
    debug_print_hex(status);
    debug_print(")\n");
  }
}
