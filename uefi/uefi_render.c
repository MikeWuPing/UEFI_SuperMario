/*
 * uefi_render.c — GOP software renderer with double buffering.
 *
 * All tile drawing goes to a 256×240 back buffer. After the frame is
 * complete, flush() scale-copies the back buffer to the GOP framebuffer
 * in one pass, eliminating flickering/tearing.
 */

#include "uefi_render.h"
#include "uefi_compat.h"

struct Tile {
  unsigned char val[8][8];
};

struct UEFI_Renderer {
  /* GOP framebuffer */
  unsigned char *fb;
  UINT32  fb_width, fb_height, fb_scanline;
  EFI_GRAPHICS_PIXEL_FORMAT pixel_format;
  UINT32  bytes_per_pixel;
  int     scale, off_x, off_y;

  /* 256×240 back buffer (NES-pixel-indexed, then flushed to FB) */
  unsigned char *back;
  int     back_stride;   /* 256 pixels * 4 bytes = 1024 */

  unsigned char  palette_indices[32];
  unsigned char  palette_rgb[0x40][3];
  struct Tile    tiles[0x200];
};

size_t UEFI_Renderer_size(void) { return sizeof(struct UEFI_Renderer); }

bool UEFI_Renderer_init(struct UEFI_Renderer *r) {
  memset(r, 0, sizeof(struct UEFI_Renderer));
  return true;
}

void UEFI_Renderer_fini(struct UEFI_Renderer *r) {
  if (r->back) {
    gBS->FreePool(r->back);
    r->back = NULL;
  }
}

void UEFI_Renderer_set_fb(struct UEFI_Renderer *r,
                           VOID *framebuffer,
                           UINT32 width, UINT32 height,
                           UINT32 pixels_per_scanline,
                           EFI_GRAPHICS_PIXEL_FORMAT format) {
  r->fb          = (unsigned char *)framebuffer;
  r->fb_width    = width;
  r->fb_height   = height;
  r->fb_scanline = pixels_per_scanline;
  r->pixel_format = format;
  r->bytes_per_pixel = 4;  /* BGRA or RGBA both use 4 bytes */

  r->scale = (int)(width / 256);
  int scaley = (int)(height / 240);
  if (scaley < r->scale) r->scale = scaley;
  if (r->scale < 1) r->scale = 1;

  r->off_x = ((int)width  - 256 * r->scale) / 2;
  r->off_y = ((int)height - 240 * r->scale) / 2;
  r->back_stride = 256 * 4;

  /* Allocate back buffer: 256 × 240 × 4 bytes = 245,760 bytes */
  gBS->AllocatePool(EfiBootServicesData, 256 * 240 * 4, (VOID **)&r->back);
}

void UEFI_Renderer_provide_palette_lookup(struct UEFI_Renderer *r,
                                           const unsigned char *rgb) {
  memcpy(r->palette_rgb, rgb, sizeof(r->palette_rgb));
}

void UEFI_Renderer_update_palette(struct UEFI_Renderer *r,
                                   const unsigned char *palette_indices) {
  memcpy(r->palette_indices, palette_indices, 32);
}

void UEFI_Renderer_update_pattern_tables(struct UEFI_Renderer *r,
                                          const unsigned char *chrrom) {
  for (int ti = 0; ti < 512; ti++) {
    struct Tile *t = &r->tiles[ti];
    const unsigned char *buf = chrrom + ti * 0x10;
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        unsigned char hi = (buf[row + 8] >> (7 - col)) & 1;
        unsigned char lo = (buf[row]     >> (7 - col)) & 1;
        t->val[row][col] = (hi << 1) | lo;
      }
    }
  }
}

/* Write a single NES pixel to the back buffer. */
static inline void back_pixel(struct UEFI_Renderer *r, int x, int y,
                               unsigned char red, unsigned char green, unsigned char blue) {
  if (x < 0 || x >= 256 || y < 0 || y >= 240) return;
  unsigned char *p = r->back + (y * r->back_stride + x * 4);
  if (r->pixel_format == PixelBlueGreenRedReserved8BitPerColor) {
    p[0] = blue; p[1] = green; p[2] = red;
  } else {
    p[0] = red; p[1] = green; p[2] = blue;
  }
}

