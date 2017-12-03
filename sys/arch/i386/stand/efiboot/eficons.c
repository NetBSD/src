/*	$NetBSD: eficons.c,v 1.4.8.2 2017/12/03 11:36:18 jdolecek Exp $	*/

/*-
 * Copyright (c) 2016 Kimihiro Nonaka <nonaka@netbsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 */

#include <sys/bitops.h>
#include <sys/stdint.h>

#include "efiboot.h"

#include "bootinfo.h"
#include "vbe.h"

extern struct x86_boot_params boot_params;

struct btinfo_console btinfo_console;

static EFI_GRAPHICS_OUTPUT_PROTOCOL *efi_gop;
static int efi_gop_mode = -1;

static CHAR16 keybuf[16];
static int keybuf_read = 0;
static int keybuf_write = 0;

static void eficons_init_video(void);
static void efi_switch_video_to_text_mode(void);

static int
getcomaddr(int idx)
{
	static const short comioport[4] = { 0x3f8, 0x2f8, 0x3e8, 0x2e8 };

	if (idx < __arraycount(comioport))
		return comioport[idx];
	return 0;
}

/*
 * XXX only pass console parameters to kernel.
 */
void
consinit(int dev, int ioport, int speed)
{
	int iodev;

#if defined(CONSPEED)
	btinfo_console.speed = CONSPEED;
#else
	btinfo_console.speed = 9600;
#endif

	switch (dev) {
	case CONSDEV_AUTO:
		/* XXX comport */
		goto nocom;

	case CONSDEV_COM0:
	case CONSDEV_COM1:
	case CONSDEV_COM2:
	case CONSDEV_COM3:
		iodev = dev;
comport:
		btinfo_console.addr = ioport;
		if (btinfo_console.addr == 0) {
			btinfo_console.addr = getcomaddr(iodev - CONSDEV_COM0);
			if (btinfo_console.addr == 0)
				goto nocom;
		}
		if (speed != 0)
			btinfo_console.speed = speed;
		break;

	case CONSDEV_COM0KBD:
	case CONSDEV_COM1KBD:
	case CONSDEV_COM2KBD:
	case CONSDEV_COM3KBD:
		iodev = dev - CONSDEV_COM0KBD + CONSDEV_COM0;
		goto comport;	/* XXX kbd */

	case CONSDEV_PC:
	default:
nocom:
		iodev = CONSDEV_PC;
		break;
	}

	strlcpy(btinfo_console.devname, iodev == CONSDEV_PC ? "pc" : "com", 16);
}

int
cninit(void)
{

	efi_switch_video_to_text_mode();
	eficons_init_video();

	consinit(boot_params.bp_consdev, boot_params.bp_consaddr,
	    boot_params.bp_conspeed);

	return 0;
}

int
getchar(void)
{
	EFI_STATUS status;
	EFI_INPUT_KEY key;
	int c;

	if (keybuf_read != keybuf_write) {
		c = keybuf[keybuf_read];
		keybuf_read = (keybuf_read + 1) % __arraycount(keybuf);
		return c;
	}

	status = uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn,
	    &key);
	while (status == EFI_NOT_READY) {
		WaitForSingleEvent(ST->ConIn->WaitForKey, 0);
		status = uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2,
		    ST->ConIn, &key);
	}
	return key.UnicodeChar;
}

void
putchar(int c)
{
	CHAR16 buf[2];

	buf[1] = 0;
	if (c == '\n') {
		buf[0] = '\r';
		Output(buf);
	}
	buf[0] = c;
	Output(buf);
}

/*ARGSUSED*/
int
iskey(int intr)
{
	EFI_STATUS status;
	EFI_INPUT_KEY key;

	if (keybuf_read != keybuf_write)
		return 1;

	status = uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn,
	    &key);
	if (EFI_ERROR(status))
		return 0;

	keybuf[keybuf_write] = key.UnicodeChar;
	keybuf_write = (keybuf_write + 1) % __arraycount(keybuf);
	return 1;
}

