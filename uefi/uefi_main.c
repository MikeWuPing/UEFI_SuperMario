/*
 * uefi_main.c — UEFI Shell entry point for SMB with embedded ROM.
 */

#include "uefi_types.h"
#include "uefi_render.h"
#include "uefi_input.h"
#include "uefi_rom.h"
#include "uefi_compat.h"
#include "uefi_debug.h"

#include "mario.h"

/* ── Global UEFI state ────────────────────────────────────────────────── */

EFI_BOOT_SERVICES *gBS = NULL;
EFI_SYSTEM_TABLE  *gST = NULL;
EFI_HANDLE         gImageHandle = NULL;

/* ── Embedded ROM (generated from smb.nes) ─────────────────────────────── */

extern const unsigned char embedded_rom[];
extern const size_t embedded_rom_size;

/* ── Embedded NES palette ──────────────────────────────────────────────── */

static const unsigned char nes_palette[0x40][3] = {
  {0x7C,0x7C,0x7C}, {0x00,0x00,0xFC}, {0x00,0x00,0xBC}, {0x44,0x28,0xBC},
  {0x94,0x00,0x84}, {0xA8,0x10,0x20}, {0xA8,0x10,0x14}, {0x88,0x24,0x00},
  {0x50,0x30,0x00}, {0x00,0x78,0x00}, {0x00,0x68,0x00}, {0x00,0x58,0x00},
  {0x00,0x40,0x58}, {0x00,0x00,0x00}, {0x00,0x00,0x00}, {0x00,0x00,0x00},
  {0xBC,0xBC,0xBC}, {0x00,0x78,0xF8}, {0x00,0x58,0xF8}, {0x68,0x44,0xFC},
  {0xD8,0x00,0xCC}, {0xE4,0x00,0x58}, {0xF8,0x38,0x00}, {0xE4,0x5C,0x10},
  {0xAC,0x7C,0x00}, {0x00,0xB8,0x00}, {0x00,0xA8,0x00}, {0x00,0xA8,0x44},
  {0x00,0x88,0x88}, {0x00,0x00,0x00}, {0x00,0x00,0x00}, {0x00,0x00,0x00},
  {0xF8,0xF8,0xF8}, {0x3C,0xBC,0xFC}, {0x68,0x88,0xFC}, {0x98,0x78,0xF8},
  {0xF8,0x78,0xF8}, {0xF8,0x58,0x98}, {0xF8,0x78,0x58}, {0xFC,0xA0,0x44},
  {0xF8,0xB8,0x00}, {0xB8,0xF8,0x18}, {0x58,0xD8,0x54}, {0x58,0xF8,0x98},
  {0x00,0xE8,0xD8}, {0x78,0x78,0x78}, {0x00,0x00,0x00}, {0x00,0x00,0x00},
  {0xFC,0xFC,0xFC}, {0xA4,0xE4,0xFC}, {0xB8,0xB8,0xF8}, {0xD8,0xB8,0xF8},
  {0xF8,0xB8,0xF8}, {0xF8,0xA4,0xC0}, {0xF0,0xD0,0xB0}, {0xFC,0xE0,0xA8},
  {0xF8,0xD8,0x78}, {0xD8,0xF8,0x78}, {0xB8,0xF8,0xB8}, {0xB8,0xF8,0xD8},
  {0x00,0xFC,0xFC}, {0xF8,0xD8,0xF8}, {0x00,0x00,0x00}, {0x00,0x00,0x00},
};

/* ── Frontend state ────────────────────────────────────────────────────── */

struct Frontend {
  struct UEFI_Renderer *renderer;
  struct UEFI_Input    *input;
  struct UEFI_RomFile  *romfile;
  struct SMB_state     *smb_state;
  struct SMB_buttons    joy_buttons;
  bool                  running;
};

/* ── Callback implementations ──────────────────────────────────────────── */

