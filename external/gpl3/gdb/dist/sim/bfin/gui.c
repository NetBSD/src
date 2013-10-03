/* Blackfin GUI (SDL) helper code

   Copyright (C) 2010-2013 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc.

   This file is part of simulators.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "config.h"

#ifdef HAVE_SDL
# include <SDL.h>
#endif
#ifdef HAVE_DLFCN_H
# include <dlfcn.h>
#endif

#include "libiberty.h"
#include "gui.h"

#ifdef HAVE_SDL

static struct {
  void *handle;
  int (*Init) (Uint32 flags);
  void (*Quit) (void);
  SDL_Surface *(*SetVideoMode) (int width, int height, int bpp, Uint32 flags);
  void (*WM_SetCaption) (const char *title, const char *icon);
  int (*ShowCursor) (int toggle);
  int (*LockSurface) (SDL_Surface *surface);
  void (*UnlockSurface) (SDL_Surface *surface);
  void (*GetRGB) (Uint32 pixel, const SDL_PixelFormat * const fmt, Uint8 *r, Uint8 *g, Uint8 *b);
  Uint32 (*MapRGB) (const SDL_PixelFormat * const format, const Uint8 r, const Uint8 g, const Uint8 b);
  void (*UpdateRect) (SDL_Surface *screen, Sint32 x, Sint32 y, Uint32 w, Uint32 h);
} sdl;

static const char * const sdl_syms[] =
{
  "SDL_Init",
  "SDL_Quit",
  "SDL_SetVideoMode",
  "SDL_WM_SetCaption",
  "SDL_ShowCursor",
  "SDL_LockSurface",
  "SDL_UnlockSurface",
  "SDL_GetRGB",
  "SDL_MapRGB",
  "SDL_UpdateRect",
};

struct gui_state {
  SDL_Surface *screen;
  const SDL_PixelFormat *format;
  int throttle, throttle_limit;
  enum gui_color color;
  int curr_line;
};

/* Load the SDL lib on the fly to avoid hard linking against it.  */
static int
bfin_gui_sdl_setup (void)
{
  int i;
  uintptr_t **funcs;

  if (sdl.handle)
    return 0;

  sdl.handle = dlopen ("libSDL-1.2.so.0", RTLD_LAZY);
  if (sdl.handle == NULL)
    return -1;

  funcs = (void *) &sdl.Init;
  for (i = 0; i < ARRAY_SIZE (sdl_syms); ++i)
    {
      funcs[i] = dlsym (sdl.handle, sdl_syms[i]);
      if (funcs[i] == NULL)
	{
	  dlclose (sdl.handle);
	  sdl.handle = NULL;
	  return -1;
	}
    }

  return 0;
}

static const SDL_PixelFormat *bfin_gui_color_format (enum gui_color color);

void *
bfin_gui_setup (void *state, int enabled, int width, int height,
		enum gui_color color)
{
  if (bfin_gui_sdl_setup ())
    return NULL;

  /* Create an SDL window if enabled and we don't have one yet.  */
  if (enabled && !state)
    {
      struct gui_state *gui = xmalloc (sizeof (*gui));
      if (!gui)
	return NULL;

      if (sdl.Init (SDL_INIT_VIDEO))
	goto error;

      gui->color = color;
      gui->format = bfin_gui_color_format (gui->color);
      gui->screen = sdl.SetVideoMode (width, height, 32,
				      SDL_ANYFORMAT|SDL_HWSURFACE);
      if (!gui->screen)
	{
	  sdl.Quit();
	  goto error;
	}

      sdl.WM_SetCaption ("GDB Blackfin Simulator", NULL);
      sdl.ShowCursor (0);
      gui->curr_line = 0;
      gui->throttle = 0;
      gui->throttle_limit = 0xf; /* XXX: let people control this ?  */
      return gui;

 error:
      free (gui);
      return NULL;
    }

  /* Else break down a window if disabled and we had one.  */
  else if (!enabled && state)
    {
      sdl.Quit();
      free (state);
      return NULL;
    }

  /* Retain existing state, whatever that may be.  */
  return state;
}