char
awaitkey(int timeout, int tell)
{
	char c = 0;

	for (;;) {
		if (tell) {
			char numbuf[32];
			int len;

			len = snprintf(numbuf, sizeof(numbuf), "%d seconds. ",
			    timeout);
			if (len > 0 && len < sizeof(numbuf)) {
				char *p = numbuf;

				printf("%s", numbuf);
				while (*p)
					*p++ = '\b';
				printf("%s", numbuf);
			}
		}
		if (iskey(1)) {
			/* flush input buffer */
			while (iskey(0))
				c = getchar();
			if (c == 0)
				c = -1;
			goto out;
		}
		if (timeout--)
			WaitForSingleEvent(ST->ConIn->WaitForKey, 10000000);
		else
			break;
	}

out:
	if (tell)
		printf("0 seconds.     \n");

	return c;
}

void
clear_pc_screen(void)
{

	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
}

static uint8_t
getdepth(const EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info)
{

	switch (info->PixelFormat) {
	case PixelBlueGreenRedReserved8BitPerColor:
	case PixelRedGreenBlueReserved8BitPerColor:
		return 32;

	case PixelBitMask:
		return fls32(info->PixelInformation.RedMask
		    | info->PixelInformation.GreenMask
		    | info->PixelInformation.BlueMask
		    | info->PixelInformation.ReservedMask);

	case PixelBltOnly:
	case PixelFormatMax:
		return 0;
	}
	return 0;
}

static void
setpixelformat(UINT32 mask, uint8_t *num, uint8_t *pos)
{
	uint8_t n, p;

	n = popcount32(mask);
	p = ffs32(mask);
	if (p > 0)
		p--;

	*num = n;
	*pos = p;
}

static void
bi_framebuffer(void)
{
	EFI_STATUS status;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
	struct btinfo_framebuffer fb;
	INT32 bestmode = -1;

	if (efi_gop == NULL) {
		framebuffer_configure(NULL);
		return;
	}

	if (efi_gop_mode >= 0) {
		bestmode = efi_gop_mode;
	} else {
#if 0
		UINT64 res, bestres = 0;
		UINTN sz;
		UINT32 i;

		/* XXX EDID? EFI_EDID_DISCOVERED_PROTOCOL */
		for (i = 0; i < efi_gop->Mode->MaxMode; i++) {
			status = uefi_call_wrapper(efi_gop->QueryMode, 4,
			    efi_gop, i, &sz, &info);
			if (EFI_ERROR(status))
				continue;

			res = (UINT64)info->HorizontalResolution *
			    (UINT64)info->VerticalResolution *
			    (UINT64)getdepth(info);
			if (res > bestres) {
				bestmode = i;
				bestres = res;
			}
		}
#endif
	}
	if (bestmode >= 0) {
		status = uefi_call_wrapper(efi_gop->SetMode, 2, efi_gop,
		    bestmode);
		if (EFI_ERROR(status) || efi_gop->Mode->Mode != bestmode)
			Print(L"GOP setmode failed: %r\n", status);
	}

	info = efi_gop->Mode->Info;
	memset(&fb, 0, sizeof(fb));
	fb.physaddr = efi_gop->Mode->FrameBufferBase;
	fb.flags = 0;
	fb.width = info->HorizontalResolution;
	fb.height = info->VerticalResolution;
	fb.depth = getdepth(info);
	fb.stride = info->PixelsPerScanLine * ((fb.depth + 7) / 8);
	fb.vbemode = 0;	/* XXX */

	switch (info->PixelFormat) {
	case PixelBlueGreenRedReserved8BitPerColor:
		fb.rnum = 8;
		fb.gnum = 8;
		fb.bnum = 8;
		fb.rpos = 16;
		fb.gpos = 8;
		fb.bpos = 0;
		break;

	case PixelRedGreenBlueReserved8BitPerColor:
		fb.rnum = 8;
		fb.gnum = 8;
		fb.bnum = 8;
		fb.rpos = 0;
		fb.gpos = 8;
		fb.bpos = 16;
		break;

	case PixelBitMask:
		setpixelformat(info->PixelInformation.RedMask,
		    &fb.rnum, &fb.rpos);
		setpixelformat(info->PixelInformation.GreenMask,
		    &fb.gnum, &fb.gpos);
		setpixelformat(info->PixelInformation.BlueMask,
		    &fb.bnum, &fb.bpos);
		break;

	case PixelBltOnly:
	case PixelFormatMax:
		Panic(L"Error: invalid pixel format (%d)", info->PixelFormat);
		break;
	}

	framebuffer_configure(&fb);
}

