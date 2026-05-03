/*
 * uefi_input.h — Keyboard input for the SMB UEFI port.
 * Reads keystrokes via EFI_SIMPLE_TEXT_INPUT_PROTOCOL and maps them to
 * NES controller buttons.
 */
#ifndef UEFI_INPUT_H
#define UEFI_INPUT_H

#include "uefi_types.h"
#include "mario.h"

#ifdef __cplusplus
extern "C" {
#endif

struct UEFI_Input;

size_t UEFI_Input_size(void);
bool   UEFI_Input_init(struct UEFI_Input *in,
                        EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn);

/* Poll keyboard state. Returns true if ESC was pressed (exit signal). */
bool   UEFI_Input_poll(struct UEFI_Input *in);

/* Fill the SMB_buttons struct with the current button state for player 1. */
void   UEFI_Input_get_buttons(struct UEFI_Input *in,
                               struct SMB_buttons *buttons);

/* Check if a key was pressed since last poll (for single-shot actions). */
bool   UEFI_Input_key_pressed(struct UEFI_Input *in, UINT16 ScanCode);

#ifdef __cplusplus
}
#endif

#endif /* UEFI_INPUT_H */
