/*	$NetBSD: palette.c,v 1.1 2000/03/19 11:10:58 takemura Exp $	*/

/*-
 * Copyright (c) 1999 Shin Takemura.
 * All rights reserved.
 *
 * This software is part of the PocketBSD.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <pbsdboot.h>

/*
 * If Windows have no color palette, we have nothing to do here.
 */
#if ( _WIN32_WCE >= 200 ) // WIN CE Ver 2.00 or greater
#define HAVE_COLOR_PALETTE
#endif

#define ASSERT(a) \
	if (!(a)) { \
		msg_printf(MSG_ERROR, TEXT("assertion failure"), \
			TEXT(#a)); \
	}

#ifdef HAVE_COLOR_PALETTE
enum palette_status palette_succeeded = PAL_ERROR;
static void palette_setup(PALETTEENTRY *pal);
#else
enum palette_status palette_succeeded = PAL_NOERROR;
#endif

static u_char compo6[6] = {   0,  51, 102, 153, 204, 255 };
static u_char compo7[7] = {   0,  42,  85, 127, 170, 212, 255 };
static HPALETTE pal = NULL;

void
palette_init(HWND hWnd)
{
#ifdef HAVE_COLOR_PALETTE
	HDC hdc;
	PAINTSTRUCT ps;
	LOGPALETTE* logpal;

	debug_printf(TEXT("*** palette init ***\n"));
	
	hdc = BeginPaint(hWnd, &ps);
	if (GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASDISPLAY &&
	    (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) &&
	    GetDeviceCaps(hdc, COLORRES) == 8) {
		ASSERT(GetDeviceCaps(hdc, BITSPIXEL) == 8);
		ASSERT(GetDeviceCaps(hdc, PLANES) == 1);
		/*
		 * create logical palette
		 */
		logpal = LocalAlloc (LPTR,
				     (sizeof(LOGPALETTE) +
				      sizeof(PALETTEENTRY) * 256));
		logpal->palVersion = 0x300;
		logpal->palNumEntries = 256;
		palette_setup(logpal->palPalEntry);

		pal = CreatePalette(logpal);
		if (pal == NULL) {
			debug_printf(TEXT("CreatePalette() failed"));
			msg_printf(MSG_ERROR, TEXT("Error"),
				   TEXT("CreatePalette() failed: %d"),
				   GetLastError());
		}
		LocalFree((HLOCAL)logpal);
	} else {
		/*
		 * The screen is not 8 bit depth nor indexed color.
		 */
		palette_succeeded = PAL_NOERROR;
	}
	EndPaint(hWnd, &ps);
#endif /* HAVE_COLOR_PALETTE */
}

#ifdef HAVE_COLOR_PALETTE
static void
palette_setup(PALETTEENTRY *pal)
{
	int i, r, g, b;

	/*
	 * 0 - 31, gray scale
	 */
	for (i = 0; i < 32; i++) {
		pal[i].peFlags = PC_EXPLICIT;
		pal[i].peRed = i * 8;
		pal[i].peGreen = i * 8;
		pal[i].peBlue = i * 8;
	}
	/*
	 * 32 - 247, RGB color
	 */
	for (r = 0; r < 6; r++) {
		for (g = 0; g < 6; g++) {
			for (b = 0; b < 6; b++) {
				pal[i].peFlags = PC_EXPLICIT;
				pal[i].peRed = compo6[r];
				pal[i].peGreen = compo6[g];
				pal[i].peBlue = compo6[b];
				i++;
			}
		}
	}
	/*
	 * 248 - 255, just white
	 */
	for ( ; i < 256; i++) {
		pal[i].peFlags = PC_EXPLICIT;
		pal[i].peRed = 255;
		pal[i].peGreen = 255;
		pal[i].peBlue = 255;
	}
}
#endif /* HAVE_COLOR_PALETTE */

void
palette_set(HWND hWnd)
{
#ifdef HAVE_COLOR_PALETTE
	int n;
	HDC hdc;
	PAINTSTRUCT ps;
	HPALETTE prev_pal;

	debug_printf(TEXT("*** palette set ***\n"));

	if (pal != NULL) {
		hdc = BeginPaint(hWnd, &ps);
		prev_pal = SelectPalette(hdc, pal, FALSE);
		if (prev_pal == NULL) {
			debug_printf(TEXT("SelectPalette() failed"));
			msg_printf(MSG_ERROR, TEXT("Error"),
			    TEXT("SelectPalette() failed: %d"),
			    GetLastError());
		} else {
			n = RealizePalette(hdc);
			debug_printf(TEXT("RealizePalette() = %d\n"), n);
			if (n == GDI_ERROR) {
				debug_printf(TEXT("RealizePalette() failed"));
				msg_printf(MSG_ERROR, TEXT("Error"),
				    TEXT("RealizePalette() failed: %d"),
				    GetLastError());
			}
		}
		EndPaint(hWnd, &ps);
	}
#endif /* HAVE_COLOR_PALETTE */
}

void
palette_check(HWND hWnd)
{
#ifdef HAVE_COLOR_PALETTE
	HDC hdc;
	PAINTSTRUCT ps;
	PALETTEENTRY syspal[256];
	PALETTEENTRY canonpal[256];
	int i, n, error;

	debug_printf(TEXT("*** palette check ***\n"));

	error = 0;
	if (pal != NULL) {
		hdc = BeginPaint(hWnd, &ps);

		palette_setup(canonpal);

		n = GetSystemPaletteEntries(hdc, 0, 256, syspal);
		if (n == 0) {
			msg_printf(MSG_ERROR, TEXT("Error"),
			    TEXT("GetSystemPaletteEntries() failed: %d"),
			    GetLastError());
			error++;
		} else {
			for (i = 0; i < n; i++) {
				debug_printf(TEXT("%3d: %02x %02x %02x"),
				    i,
				    syspal[i].peRed,
				    syspal[i].peGreen,
				    syspal[i].peBlue);
				if (syspal[i].peRed != canonpal[i].peRed ||
				    syspal[i].peGreen != canonpal[i].peGreen ||
				    syspal[i].peBlue != canonpal[i].peBlue ) {
					debug_printf(TEXT(" *"));
					error++;
				}
				debug_printf(TEXT("\n"));
			}			
		}
		EndPaint(hWnd, &ps);
		if (error) {
			palette_succeeded = PAL_ERROR;
		} else {
			palette_succeeded = PAL_SUCCEEDED;
		}
	}
#endif /* HAVE_COLOR_PALETTE */
}
