/*	$NetBSD: eficons.c,v 1.4.10.1 2018/03/30 06:20:11 pgoyette Exp $	*/

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

static SERIAL_IO_INTERFACE *serios[4];
static int default_comspeed =
#if defined(CONSPEED)
    CONSPEED;
#else
    9600;
#endif
static u_char serbuf[16];
static int serbuf_read = 0;
static int serbuf_write = 0;

static void eficons_init_video(void);
static void efi_switch_video_to_text_mode(void);

static int efi_cons_getc(void);
static int efi_cons_putc(int);
static int efi_cons_iskey(int);
static int efi_cons_waitforinputevent(uint64_t);

static void efi_com_probe(void);
static bool efi_valid_com(int);
static int efi_com_init(int, int);
static int efi_com_getc(void);
static int efi_com_putc(int);
static int efi_com_status(int);
static int efi_com_waitforinputevent(uint64_t);

static int iodev;
static int (*internal_getchar)(void) = efi_cons_getc;
static int (*internal_putchar)(int) = efi_cons_putc;
static int (*internal_iskey)(int) = efi_cons_iskey;
static int (*internal_waitforinputevent)(uint64_t) = efi_cons_waitforinputevent;

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
	int i;

	btinfo_console.speed = default_comspeed;

	switch (dev) {
	case CONSDEV_AUTO:
		for (i = 0; i < __arraycount(serios); i++) {
			iodev = CONSDEV_COM0 + i;
			if (!efi_valid_com(iodev))
				continue;
			btinfo_console.addr = getcomaddr(i);

			efi_cons_putc('0' + i);
			efi_com_init(btinfo_console.addr, btinfo_console.speed);
			/* check for:
			 *  1. successful output
			 *  2. optionally, keypress within 7s
			 */
			if (efi_com_putc(':') &&
			    efi_com_putc('-') &&
			    efi_com_putc('(') &&
			    awaitkey(7, 0))
				goto ok;
		}
		goto nocom;
ok:
		break;

	case CONSDEV_COM0:
	case CONSDEV_COM1:
	case CONSDEV_COM2:
	case CONSDEV_COM3:
		iodev = dev;
		btinfo_console.addr = ioport;
		if (btinfo_console.addr == 0) {
			if (!efi_valid_com(iodev))
				goto nocom;
			btinfo_console.addr = getcomaddr(iodev - CONSDEV_COM0);
		}
		if (speed != 0)
			btinfo_console.speed = speed;
		efi_com_init(btinfo_console.addr, btinfo_console.speed);
		break;

	case CONSDEV_COM0KBD:
	case CONSDEV_COM1KBD:
	case CONSDEV_COM2KBD:
	case CONSDEV_COM3KBD:
		iodev = dev - CONSDEV_COM0KBD + CONSDEV_COM0;
		if (!efi_valid_com(iodev))
			goto nocom;
		btinfo_console.addr = getcomaddr(iodev - CONSDEV_COM0);

		efi_cons_putc('0' + iodev - CONSDEV_COM0);
		efi_com_init(btinfo_console.addr, btinfo_console.speed);
		/* check for:
		 *  1. successful output
		 *  2. optionally, keypress within 7s
		 */
		if (efi_com_putc(':') &&
		    efi_com_putc('-') &&
		    efi_com_putc('(') &&
		    awaitkey(7, 0))
			goto kbd;
		/*FALLTHROUGH*/
	case CONSDEV_PC:
	default:
nocom:
		iodev = CONSDEV_PC;
		internal_putchar = efi_cons_putc;
kbd:
		internal_getchar = efi_cons_getc;
		internal_iskey = efi_cons_iskey;
		internal_waitforinputevent = efi_cons_waitforinputevent;
		memset(keybuf, 0, sizeof(keybuf));
		keybuf_read = keybuf_write = 0;
		break;
	}

	strlcpy(btinfo_console.devname, iodev == CONSDEV_PC ? "pc" : "com", 16);
}

int
cninit(void)
{

	efi_switch_video_to_text_mode();
	eficons_init_video();
	efi_com_probe();

	consinit(boot_params.bp_consdev, boot_params.bp_consaddr,
	    boot_params.bp_conspeed);

	return 0;
}

void
efi_cons_show(void)
{
	const bool pc_is_console = strcmp(btinfo_console.devname, "pc") == 0;
	const bool com_is_console = strcmp(btinfo_console.devname, "com") == 0;
	bool first = true;
	bool found = false;
	int i;

	if (efi_gop != NULL) {
		printf("pc");
		if (pc_is_console)
			printf("*");
		first = false;
	}

	for (i = 0; i < __arraycount(serios); i++) {
		if (serios[i] != NULL) {
			if (!first)
				printf(" ");
			first = false;

			printf("com%d", i);
			if (com_is_console &&
			    btinfo_console.addr == getcomaddr(i)) {
				printf(",%d*", btinfo_console.speed);
				found = true;
			}
		}
	}
	if (!found && com_is_console) {
		if (!first)
			printf(" ");
		first = false;

		printf("com,0x%x,%d*", btinfo_console.addr,
		    btinfo_console.speed);
	}

	printf("\n");
}