static int
SDL_ConvertBlitLineFrom (const Uint8 *src, const SDL_PixelFormat * const format,
			 SDL_Surface *dst, int dsty)
{
  Uint8 r, g, b;
  Uint32 *pixels;
  unsigned i, j;

  if (SDL_MUSTLOCK (dst))
    if (sdl.LockSurface (dst))
      return 1;

  pixels = dst->pixels;
  pixels += (dsty * dst->pitch / 4);

  for (i = 0; i < dst->w; ++i)
    {
      /* Exract the packed source pixel; RGB or BGR.  */
      Uint32 pix = 0;
      for (j = 0; j < format->BytesPerPixel; ++j)
	if (format->Rshift)
	  pix = (pix << 8) | src[j];
	else
	  pix = pix | ((Uint32)src[j] << (j * 8));

      /* Unpack the source pixel into its components.  */
      sdl.GetRGB (pix, format, &r, &g, &b);
      /* Translate into the screen pixel format.  */
      *pixels++ = sdl.MapRGB (dst->format, r, g, b);

      src += format->BytesPerPixel;
    }

  if (SDL_MUSTLOCK (dst))
    sdl.UnlockSurface (dst);

  sdl.UpdateRect (dst, 0, dsty, dst->w, 1);

  return 0;
}

unsigned
bfin_gui_update (void *state, const void *source, unsigned nr_bytes)
{
  struct gui_state *gui = state;
  int ret;

  if (!gui)
    return 0;

  /* XXX: Make this an option ?  */
  gui->throttle = (gui->throttle + 1) & gui->throttle_limit;
  if (gui->throttle)
    return 0;

  ret = SDL_ConvertBlitLineFrom (source, gui->format, gui->screen,
				 gui->curr_line);
  if (ret)
    return 0;

  gui->curr_line = (gui->curr_line + 1) % gui->screen->h;

  return nr_bytes;
}

#define FMASK(cnt, shift) (((1 << (cnt)) - 1) << (shift))
#define _FORMAT(bpp, rcnt, gcnt, bcnt, acnt, rsh, gsh, bsh, ash) \
  NULL, bpp, (bpp)/8, 8-(rcnt), 8-(gcnt), 8-(bcnt), 8-(acnt), rsh, gsh, bsh, ash, \
  FMASK (rcnt, rsh), FMASK (gcnt, gsh), FMASK (bcnt, bsh), FMASK (acnt, ash),
#define FORMAT(rcnt, gcnt, bcnt, acnt, rsh, gsh, bsh, ash) \
  _FORMAT(((((rcnt) + (gcnt) + (bcnt) + (acnt)) + 7) / 8) * 8, \
	  rcnt, gcnt, bcnt, acnt, rsh, gsh, bsh, ash)

static const SDL_PixelFormat sdl_rgb_565 =
{
  FORMAT (5, 6, 5, 0, 11, 5, 0, 0)
};
static const SDL_PixelFormat sdl_bgr_565 =
{
  FORMAT (5, 6, 5, 0, 0, 5, 11, 0)
};
static const SDL_PixelFormat sdl_rgb_888 =
{
  FORMAT (8, 8, 8, 0, 16, 8, 0, 0)
};
static const SDL_PixelFormat sdl_bgr_888 =
{
  FORMAT (8, 8, 8, 0, 0, 8, 16, 0)
};
static const SDL_PixelFormat sdl_rgba_8888 =
{
  FORMAT (8, 8, 8, 8, 24, 16, 8, 0)
};

static const struct {
  const char *name;
  const SDL_PixelFormat *format;
  enum gui_color color;
} color_spaces[] = {
  { "rgb565",   &sdl_rgb_565,   GUI_COLOR_RGB_565,   },
  { "bgr565",   &sdl_bgr_565,   GUI_COLOR_BGR_565,   },
  { "rgb888",   &sdl_rgb_888,   GUI_COLOR_RGB_888,   },
  { "bgr888",   &sdl_bgr_888,   GUI_COLOR_BGR_888,   },
  { "rgba8888", &sdl_rgba_8888, GUI_COLOR_RGBA_8888, },
};

enum gui_color bfin_gui_color (const char *color)
{
  int i;

  if (!color)
    goto def;

  for (i = 0; i < ARRAY_SIZE (color_spaces); ++i)
    if (!strcmp (color, color_spaces[i].name))
      return color_spaces[i].color;

  /* Pick a random default.  */
 def:
  return GUI_COLOR_RGB_888;
}

static const SDL_PixelFormat *bfin_gui_color_format (enum gui_color color)
{
  int i;

  for (i = 0; i < ARRAY_SIZE (color_spaces); ++i)
    if (color == color_spaces[i].color)
      return color_spaces[i].format;

  return NULL;
}

int bfin_gui_color_depth (enum gui_color color)
{
  const SDL_PixelFormat *format = bfin_gui_color_format (color);
  return format ? format->BitsPerPixel : 0;
}

#endif
