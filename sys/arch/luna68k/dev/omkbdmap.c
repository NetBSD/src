/*	$NetBSD: omkbdmap.c,v 1.3 2023/01/15 05:08:33 tsutsui Exp $	*/
/*	$OpenBSD: omkbdmap.c,v 1.2 2013/11/16 18:31:44 miod Exp $	*/

/* Partially from:
 *	NetBSD: lunaws.c,v 1.6 2002/03/17 19:40:42 atatat Exp
 *
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#include "opt_wsdisplay_compat.h"

#include <sys/types.h>

#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <luna68k/dev/omkbdmap.h>

#ifdef WSDISPLAY_COMPAT_RAWKBD

/*
 * Translate LUNA keycodes to US keyboard XT scancodes, for proper
 * X11-over-wsmux operation.
 *
 * XXX: Needs re-think how we should treat RAWKBD code on NetBSD.
 */
const uint8_t omkbd_raw[0x80] = {
	0x00,		/* 0x00 */
	0x00,		/* 0x01 */
	0x00,		/* 0x02 */
	0x00,		/* 0x03 */
	0x00,		/* 0x04 */
	0x00,		/* 0x05 */
	0x00,		/* 0x06 */
	0x00,		/* 0x07 */
	0x00,		/* 0x08 */
	0x0f,		/* 0x09 */
	0x1d,		/* 0x0a */
	0x70,		/* 0x0b: Kana / No RAWKEY_XXX symbol */
	0x36,		/* 0x0c */
	0x2a,		/* 0x0d */
	0x3a,		/* 0x0e */
	0x38,		/* 0x0f: Zenmen */
	0x01,		/* 0x10 */
	0x0e,		/* 0x11 */
	0x1c,		/* 0x12 */
	0x00,		/* 0x13 */
	0x39,		/* 0x14 */
	0xd3,		/* 0x15 */
	0x38,		/* 0x16: Henkan */
	0xb8,		/* 0x17: Kakutei */
	0x57,		/* 0x18: Shokyo */
	0x58,		/* 0x19: Yobidashi */
	0x00,		/* 0x1a: Bunsetsu L / f13 */
	0x00,		/* 0x1b: Bunsetsu R / f14 */
	0x48,		/* 0x1c */
	0x4b,		/* 0x1d */
	0x4d,		/* 0x1e */
	0x50,		/* 0x1f */
	0x57,		/* 0x20 */
	0x58,		/* 0x21 */
	0x02,		/* 0x22: exclam */
	0x03,		/* 0x23: quotedbl */
	0x04,		/* 0x24: numbersign */
	0x05,		/* 0x25: dollar */
	0x06,		/* 0x26: percent */
	0x07,		/* 0x27: ampersand */
	0x08,		/* 0x28: apostrophe */
	0x09,		/* 0x29: parenleft */
	0x0a,		/* 0x2a: parenright  */
	0x0b,		/* 0x2b */
	0x0c,		/* 0x2c: equal */
	0x0d,		/* 0x2d: asciitilde */
	0x7d,		/* 0x2e: bar */
	0x00,		/* 0x2f */
	0x00,		/* 0x30: f13 */
	0x00,		/* 0x31: f14 */
	0x10,		/* 0x32 */
	0x11,		/* 0x33 */
	0x12,		/* 0x34 */
	0x13,		/* 0x35 */
	0x14,		/* 0x36 */
	0x15,		/* 0x37 */
	0x16,		/* 0x38 */
	0x17,		/* 0x39 */
	0x18,		/* 0x3a */
	0x19,		/* 0x3b */
	0x1a,		/* 0x3c: grave */
	0x1b,		/* 0x3d: braceleft */
	0x00,		/* 0x3e */
	0x00,		/* 0x3f */
	0x00,		/* 0x40 */
	0x00,		/* 0x41 */
	0x1e,		/* 0x42 */
	0x1f,		/* 0x43 */
	0x20,		/* 0x44 */
	0x21,		/* 0x45 */
	0x22,		/* 0x46 */
	0x23,		/* 0x47 */
	0x24,		/* 0x48 */
	0x25,		/* 0x49 */
	0x26,		/* 0x4a */
	0x27,		/* 0x4b: plus */
	0x28,		/* 0x4c: asterisk */
	0x2b,		/* 0x4d: braceright */
	0x00,		/* 0x4e */
	0x00,		/* 0x4f */
	0x00,		/* 0x50 */
	0x00,		/* 0x51 */
	0x2c,		/* 0x52 */
	0x2d,		/* 0x53 */
	0x2e,		/* 0x54 */
	0x2f,		/* 0x55 */
	0x30,		/* 0x56 */
	0x31,		/* 0x57 */
	0x32,		/* 0x58 */
	0x33,		/* 0x59: less */
	0x34,		/* 0x5a: greater */
	0x35,		/* 0x5b: question */
	0x73,		/* 0x5c: underscore */
	0x00,		/* 0x5d */
	0x00,		/* 0x5e */
	0x00,		/* 0x5f */
	0x53,		/* 0x60 */
	0x4e,		/* 0x61 */
	0x4a,		/* 0x62 */
	0x47,		/* 0x63: KP 7 */
	0x48,		/* 0x64: KP 8 */
	0x49,		/* 0x65: KP 9 */
	0x4b,		/* 0x66: KP 4 */
	0x4c,		/* 0x67: KP 5 */
	0x4d,		/* 0x68: KP 6 */
	0x4f,		/* 0x69: KP 1 */
	0x50,		/* 0x6a: KP 2 */
	0x51,		/* 0x6b: KP 3 */
	0x52,		/* 0x6c: KP 0 */
	0x53,		/* 0x6d: KP Decimal */
	0x9c,		/* 0x6e */
	0x00,		/* 0x6f */
	0x00,		/* 0x70 */
	0x00,		/* 0x71 */
	0x3b,		/* 0x72 */
	0x3c,		/* 0x73 */
	0x3d,		/* 0x74 */
	0x3e,		/* 0x75 */
	0x3f,		/* 0x76 */
	0x40,		/* 0x77 */
	0x41,		/* 0x78 */
	0x42,		/* 0x79 */
	0x43,		/* 0x7a */
	0x44,		/* 0x7b */
	0x37,		/* 0x7c */
	0xb5,		/* 0x7d */
	0x76,		/* 0x7E */
	0x00,		/* 0x7f: KP Separator */
};
#endif

