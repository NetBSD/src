/* $NetBSD: linux_keymap.c,v 1.2.2.2 2017/12/03 11:37:37 jdolecek Exp $ */

/*-
 * Copyright (c) 1997-2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes and Jared McNeill.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_keymap.c,v 1.2.2.2 2017/12/03 11:37:37 jdolecek Exp $");

#include <sys/types.h>
#include <dev/wscons/linux_keymap.h>

static const uint8_t linux_key_to_usb_map[] = {
	[0  ] /*         KEY_RESERVED */ =	0x00,
	[1  ] /*              KEY_ESC */ =	0x29,
	[2  ] /*                KEY_1 */ =	0x1e,
	[3  ] /*                KEY_2 */ =	0x1f,
	[4  ] /*                KEY_3 */ =	0x20,
	[5  ] /*                KEY_4 */ =	0x21,
	[6  ] /*                KEY_5 */ =	0x22,
	[7  ] /*                KEY_6 */ =	0x23,
	[8  ] /*                KEY_7 */ =	0x24,
	[9  ] /*                KEY_8 */ =	0x25,
	[10 ] /*                KEY_9 */ =	0x26,
	[11 ] /*                KEY_0 */ =	0x27,
	[12 ] /*            KEY_MINUS */ =	0x2d,
	[13 ] /*            KEY_EQUAL */ =	0x2e,
	[14 ] /*        KEY_BACKSPACE */ =	0x2a,
	[15 ] /*              KEY_TAB */ =	0x2b,
	[16 ] /*                KEY_Q */ =	0x14,
	[17 ] /*                KEY_W */ =	0x1a,
	[18 ] /*                KEY_E */ =	0x08,
	[19 ] /*                KEY_R */ =	0x15,
	[20 ] /*                KEY_T */ =	0x17,
	[21 ] /*                KEY_Y */ =	0x1c,
	[22 ] /*                KEY_U */ =	0x18,
	[23 ] /*                KEY_I */ =	0x0c,
	[24 ] /*                KEY_O */ =	0x12,
	[25 ] /*                KEY_P */ =	0x13,
	[26 ] /*        KEY_LEFTBRACE */ =	0x2f,
	[27 ] /*       KEY_RIGHTBRACE */ =	0x30,
	[28 ] /*            KEY_ENTER */ =	0x28,
	[29 ] /*         KEY_LEFTCTRL */ =	0xe0,
	[30 ] /*                KEY_A */ =	0x04,
	[31 ] /*                KEY_S */ =	0x16,
	[32 ] /*                KEY_D */ =	0x07,
	[33 ] /*                KEY_F */ =	0x09,
	[34 ] /*                KEY_G */ =	0x0a,
	[35 ] /*                KEY_H */ =	0x0b,
	[36 ] /*                KEY_J */ =	0x0d,
	[37 ] /*                KEY_K */ =	0x0e,
	[38 ] /*                KEY_L */ =	0x0f,
	[39 ] /*        KEY_SEMICOLON */ =	0x33,
	[40 ] /*       KEY_APOSTROPHE */ =	0x34,
	[41 ] /*            KEY_GRAVE */ =	0x35,
	[42 ] /*        KEY_LEFTSHIFT */ =	0xe1,
	[43 ] /*        KEY_BACKSLASH */ =	0x31,
	[44 ] /*                KEY_Z */ =	0x1d,
	[45 ] /*                KEY_X */ =	0x1b,
	[46 ] /*                KEY_C */ =	0x06,
	[47 ] /*                KEY_V */ =	0x19,
	[48 ] /*                KEY_B */ =	0x05,
	[49 ] /*                KEY_N */ =	0x11,
	[50 ] /*                KEY_M */ =	0x10,
	[51 ] /*            KEY_COMMA */ =	0x36,
	[52 ] /*              KEY_DOT */ =	0x37,
	[53 ] /*            KEY_SLASH */ =	0x38,
	[54 ] /*       KEY_RIGHTSHIFT */ =	0xe5,
	[55 ] /*       KEY_KPASTERISK */ =	0x55,
	[56 ] /*          KEY_LEFTALT */ =	0xe2,
	[57 ] /*            KEY_SPACE */ =	0x2c,
	[58 ] /*         KEY_CAPSLOCK */ =	0x39,
	[59 ] /*               KEY_F1 */ =	0x3a,
	[60 ] /*               KEY_F2 */ =	0x3b,
	[61 ] /*               KEY_F3 */ =	0x3c,
	[62 ] /*               KEY_F4 */ =	0x3d,
	[63 ] /*               KEY_F5 */ =	0x3e,
	[64 ] /*               KEY_F6 */ =	0x3f,
	[65 ] /*               KEY_F7 */ =	0x40,
	[66 ] /*               KEY_F8 */ =	0x41,
	[67 ] /*               KEY_F9 */ =	0x42,
	[68 ] /*              KEY_F10 */ =	0x43,
	[69 ] /*          KEY_NUMLOCK */ =	0x53,
	[70 ] /*       KEY_SCROLLLOCK */ =	0x47,
	[71 ] /*              KEY_KP7 */ =	0x5f,
	[72 ] /*              KEY_KP8 */ =	0x60,
	[73 ] /*              KEY_KP9 */ =	0x61,
	[74 ] /*          KEY_KPMINUS */ =	0x56,
	[75 ] /*              KEY_KP4 */ =	0x5c,
	[76 ] /*              KEY_KP5 */ =	0x5d,
	[77 ] /*              KEY_KP6 */ =	0x5e,
	[78 ] /*           KEY_KPPLUS */ =	0x57,
	[79 ] /*              KEY_KP1 */ =	0x59,
	[80 ] /*              KEY_KP2 */ =	0x5a,
	[81 ] /*              KEY_KP3 */ =	0x5b,
	[82 ] /*              KEY_KP0 */ =	0x62,
	[83 ] /*            KEY_KPDOT */ =	0x63,
	[85 ] /*   KEY_ZENKAKUHANKAKU */ =	0x94,
