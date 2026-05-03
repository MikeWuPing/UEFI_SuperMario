/*
 * uefi_rom.c — ROM file I/O implementation.
 *
 * Opens an NES/FDS ROM file from the UEFI filesystem using
 * EFI_SIMPLE_FILE_SYSTEM_PROTOCOL → EFI_FILE_PROTOCOL.
 *
 * The SMB core library calls read_rom_bytes / seek_rom callbacks which
 * we wire up to EFI_FILE_PROTOCOL Read / SetPosition.
 */

#include "uefi_rom.h"
#include "uefi_compat.h"
#include "uefi_debug.h"

struct UEFI_RomFile {
  EFI_FILE_PROTOCOL *File;       /* file-based ROM */
  const unsigned char *data;     /* memory-based ROM */
  size_t data_size;
  size_t position;
};

EFI_STATUS UEFI_RomFile_open(struct UEFI_RomFile **out,
                              EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Sfsp,
                              CHAR16 *filename) {
  EFI_STATUS status;
  EFI_FILE_PROTOCOL *Root;
  EFI_FILE_PROTOCOL *File;

  *out = NULL;

  debug_print("  OpenVolume...\n");
  status = Sfsp->OpenVolume(Sfsp, &Root);
  debug_print_efi_status("  OpenVolume", status);
  if (status != EFI_SUCCESS) {
    return status;
  }

  debug_print("  Root->Open(");
  /* Print ASCII approximation of filename */
  for (int i = 0; filename[i] && i < 40; i++) {
    debug_putc((char)filename[i]);
  }
  debug_print(")...\n");
  status = Root->Open(Root, &File, filename,
                       EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
  debug_print_efi_status("  Root->Open", status);
  if (status != EFI_SUCCESS) {
    return status;
  }

  struct UEFI_RomFile *f;
  status = gBS->AllocatePool(EfiBootServicesData, sizeof(*f), (VOID **)&f);
  if (status != EFI_SUCCESS) {
    File->Close(File);
    return status;
  }

  f->File = File;
  *out = f;
  return EFI_SUCCESS;
}

EFI_STATUS UEFI_RomFile_from_fp(struct UEFI_RomFile **out,
                                  EFI_FILE_PROTOCOL *File) {
  struct UEFI_RomFile *f;
  EFI_STATUS status = gBS->AllocatePool(EfiBootServicesData, sizeof(*f),
                                         (VOID **)&f);
  if (EFI_ERROR(status)) {
    *out = NULL;
    return status;
  }
  f->File = File;
  *out = f;
  return EFI_SUCCESS;
}

EFI_STATUS UEFI_RomFile_close(struct UEFI_RomFile *f) {
  EFI_STATUS status = f->File->Close(f->File);
  gBS->FreePool(f);
  return status;
}

bool UEFI_RomFile_read(void *userdata, unsigned char *buf, size_t size) {
  struct UEFI_RomFile *f = (struct UEFI_RomFile *)userdata;
  if (f->File) {
    UINTN read_size = size;
    EFI_STATUS status = f->File->Read(f->File, &read_size, buf);
    return (status == EFI_SUCCESS && read_size == size);
  }
  if (f->data) {
    if (f->position + size > f->data_size) return false;
    memcpy(buf, f->data + f->position, size);
    f->position += size;
    return true;
  }
  return false;
}

bool UEFI_RomFile_seek(void *userdata, size_t offset) {
  struct UEFI_RomFile *f = (struct UEFI_RomFile *)userdata;
  if (f->File) {
    EFI_STATUS status = f->File->SetPosition(f->File, (UINT64)offset);
    return status == EFI_SUCCESS;
  }
  if (f->data) {
    if (offset > f->data_size) return false;
    f->position = offset;
    return true;
  }
  return false;
}

EFI_STATUS UEFI_RomFile_from_memory(struct UEFI_RomFile **out,
                                     const unsigned char *data, size_t size) {
  struct UEFI_RomFile *f;
  EFI_STATUS status = gBS->AllocatePool(EfiBootServicesData, sizeof(*f),
                                         (VOID **)&f);
  if (EFI_ERROR(status)) {
    *out = NULL;
    return status;
  }
  f->File = NULL;
  f->data = data;
  f->data_size = size;
  f->position = 0;
  *out = f;
  return EFI_SUCCESS;
}