#define KC(n) KS_KEYCODE(n)

static const keysym_t omkbd_keydesc_jp[] = {
/*	pos		command		normal		shifted */
	KC(0x09),			KS_Tab,
	KC(0x0a),   KS_Cmd1,		KS_Control_L,
	KC(0x0b),			KS_Mode_switch,	/* Kana */
	KC(0x0c),			KS_Shift_R,
	KC(0x0d),			KS_Shift_L,
	KC(0x0e),			KS_Caps_Lock,
	KC(0x0f),   KS_Cmd2,		KS_Meta_L,	/* Zenmen */
	KC(0x10),   KS_Cmd_Debugger,	KS_Escape,
	KC(0x11),			KS_BackSpace,
	KC(0x12),			KS_Return,
	KC(0x14),			KS_space,
	KC(0x15),			KS_Delete,
	KC(0x16),			KS_Alt_L,	/* Henkan */
	KC(0x17),			KS_Alt_R,	/* Kakutei */
	KC(0x18),			KS_f11,		/* Shokyo */
	KC(0x19),			KS_f12,		/* Yobidashi */
	KC(0x1a),			KS_f13,		/* Bunsetsu L */
	KC(0x1b),			KS_f14,		/* Bunsetsu R */
	KC(0x1c),			KS_KP_Up,
	KC(0x1d),			KS_KP_Left,
	KC(0x1e),			KS_KP_Right,
	KC(0x1f),			KS_KP_Down,
	/* 0x20,			KS_f11, */
	/* 0x21,			KS_f12, */
	KC(0x22),			KS_1,		KS_exclam,
	KC(0x23),			KS_2,		KS_quotedbl,
	KC(0x24),			KS_3,		KS_numbersign,
	KC(0x25),			KS_4,		KS_dollar,
	KC(0x26),			KS_5,		KS_percent,
	KC(0x27),			KS_6,		KS_ampersand,
	KC(0x28),			KS_7,		KS_apostrophe,
	KC(0x29),			KS_8,		KS_parenleft,
	KC(0x2a),			KS_9,		KS_parenright,
	KC(0x2b),			KS_0,
	KC(0x2c),			KS_minus,	KS_equal,
	KC(0x2d),			KS_asciicircum,	KS_asciitilde,
	KC(0x2e),			KS_backslash,	KS_bar,

	/* 0x30,			KS_f13, */
	/* 0x31,			KS_f14, */
	KC(0x32),			KS_q,
	KC(0x33),			KS_w,
	KC(0x34),			KS_e,
	KC(0x35),			KS_r,
	KC(0x36),			KS_t,
	KC(0x37),			KS_y,
	KC(0x38),			KS_u,
	KC(0x39),			KS_i,
	KC(0x3a),			KS_o,
	KC(0x3b),			KS_p,
	KC(0x3c),			KS_at,		KS_grave,
	KC(0x3d),			KS_bracketleft,	KS_braceleft,

	KC(0x42),			KS_a,
	KC(0x43),			KS_s,
	KC(0x44),			KS_d,
	KC(0x45),			KS_f,
	KC(0x46),			KS_g,
	KC(0x47),			KS_h,
	KC(0x48),			KS_j,
	KC(0x49),			KS_k,
	KC(0x4a),			KS_l,
	KC(0x4b),			KS_semicolon,	KS_plus,
	KC(0x4c),			KS_colon,	KS_asterisk,
	KC(0x4d),			KS_bracketright, KS_braceright,

	KC(0x52),			KS_z,
	KC(0x53),			KS_x,
	KC(0x54),			KS_c,
	KC(0x55),			KS_v,
	KC(0x56),			KS_b,
	KC(0x57),			KS_n,
	KC(0x58),			KS_m,
	KC(0x59),			KS_comma,	KS_less,
	KC(0x5a),			KS_period,	KS_greater,
	KC(0x5b),			KS_slash,	KS_question,
	KC(0x5c),			KS_underscore,

	KC(0x60),			KS_KP_Delete,
	KC(0x61),			KS_KP_Add,
	KC(0x62),			KS_KP_Subtract,
	KC(0x63),			KS_KP_7,
	KC(0x64),			KS_KP_8,
	KC(0x65),			KS_KP_9,
	KC(0x66),			KS_KP_4,
	KC(0x67),			KS_KP_5,
	KC(0x68),			KS_KP_6,
	KC(0x69),			KS_KP_1,
	KC(0x6a),			KS_KP_2,
	KC(0x6b),			KS_KP_3,
	KC(0x6c),			KS_KP_0,
	KC(0x6d),			KS_KP_Decimal,
	KC(0x6e),			KS_KP_Enter,

	KC(0x72),			KS_f1,
	KC(0x73),			KS_f2,
	KC(0x74),			KS_f3,
	KC(0x75),			KS_f4,
	KC(0x76),			KS_f5,
	KC(0x77),			KS_f6,
	KC(0x78),			KS_f7,
	KC(0x79),			KS_f8,
	KC(0x7a),			KS_f9,
	KC(0x7b),			KS_f10,
	KC(0x7c),			KS_KP_Multiply,
	KC(0x7d),			KS_KP_Divide,
	KC(0x7e),			KS_KP_Equal,
	KC(0x7f),			KS_KP_Separator,
};

#define	SIZE(map) (sizeof(map)/sizeof(keysym_t))

const struct wscons_keydesc omkbd_keydesctab[] = {
       { KB_JP, 0, SIZE(omkbd_keydesc_jp), omkbd_keydesc_jp, },
       { 0, 0, 0, 0 },
};