#if 0
	[86 ] /*            KEY_102ND */ =	???
#endif
	[87 ] /*              KEY_F11 */ =	0x44,
	[88 ] /*              KEY_F12 */ =	0x45,
	[89 ] /*               KEY_RO */ =	0x87,
	[90 ] /*         KEY_KATAKANA */ =	0x92,
	[91 ] /*         KEY_HIRAGANA */ =	0x93,
	[92 ] /*           KEY_HENKAN */ =	0x8a,
	[93 ] /* KEY_KATAKANAHIRAGANA */ =	0x88,
	[94 ] /*         KEY_MUHENKAN */ =	0x8b,
	[95 ] /*        KEY_KPJPCOMMA */ =	0x8c,
	[96 ] /*          KEY_KPENTER */ =	0x58,
	[97 ] /*        KEY_RIGHTCTRL */ =	0xe4,
	[98 ] /*          KEY_KPSLASH */ =	0x54,
	[99 ] /*            KEY_SYSRQ */ =	0x9a,
	[100] /*         KEY_RIGHTALT */ =	0xe6,
#if 0
	[101] /*         KEY_LINEFEED */ =	???
#endif
	[102] /*             KEY_HOME */ =	0x4a,
	[103] /*               KEY_UP */ =	0x52,
	[104] /*           KEY_PAGEUP */ =	0x4b,
	[105] /*             KEY_LEFT */ =	0x50,
	[106] /*            KEY_RIGHT */ =	0x4f,
	[107] /*              KEY_END */ =	0x4d,
	[108] /*             KEY_DOWN */ =	0x51,
	[109] /*         KEY_PAGEDOWN */ =	0x4e,
	[110] /*           KEY_INSERT */ =	0x49,
	[111] /*           KEY_DELETE */ =	0x4c,
#if 0
	[112] /*            KEY_MACRO */ =	???
#endif
	[113] /*             KEY_MUTE */ =	0x7f,
	[114] /*       KEY_VOLUMEDOWN */ =	0x81,
	[115] /*         KEY_VOLUMEUP */ =	0x80,
	[116] /*            KEY_POWER */ =	0x66,
	[117] /*          KEY_KPEQUAL */ =	0x67,
#if 0
	[118] /*      KEY_KPPLUSMINUS */ =	???
#endif
	[119] /*            KEY_PAUSE */ =	0x48,
#if 0
	[120] /*            KEY_SCALE */ =	???
#endif
	[121] /*          KEY_KPCOMMA */ =	0x85,
	[122] /*          KEY_HANGEUL */ =	0x90,
	[123] /*            KEY_HANJA */ =	0x91,
	[124] /*              KEY_YEN */ =	0x89,
	[125] /*         KEY_LEFTMETA */ =	0xe3,
	[126] /*        KEY_RIGHTMETA */ =	0xe7,
#if 0
	[127] /*          KEY_COMPOSE */ =	???
#endif
};

uint8_t
linux_key_to_usb(u_int code)
{
	if (code >= __arraycount(linux_key_to_usb_map))
		return 0x00;	/* No Event */
	return linux_key_to_usb_map[code];
}