static bool cb_read_rom_bytes(void *userdata, unsigned char *buf, size_t size) {
  struct Frontend *fe = (struct Frontend *)userdata;
  return UEFI_RomFile_read(fe->romfile, buf, size);
}
static bool cb_seek_rom(void *userdata, size_t offset) {
  struct Frontend *fe = (struct Frontend *)userdata;
  return UEFI_RomFile_seek(fe->romfile, offset);
}
static void cb_update_pattern_tables(void *userdata, const unsigned char *chrrom) {
  struct Frontend *fe = (struct Frontend *)userdata;
  UEFI_Renderer_update_pattern_tables(fe->renderer, chrrom);
}
static void cb_update_palette(void *userdata, const unsigned char *pal) {
  struct Frontend *fe = (struct Frontend *)userdata;
  UEFI_Renderer_update_palette(fe->renderer, pal);
}
static void cb_draw_tile(void *userdata, const struct SMB_tile tile) {
  struct Frontend *fe = (struct Frontend *)userdata;
  UEFI_Renderer_draw_tile(fe->renderer, tile);
}
static void cb_joy1(void *userdata, struct SMB_buttons *buttons) {
  struct Frontend *fe = (struct Frontend *)userdata;
  *buttons = fe->joy_buttons;
}
static void cb_joy2(void *userdata, struct SMB_buttons *buttons) {
  (void)userdata;
  memset(buttons, 0, sizeof(*buttons));
}
static void cb_apu_write_register(void *userdata, unsigned short addr, unsigned char data) {
  (void)userdata; (void)addr; (void)data;
}
static void cb_apu_end_frame(void *userdata) {
  (void)userdata;
}
static unsigned char cb_smb2j_load_games_beaten(void *userdata) {
  (void)userdata; return 0;
}
static bool cb_smb2j_save_games_beaten(void *userdata, unsigned char gb) {
  (void)userdata; (void)gb; return true;
}

/* ── Entry point ───────────────────────────────────────────────────────── */