int
vbe_commit(void)
{

	bi_framebuffer();
	return 0;
}

static void
print_text_modes(void)
{
	EFI_STATUS status;
	UINTN cols, rows;
	INT32 i, curmode;

	curmode = ST->ConOut->Mode->Mode;
	for (i = 0; i < ST->ConOut->Mode->MaxMode; i++) {
		status = uefi_call_wrapper(ST->ConOut->QueryMode, 4,
		    ST->ConOut, i, &cols, &rows);
		if (EFI_ERROR(status))
			continue;
		Print(L"%c%d: %dx%d\n", i == curmode ? '*' : ' ',
		    i, cols, rows);
	}
}

static int
efi_find_text_mode(char *arg)
{
	EFI_STATUS status;
	UINTN cols, rows;
	INT32 i;
	char mode[32];

	for (i = 0; i < ST->ConOut->Mode->MaxMode; i++) {
		status = uefi_call_wrapper(ST->ConOut->QueryMode, 4,
		    ST->ConOut, i, &cols, &rows);
		if (EFI_ERROR(status))
			continue;
		snprintf(mode, sizeof(mode), "%lux%lu", (long)cols, (long)rows);
		if (strcmp(arg, mode) == 0)
			return i;
	}
	return -1;
}

void
command_text(char *arg)
{
	EFI_STATUS status;
	INT32 modenum;

	if (*arg == '\0' || strcmp(arg, "list") == 0) {
		print_text_modes();
		return;
	}

	if (strchr(arg, 'x') != NULL) {
		modenum = efi_find_text_mode(arg);
		if (modenum == -1) {
			printf("mode %s not supported by firmware\n", arg);
			return;
		}
	} else {
		modenum = strtoul(arg, NULL, 0);
	}

	status = uefi_call_wrapper(ST->ConOut->SetMode, 2, ST->ConOut, modenum);
	if (!EFI_ERROR(status))
		return;

	printf("invalid flag, must be 'list', a display mode, "
	    "or a mode number\n");
}

static int
print_gop_modes(void)
{
	EFI_STATUS status;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
	UINTN sz;
	UINT32 i;
	uint8_t depth;

	if (efi_gop == NULL)
		return 1;

	for (i = 0; i < efi_gop->Mode->MaxMode; i++) {
		status = uefi_call_wrapper(efi_gop->QueryMode, 4, efi_gop, i,
		    &sz, &info);
		if (EFI_ERROR(status) && status == EFI_NOT_STARTED) {
			status = uefi_call_wrapper(efi_gop->SetMode, 2,
			    efi_gop, efi_gop->Mode->Mode);
			status = uefi_call_wrapper(efi_gop->QueryMode, 4,
			    efi_gop, i, &sz, &info);
		}
		if (EFI_ERROR(status))
			continue;

		Print(L"%c%d: %dx%d ",
		    memcmp(info, efi_gop->Mode->Info, sizeof(*info)) == 0 ?
		      '*' : ' ',
		      i, info->HorizontalResolution, info->VerticalResolution);
		switch (info->PixelFormat) {
		case PixelRedGreenBlueReserved8BitPerColor:
			Print(L"RGBR");
			break;
		case PixelBlueGreenRedReserved8BitPerColor:
			Print(L"BGRR");
			break;
		case PixelBitMask:
			Print(L"R:%08x G:%08x B:%08x X:%08x",
			    info->PixelInformation.RedMask,
			    info->PixelInformation.GreenMask,
			    info->PixelInformation.BlueMask,
			    info->PixelInformation.ReservedMask);
			break;
		case PixelBltOnly:
			Print(L"(blt only)");
			break;
		default:
			Print(L"(Invalid pixel format)");
			break;
		}
		Print(L" pitch %d", info->PixelsPerScanLine);
		depth = getdepth(info);
		if (depth > 0)
			Print(L" bpp %d", depth);
		Print(L"\n");
	}

	return 0;
}

