/*
 * uefi_debug.h — Serial port debug output for QEMU (COM1 at 0x3F8).
 * All debug_xxx() functions write to the serial port so messages are
 * visible on the host via QEMU's `-serial stdio`.
 */
#ifndef UEFI_DEBUG_H
#define UEFI_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

void debug_init(void);
void debug_putc(char c);
void debug_print(const char *s);
void debug_print_u64(unsigned long long v);
void debug_print_hex(unsigned long long v);
void debug_print_efi_status(const char *msg, unsigned long long status);

#ifdef __cplusplus
}
#endif

#endif /* UEFI_DEBUG_H */