EFI_STATUS EFIAPI uefi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS                status;
  struct Frontend          *fe = NULL;
  struct SMB_callbacks      callbacks;
  EFI_GRAPHICS_OUTPUT_PROTOCOL    *Gop = NULL;

  debug_init();
  debug_print("\n\n=== SMB UEFI Port (embedded ROM) ===\n");

  gBS = (EFI_BOOT_SERVICES *)SystemTable->BootServices;
  gST = SystemTable;
  gImageHandle = ImageHandle;

  debug_print("SystemTable: ");
  debug_print_hex((UINTN)SystemTable);
  debug_print("\nROM size: ");
  debug_print_u64(embedded_rom_size);
  debug_print(" bytes\n");

  /* ── Locate GOP ──────────────────────────────────────────────────────── */
  static EFI_GUID GopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
  {
    UINTN num = 0; EFI_HANDLE *h = NULL;
    status = gBS->LocateHandleBuffer(ByProtocol, &GopGuid, NULL, &num, &h);
    debug_print_efi_status("Locate GOP", status);
    if (!EFI_ERROR(status) && num > 0) {
      status = gBS->HandleProtocol(h[0], &GopGuid, (VOID **)&Gop);
      gBS->FreePool(h);
    }
  }
  debug_print_efi_status("GOP handle", status);
  if (EFI_ERROR(status)) goto exit;

  if (Gop->Mode && Gop->Mode->Info) {
    debug_print("GOP: ");
    debug_print_u64(Gop->Mode->Info->HorizontalResolution);
    debug_print("x");
    debug_print_u64(Gop->Mode->Info->VerticalResolution);
    debug_print(" fmt=");
    debug_print_u64(Gop->Mode->Info->PixelFormat);
    debug_print("\n");
  }

  /* ── Allocate frontend ───────────────────────────────────────────────── */
  status = gBS->AllocatePool(EfiBootServicesData, sizeof(*fe), (VOID **)&fe);
  if (EFI_ERROR(status)) goto exit;
  memset(fe, 0, sizeof(*fe));
  fe->running = true;

  /* ── Create ROM from embedded data ──────────────────────────────────── */
  status = UEFI_RomFile_from_memory(&fe->romfile,
                                     embedded_rom, embedded_rom_size);
  debug_print_efi_status("Embedded ROM", status);
  if (EFI_ERROR(status)) goto exit;

  /* ── Initialize renderer ─────────────────────────────────────────────── */
  status = gBS->AllocatePool(EfiBootServicesData, UEFI_Renderer_size(),
                              (VOID **)&fe->renderer);
  if (EFI_ERROR(status)) goto exit;
  UEFI_Renderer_init(fe->renderer);

  {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode = Gop->Mode;
    UEFI_Renderer_set_fb(fe->renderer,
                          (VOID *)(UINTN)Mode->FrameBufferBase,
                          Mode->Info->HorizontalResolution,
                          Mode->Info->VerticalResolution,
                          Mode->Info->PixelsPerScanLine,
                          Mode->Info->PixelFormat);
    UEFI_Renderer_provide_palette_lookup(fe->renderer, &nes_palette[0][0]);
    UEFI_Renderer_fill_border(fe->renderer);
  }
  debug_print("Renderer ready\n");

  /* ── Initialize input ────────────────────────────────────────────────── */
  status = gBS->AllocatePool(EfiBootServicesData, UEFI_Input_size(),
                              (VOID **)&fe->input);
  if (EFI_ERROR(status)) goto exit;
  UEFI_Input_init(fe->input, gST->ConIn);
  debug_print("Input ready\n");

  /* ── Initialize SMB state ────────────────────────────────────────────── */
  status = gBS->AllocatePool(EfiBootServicesData, SMB_state_size(),
                              (VOID **)&fe->smb_state);
  if (EFI_ERROR(status)) goto exit;

  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.userdata                = fe;
  callbacks.read_rom_bytes          = cb_read_rom_bytes;
  callbacks.seek_rom                = cb_seek_rom;
  callbacks.update_pattern_tables   = cb_update_pattern_tables;
  callbacks.update_palette          = cb_update_palette;
  callbacks.draw_tile               = cb_draw_tile;
  callbacks.joy1                    = cb_joy1;
  callbacks.joy2                    = cb_joy2;
  callbacks.apu_write_register      = cb_apu_write_register;
  callbacks.apu_end_frame           = cb_apu_end_frame;
  callbacks.smb2j_load_games_beaten = cb_smb2j_load_games_beaten;
  callbacks.smb2j_save_games_beaten = cb_smb2j_save_games_beaten;

  debug_print("Calling SMB_state_init...\n");
  if (!SMB_state_init(fe->smb_state, &callbacks)) {
    debug_print("SMB_state_init FAILED\n");
    goto exit;
  }
  debug_print("SMB_state_init OK (game=");
  debug_print_u64(SMB_which_game(fe->smb_state));
  debug_print(")\n=== Entering game loop ===\n");

  /* ── Game loop ───────────────────────────────────────────────────────── */
  {
    UINTN fc = 0;
    while (fe->running) {
      /* Input poll uses WaitForEvent with a ~16ms timer — also
       * serves as frame pacing (no separate Stall needed). */
      if (UEFI_Input_poll(fe->input)) break;
      UEFI_Input_get_buttons(fe->input, &fe->joy_buttons);
      UEFI_Renderer_clear(fe->renderer);
      SMB_tick(fe->smb_state);
      UEFI_Renderer_flush(fe->renderer);
      fc++;
      if (fc == 1) debug_print("First frame rendered\n");
      if ((fc % 60) == 0) {
        debug_print("Frame ");
        debug_print_u64(fc);
        debug_print("\n");
      }
    }
    debug_print("Exited game loop (frame ");
    debug_print_u64(fc);
    debug_print(")\n");
  }

exit:
  debug_print("Cleanup...\n");
  if (fe) {
    if (fe->smb_state) gBS->FreePool(fe->smb_state);
    if (fe->input)      gBS->FreePool(fe->input);
    if (fe->renderer)   { UEFI_Renderer_fini(fe->renderer); gBS->FreePool(fe->renderer); }
    if (fe->romfile)    { UEFI_RomFile_close(fe->romfile); }
    gBS->FreePool(fe);
  }
  gST->ConOut->ClearScreen(gST->ConOut);
  debug_print("=== Done ===\n");
  return EFI_SUCCESS;
}