void UEFI_Renderer_draw_tile(struct UEFI_Renderer *r,
                              const struct SMB_tile tile) {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      int px = tile.x + col;
      int py = tile.y + row;
      if (tile.flip_horz) px = tile.x + 7 - col;
      if (tile.flip_vert) py = tile.y + 7 - row;
      if (px < 0 || px >= 256 || py < 0 || py >= 240) continue;

      /* Tile data indexed by ORIGINAL row/col (before flip);
       * screen position uses flipped px/py. Matches NES PPU behavior. */
      unsigned char ci = r->tiles[tile.tileidx].val[row][col];
      if (ci == 0) continue;

      unsigned char pi = r->palette_indices[tile.paletteidx * 4 + ci] & 0x3F;
      unsigned char *rgb = r->palette_rgb[pi];
      back_pixel(r, px, py, rgb[0], rgb[1], rgb[2]);
    }
  }
}

void UEFI_Renderer_clear(struct UEFI_Renderer *r) {
  if (!r->back) return;
  unsigned char *rgb = r->palette_rgb[r->palette_indices[0]];
  unsigned char rv = rgb[0], gv = rgb[1], bv = rgb[2];

  unsigned char pix[4];
  if (r->pixel_format == PixelBlueGreenRedReserved8BitPerColor) {
    pix[0] = bv; pix[1] = gv; pix[2] = rv;
  } else {
    pix[0] = rv; pix[1] = gv; pix[2] = bv;
  }

  /* Fill each row */
  for (int y = 0; y < 240; y++) {
    unsigned char *row = r->back + y * r->back_stride;
    for (int x = 0; x < 256; x++) {
      unsigned char *p = row + x * 4;
      p[0] = pix[0]; p[1] = pix[1]; p[2] = pix[2];
    }
  }
}

void UEFI_Renderer_fill_border(struct UEFI_Renderer *r) {
  /* Fill entire GOP framebuffer black */
  UINT32 total = r->fb_scanline * r->fb_height;
  unsigned char *p = r->fb;
  for (UINT32 i = 0; i < total; i++) {
    p[0] = 0; p[1] = 0; p[2] = 0;
    if (r->bytes_per_pixel == 4) p[3] = 0;
    p += r->bytes_per_pixel;
  }
}

void UEFI_Renderer_flush(struct UEFI_Renderer *r) {
  if (!r->back || !r->fb) return;

  int s = r->scale;

  for (int y = 0; y < 240; y++) {
    const unsigned char *src = r->back + y * r->back_stride;
    int base_y = r->off_y + y * s;

    for (int x = 0; x < 256; x++) {
      unsigned char rv = src[2];   /* back buffer is BGR(A) */
      unsigned char gv = src[1];
      unsigned char bv = src[0];
      src += 4;

      /* Write scaled block to framebuffer */
      for (int dy = 0; dy < s; dy++) {
        int sy = base_y + dy;
        if (sy < 0 || (UINT32)sy >= r->fb_height) continue;
        unsigned char *dst = r->fb + (sy * r->fb_scanline + r->off_x + x * s) * r->bytes_per_pixel;

        for (int dx = 0; dx < s; dx++) {
          int sx = r->off_x + x * s + dx;
          if (sx < 0 || (UINT32)sx >= r->fb_width) continue;
          unsigned char *d = r->fb + (sy * r->fb_scanline + sx) * r->bytes_per_pixel;

          if (r->pixel_format == PixelBlueGreenRedReserved8BitPerColor) {
            d[0] = bv; d[1] = gv; d[2] = rv;
          } else {
            d[0] = rv; d[1] = gv; d[2] = bv;
          }
        }
      }
    }
  }
}