static int
efi_find_gop_mode(char *arg)
{
	EFI_STATUS status;
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
	UINTN sz;
	UINT32 i;
	char mode[32];
	uint8_t depth;

	for (i = 0; i < efi_gop->Mode->MaxMode; i++) {
		status = uefi_call_wrapper(efi_gop->QueryMode, 4, efi_gop, i,
		    &sz, &info);
		if (EFI_ERROR(status))
			continue;

		depth = getdepth(info);
		if (depth == 0)
			continue;

		snprintf(mode, sizeof(mode), "%lux%lux%u",
		    (long)info->HorizontalResolution,
		    (long)info->HorizontalResolution,
		    depth);
		if (strcmp(arg, mode) == 0)
			return i;
	}
	return -1;
}

void
command_gop(char *arg)
{
	EFI_STATUS status;
	INT32 modenum;

	if (efi_gop == NULL) {
		printf("GOP not supported by firmware\n");
		return;
	}

	if (*arg == '\0' || strcmp(arg, "list") == 0) {
		print_gop_modes();
		return;
	}

	if (strchr(arg, 'x') != NULL) {
		modenum = efi_find_gop_mode(arg);
		if (modenum == -1) {
			printf("mode %s not supported by firmware\n", arg);
			return;
		}
	} else {
		modenum = strtoul(arg, NULL, 0);
	}

	status = uefi_call_wrapper(efi_gop->SetMode, 2, efi_gop, modenum);
	if (!EFI_ERROR(status) && efi_gop->Mode->Mode == modenum) {
		efi_gop_mode = modenum;
		return;
	}

	printf("invalid flag, must be 'list', a display mode, "
	    "or a mode number\n");
}

static void
eficons_init_video(void)
{
	EFI_STATUS status;
	UINTN cols, rows;
	INT32 i, best, mode80x25, mode100x31;

	/*
	 * Setup text mode
	 */
	uefi_call_wrapper(ST->ConOut->Reset, 2, ST->ConOut, TRUE);

	mode80x25 = mode100x31 = -1;
	for (i = 0; i < ST->ConOut->Mode->MaxMode; i++) {
		status = uefi_call_wrapper(ST->ConOut->QueryMode, 4,
		    ST->ConOut, i, &cols, &rows);
		if (EFI_ERROR(status))
			continue;

		if (mode80x25 < 0 && cols == 80 && rows == 25)
			mode80x25 = i;
		else if (mode100x31 < 0 && cols == 100 && rows == 31)
			mode100x31 = i;
	}
	best = mode100x31 >= 0 ? mode100x31 : mode80x25 >= 0 ? mode80x25 : -1;
	if (best >= 0)
		uefi_call_wrapper(ST->ConOut->SetMode, 2, ST->ConOut, best);
	uefi_call_wrapper(ST->ConOut->EnableCursor, 2, ST->ConOut, TRUE);
	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

	LibLocateProtocol(&GraphicsOutputProtocol, (void **)&efi_gop);
}

/*
 * for Apple EFI
 */
#define	CONSOLE_CONTROL_PROTOCOL \
	{0xf42f7782, 0x12e, 0x4c12, {0x99, 0x56, 0x49, 0xf9, 0x43, 0x4, 0xf7, 0x21}}
static EFI_GUID ConsoleControlProtocol = CONSOLE_CONTROL_PROTOCOL;

struct _EFI_CONSOLE_CONTROL_INTERFACE;
typedef struct _EFI_CONSOLE_CONTROL_INTERFACE EFI_CONSOLE_CONTROL_INTERFACE;
typedef enum { EfiConsoleControlScreenText } EFI_CONSOLE_CONTROL_SCREEN_MODE;
typedef EFI_STATUS (EFIAPI *EFI_CONSOLE_CONTROL_PROTOCOL_SET_MODE) (
	IN EFI_CONSOLE_CONTROL_INTERFACE *This,
	IN EFI_CONSOLE_CONTROL_SCREEN_MODE Mode
);
struct _EFI_CONSOLE_CONTROL_INTERFACE {
	VOID *GetMode;
	EFI_CONSOLE_CONTROL_PROTOCOL_SET_MODE SetMode;
	VOID *LockStdIn;
};

static void
efi_switch_video_to_text_mode(void)
{
	EFI_STATUS status;
	EFI_CONSOLE_CONTROL_INTERFACE *cci;

	/* Set up the console, so printf works. */
	status = LibLocateProtocol(&ConsoleControlProtocol, (void **)&cci);
	if (!EFI_ERROR(status)) {
		uefi_call_wrapper(cci->SetMode, 2, cci,
		    EfiConsoleControlScreenText);
	}
}
