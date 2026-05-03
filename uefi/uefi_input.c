/*
 * uefi_input.c — Timer-based non-blocking keyboard input.
 *
 * Uses a periodic timer + WaitForEvent to check for keys without blocking.
 * If a key is available (WaitForKey event fires), reads it.
 * If the timer fires first (no key), returns immediately so the game loop
 * keeps running at 60fps. This catches ALL key types — scan codes AND
 * Unicode characters (Z, X, Space, Enter, etc.).
 */

#include "uefi_input.h"
#include "uefi_compat.h"
#include "uefi_debug.h"

enum { KEY_U, KEY_D, KEY_L, KEY_R, KEY_A, KEY_B, KEY_SELECT, KEY_START, KEY_COUNT };

#define SCAN_UP    0x01
#define SCAN_DOWN  0x02
#define SCAN_RIGHT 0x03
#define SCAN_LEFT  0x04
#define SCAN_ESC   0x17

#define HOLD_FRAMES 6

struct UEFI_Input {
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
  EFI_EVENT  TimerEvent;      /* fires every frame_period */
  EFI_EVENT  WaitEvents[2];   /* [0]=TimerEvent, [1]=ConIn->WaitForKey */
  int        key_hold[KEY_COUNT];
  bool       esc_pressed;
  int        keys_read;
};

size_t UEFI_Input_size(void) { return sizeof(struct UEFI_Input); }

bool UEFI_Input_init(struct UEFI_Input *in, EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn) {
  memset(in, 0, sizeof(struct UEFI_Input));
  in->ConIn = ConIn;

  /* Create a periodic timer that fires every 16ms (~60 Hz).
   * Timer period is in 100ns units: 16ms = 160,000 × 100ns */
  EFI_STATUS status = gBS->CreateEvent(EVT_TIMER, TPL_NOTIFY, NULL, NULL,
                                        &in->TimerEvent);
  if (EFI_ERROR(status)) return false;

  status = gBS->SetTimer(in->TimerEvent, TimerPeriodic, 160000ULL);
  if (EFI_ERROR(status)) return false;

  /* WaitEvents array: keyboard FIRST so it wins when both are signaled */
  in->WaitEvents[0] = in->ConIn->WaitForKey;
  in->WaitEvents[1] = in->TimerEvent;

  return true;
}

bool UEFI_Input_poll(struct UEFI_Input *in) {
  /* Wait for either the timer OR a key press, whichever comes first.
   * Timer fires at ~60 Hz so the game loop never blocks for long. */
  UINTN index = 0;
  EFI_STATUS status = gBS->WaitForEvent(2, in->WaitEvents, &index);

  if (EFI_ERROR(status)) {
    /* Timeout or error — no key available */
    return in->esc_pressed;
  }

  if (index == 1) {
    /* Timer fired — no key available, continue game loop */
    return in->esc_pressed;
  }

  /* Keyboard event (index == 0): read ALL available keys */
  do {
    EFI_INPUT_KEY key;
    status = in->ConIn->ReadKeyStroke(in->ConIn, &key);
    if (EFI_ERROR(status)) break;

    in->keys_read++;
    if (in->keys_read <= 30) {
      debug_print("Key: sc=");
      debug_print_hex(key.ScanCode);
      debug_print(" uc=");
      debug_print_hex(key.UnicodeChar);
      debug_print("\n");
    }

    switch (key.ScanCode) {
    case SCAN_UP:    in->key_hold[KEY_U] = HOLD_FRAMES; break;
    case SCAN_DOWN:  in->key_hold[KEY_D] = HOLD_FRAMES; break;
    case SCAN_LEFT:  in->key_hold[KEY_L] = HOLD_FRAMES; break;
    case SCAN_RIGHT: in->key_hold[KEY_R] = HOLD_FRAMES; break;
    case SCAN_ESC:   in->esc_pressed = true; break;
    }

    switch (key.UnicodeChar) {
    /* Action buttons: try multiple mappings for QEMU compatibility */
    case L'1': case L'/': case L'?': in->key_hold[KEY_A] = HOLD_FRAMES; break;
    case L'2': case L'.': case L'>': in->key_hold[KEY_B] = HOLD_FRAMES; break;
    case L'\r': case L'\n': in->key_hold[KEY_START] = HOLD_FRAMES; break;
    case L' ':  in->key_hold[KEY_SELECT] = HOLD_FRAMES; break;
    }

    /* Check if more keys are queued (non-blocking) */
    status = gBS->CheckEvent(in->ConIn->WaitForKey);
  } while (status == EFI_SUCCESS);

  return in->esc_pressed;
}

void UEFI_Input_get_buttons(struct UEFI_Input *in, struct SMB_buttons *buttons) {
  buttons->u      = in->key_hold[KEY_U] > 0;
  buttons->d      = in->key_hold[KEY_D] > 0;
  buttons->l      = in->key_hold[KEY_L] > 0;
  buttons->r      = in->key_hold[KEY_R] > 0;
  buttons->a      = in->key_hold[KEY_A] > 0;
  buttons->b      = in->key_hold[KEY_B] > 0;
  buttons->select = in->key_hold[KEY_SELECT] > 0;
  buttons->start  = in->key_hold[KEY_START] > 0;

  for (int i = 0; i < KEY_COUNT; i++) {
    if (in->key_hold[i] > 0) in->key_hold[i]--;
  }
}

bool UEFI_Input_key_pressed(struct UEFI_Input *in, UINT16 ScanCode) {
  (void)in; (void)ScanCode; return false;
}