static int
efi_cons_getc(void)
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

static int
efi_cons_putc(int c)
{
	CHAR16 buf[2];

	buf[0] = c;
	buf[1] = 0;
	Output(buf);

	return 1;
}

/*ARGSUSED*/
static int
efi_cons_iskey(int intr)
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

static int
efi_cons_waitforinputevent(uint64_t timeout)
{
	EFI_STATUS status;

	status = WaitForSingleEvent(ST->ConIn->WaitForKey, timeout);
	if (!EFI_ERROR(status))
		return 0;
	if (status == EFI_TIMEOUT)
		return ETIMEDOUT;
	return EINVAL;
}

int
getchar(void)
{

	return internal_getchar();
}

void
putchar(int c)
{

	if (c == '\n')
		internal_putchar('\r');
	internal_putchar(c);
}

int
iskey(int intr)
{

	return internal_iskey(intr);
}

char
awaitkey(int timeout, int tell)
{
	char c = 0;

	for (;;) {
		if (tell && timeout) {
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
			internal_waitforinputevent(10000000);
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
			printf("GOP setmode failed: %" PRIxMAX "\n",
			    (uintmax_t)status);
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
		panic("Error: invalid pixel format (%d)", info->PixelFormat);
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
		printf("%c%d: %" PRIxMAX "x%" PRIxMAX "\n",
		    i == curmode ? '*' : ' ', i, (uintmax_t)cols, (uintmax_t)rows);
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
		snprintf(mode, sizeof(mode), "%" PRIuMAX "x%" PRIuMAX,
		    (uintmax_t)cols, (uintmax_t)rows);
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

		printf("%c%d: %dx%d ",
		    memcmp(info, efi_gop->Mode->Info, sizeof(*info)) == 0 ?
		      '*' : ' ',
		      i, info->HorizontalResolution, info->VerticalResolution);
		switch (info->PixelFormat) {
		case PixelRedGreenBlueReserved8BitPerColor:
			printf("RGBR");
			break;
		case PixelBlueGreenRedReserved8BitPerColor:
			printf("BGRR");
			break;
		case PixelBitMask:
			printf("R:%08x G:%08x B:%08x X:%08x",
			    info->PixelInformation.RedMask,
			    info->PixelInformation.GreenMask,
			    info->PixelInformation.BlueMask,
			    info->PixelInformation.ReservedMask);
			break;
		case PixelBltOnly:
			printf("(blt only)");
			break;
		default:
			printf("(Invalid pixel format)");
			break;
		}
		printf(" pitch %d", info->PixelsPerScanLine);
		depth = getdepth(info);
		if (depth > 0)
			printf(" bpp %d", depth);
		printf("\n");
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

/*
 * serial port
 */
static void
efi_com_probe(void)
{
	EFI_STATUS status;
	UINTN i, nhandles;
	EFI_HANDLE *handles;
	EFI_DEVICE_PATH	*dp, *dp0;
	EFI_DEV_PATH_PTR dpp;
	SERIAL_IO_INTERFACE *serio;
	int uid = -1;

	status = LibLocateHandle(ByProtocol, &SerialIoProtocol, NULL,
	    &nhandles, &handles);
	if (EFI_ERROR(status))
		return;

	for (i = 0; i < nhandles; i++) {
		/*
		 * Identify port number of the handle.  This assumes ACPI
		 * UID 0-3 map to legacy COM[1-4] and they use the legacy
		 * port address.
		 */
		status = uefi_call_wrapper(BS->HandleProtocol, 3, handles[i],
		    &DevicePathProtocol, (void **)&dp0);
		if (EFI_ERROR(status))
			continue;

		for (uid = -1, dp = dp0;
		     !IsDevicePathEnd(dp);
		     dp = NextDevicePathNode(dp)) {

			if (DevicePathType(dp) == ACPI_DEVICE_PATH &&
			    DevicePathSubType(dp) == ACPI_DP) {
				dpp = (EFI_DEV_PATH_PTR)dp;
				if (dpp.Acpi->HID == EISA_PNP_ID(0x0501)) {
					uid = dpp.Acpi->UID;
					break;
				}
			}
		}
		if (uid < 0 || __arraycount(serios) <= uid)
			continue;

		/* Prepare SERIAL_IO_INTERFACE */
		status = uefi_call_wrapper(BS->HandleProtocol, 3, handles[i],
		    &SerialIoProtocol, (void **)&serio);
		if (EFI_ERROR(status))
			continue;

		serios[uid] = serio;
	}

	FreePool(handles);

}

static bool
efi_valid_com(int dev)
{
	int idx;

	switch (dev) {
	default:
	case CONSDEV_PC:
		return false;

	case CONSDEV_COM0:
	case CONSDEV_COM1:
	case CONSDEV_COM2:
	case CONSDEV_COM3:
		idx = dev - CONSDEV_COM0;
		break;
	}

	return idx < __arraycount(serios) &&
	    serios[idx] != NULL &&
	    getcomaddr(idx) != 0;
}

static int
efi_com_init(int addr, int speed)
{
	EFI_STATUS status;
	SERIAL_IO_INTERFACE *serio;

	if (speed <= 0)
		return 0;

	if (!efi_valid_com(iodev))
		return 0;

	serio = serios[iodev - CONSDEV_COM0];

	if (serio->Mode->BaudRate != btinfo_console.speed) {
		status = uefi_call_wrapper(serio->SetAttributes, 7, serio,
		    speed, serio->Mode->ReceiveFifoDepth,
		    serio->Mode->Timeout, serio->Mode->Parity,
		    serio->Mode->DataBits, serio->Mode->StopBits);
		if (EFI_ERROR(status)) {
			printf("com%d: SetAttribute() failed with status=%" PRIxMAX
			    "\n", iodev - CONSDEV_COM0, (uintmax_t)status);
			return 0;
		}
	}

	default_comspeed = speed;
	internal_getchar = efi_com_getc;
	internal_putchar = efi_com_putc;
	internal_iskey = efi_com_status;
	internal_waitforinputevent = efi_com_waitforinputevent;
	memset(serbuf, 0, sizeof(serbuf));
	serbuf_read = serbuf_write = 0;

	return speed;
}

static int
efi_com_getc(void)
{
	EFI_STATUS status;
	SERIAL_IO_INTERFACE *serio;
	UINTN sz;
	u_char c;

	if (!efi_valid_com(iodev))
		panic("Invalid serial port: iodev=%d", iodev);

	if (serbuf_read != serbuf_write) {
		c = serbuf[serbuf_read];
		serbuf_read = (serbuf_read + 1) % __arraycount(serbuf);
		return c;
	}

	serio = serios[iodev - CONSDEV_COM0];

	for (;;) {
		sz = 1;
		status = uefi_call_wrapper(serio->Read, 3, serio, &sz, &c);
		if (!EFI_ERROR(status) && sz > 0)
			break;
		if (status != EFI_TIMEOUT && EFI_ERROR(status))
			panic("Error reading from serial status=%"PRIxMAX,
			    (uintmax_t)status);
	}
	return c;
}

static int
efi_com_putc(int c)
{
	EFI_STATUS status;
	SERIAL_IO_INTERFACE *serio;
	UINTN sz = 1;
	u_char buf;

	if (!efi_valid_com(iodev))
		return 0;

	serio = serios[iodev - CONSDEV_COM0];
	buf = c;
	status = uefi_call_wrapper(serio->Write, 3, serio, &sz, &buf);
	if (EFI_ERROR(status) || sz < 1)
		return 0;
	return 1;
}

/*ARGSUSED*/
static int
efi_com_status(int intr)
{
	EFI_STATUS status;
	SERIAL_IO_INTERFACE *serio;
	UINTN sz;
	u_char c;

	if (!efi_valid_com(iodev))
		panic("Invalid serial port: iodev=%d", iodev);

	if (serbuf_read != serbuf_write)
		return 1;

	serio = serios[iodev - CONSDEV_COM0];
	sz = 1;
	status = uefi_call_wrapper(serio->Read, 3, serio, &sz, &c);
	if (EFI_ERROR(status) || sz < 1)
		return 0;

	serbuf[serbuf_write] = c;
	serbuf_write = (serbuf_write + 1) % __arraycount(serbuf);
	return 1;
}

static void
efi_com_periodic_event(EFI_EVENT event, void *ctx)
{
	EFI_EVENT timer = ctx;

	if (efi_com_status(0)) {
		uefi_call_wrapper(BS->SetTimer, 3, event, TimerCancel, 0);
		uefi_call_wrapper(BS->SignalEvent, 1, timer);
	}
}

static int
efi_com_waitforinputevent(uint64_t timeout)
{
	EFI_STATUS status;
	EFI_EVENT timer, periodic;

	status = uefi_call_wrapper(BS->CreateEvent, 5, EVT_TIMER, 0, NULL, NULL,
	    &timer);
	if (EFI_ERROR(status))
		return EINVAL;

        status = uefi_call_wrapper(BS->CreateEvent, 5,
	    EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, efi_com_periodic_event,
	    timer, &periodic);
	if (EFI_ERROR(status)) {
		uefi_call_wrapper(BS->CloseEvent, 1, timer);
		return EINVAL;
	}

	status = uefi_call_wrapper(BS->SetTimer, 3, periodic, TimerPeriodic,
	    1000000);	/* 100ms */
	if (EFI_ERROR(status)) {
		uefi_call_wrapper(BS->CloseEvent, 1, periodic);
		uefi_call_wrapper(BS->CloseEvent, 1, timer);
		return EINVAL;
	}
	status = WaitForSingleEvent(&timer, timeout);
	uefi_call_wrapper(BS->SetTimer, 3, periodic, TimerCancel, 0);
	uefi_call_wrapper(BS->CloseEvent, 1, periodic);
	uefi_call_wrapper(BS->CloseEvent, 1, timer);
	if (!EFI_ERROR(status))
		return 0;
	if (status == EFI_TIMEOUT)
		return ETIMEDOUT;
	return EINVAL;
}
