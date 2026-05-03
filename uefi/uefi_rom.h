/*
 * uefi_rom.h — ROM file I/O for the SMB UEFI port.
 * Opens an NES/FDS ROM file via EFI_SIMPLE_FILE_SYSTEM_PROTOCOL and
 * provides the read_rom_bytes / seek_rom callbacks for the game library.
 */
#ifndef UEFI_ROM_H
#define UEFI_ROM_H

#include "uefi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct UEFI_RomFile;

/* Open a ROM file. filename is a CHAR16 (UCS-2) path relative to the volume. */
EFI_STATUS UEFI_RomFile_open(struct UEFI_RomFile **out,
                              EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Sfsp,
                              CHAR16 *filename);

/* Wrap an already-opened EFI_FILE_PROTOCOL in a UEFI_RomFile. */
EFI_STATUS UEFI_RomFile_from_fp(struct UEFI_RomFile **out,
                                 EFI_FILE_PROTOCOL *File);

/* Create a ROM source from an in-memory buffer (embedded ROM). */
EFI_STATUS UEFI_RomFile_from_memory(struct UEFI_RomFile **out,
                                     const unsigned char *data, size_t size);

EFI_STATUS UEFI_RomFile_close(struct UEFI_RomFile *f);

/* Callback-compatible functions for SMB_callbacks.read_rom_bytes / seek_rom.
 * userdata must point to a struct UEFI_RomFile*. */
bool UEFI_RomFile_read(void *userdata, unsigned char *buf, size_t size);
bool UEFI_RomFile_seek(void *userdata, size_t offset);

#ifdef __cplusplus
}
#endif

#endif /* UEFI_ROM_H */
