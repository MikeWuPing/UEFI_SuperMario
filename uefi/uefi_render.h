/*
 * uefi_render.h — GOP software renderer with double buffering.
 */
#ifndef UEFI_RENDER_H
#define UEFI_RENDER_H

#include "uefi_types.h"
#include "mario.h"

#ifdef __cplusplus
extern "C" {
#endif

struct UEFI_Renderer;

size_t UEFI_Renderer_size(void);
bool   UEFI_Renderer_init(struct UEFI_Renderer *r);
void   UEFI_Renderer_fini(struct UEFI_Renderer *r);

void   UEFI_Renderer_set_fb(struct UEFI_Renderer *r,
                             VOID *framebuffer,
                             UINT32 width, UINT32 height,
                             UINT32 pixels_per_scanline,
                             EFI_GRAPHICS_PIXEL_FORMAT format);

void   UEFI_Renderer_provide_palette_lookup(struct UEFI_Renderer *r,
                                             const unsigned char *rgb);
void   UEFI_Renderer_update_palette(struct UEFI_Renderer *r,
                                     const unsigned char *palette_indices);
void   UEFI_Renderer_update_pattern_tables(struct UEFI_Renderer *r,
                                            const unsigned char *chrrom);
void   UEFI_Renderer_draw_tile(struct UEFI_Renderer *r,
                                const struct SMB_tile tile);
void   UEFI_Renderer_clear(struct UEFI_Renderer *r);
void   UEFI_Renderer_fill_border(struct UEFI_Renderer *r);

/* Copy the 256×240 back buffer to the GOP framebuffer with scaling. */
void   UEFI_Renderer_flush(struct UEFI_Renderer *r);

#ifdef __cplusplus
}
#endif

#endif /* UEFI_RENDER_H */
