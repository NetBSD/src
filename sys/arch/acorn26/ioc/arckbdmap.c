/* $NetBSD: arckbdmap.c,v 1.4 2003/07/14 22:48:22 lukem Exp $ */
/*-
 * Copyright (c) 1998 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * arckbdmap.c - Archimedes keyboard maps
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: arckbdmap.c,v 1.4 2003/07/14 22:48:22 lukem Exp $");

#include <sys/types.h>
#include <sys/device.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <arch/acorn26/ioc/arckbdvar.h>
#include <arch/acorn26/ioc/arckbdreg.h>

#define KC(n)		(0xe000 | (n))  /* see wsksymdef.h */

/* Arc keycodes are 0xrc where r and c are the row and column codes */

static const keysym_t arckbd_keydesc_uk[] = {
/*	pos		command		normal		shifted */
	KC(0x00),	KS_Cmd_Debugger,KS_Escape,
	KC(0x01),	KS_Cmd_Screen0,	KS_f1,
	KC(0x02),	KS_Cmd_Screen1,	KS_f2,
	KC(0x03),	KS_Cmd_Screen2,	KS_f3,
	KC(0x04),	KS_Cmd_Screen3,	KS_f4,
	KC(0x05),	KS_Cmd_Screen4,	KS_f5,
	KC(0x06),	KS_Cmd_Screen5,	KS_f6,
	KC(0x07),	KS_Cmd_Screen6,	KS_f7,
	KC(0x08),	KS_Cmd_Screen7,	KS_f8,
	KC(0x09),	KS_Cmd_Screen8,	KS_f9,
	KC(0x0a),	KS_Cmd_Screen9,	KS_f10,
	KC(0x0b),			KS_f11,
	KC(0x0c),			KS_f12,
	KC(0x0d),			KS_voidSymbol, /* print: no keysym yet */
	KC(0x0e),			KS_Hold_Screen, /* scroll lock */
	KC(0x0f),			KS_voidSymbol, /* break: no keysym yet */

	KC(0x10),			KS_grave,	KS_asciitilde,
	KC(0x11),			KS_1,		KS_exclam,
	KC(0x12),			KS_2,		KS_at,
	KC(0x13),			KS_3,		KS_numbersign,
	KC(0x14),			KS_4,		KS_dollar,
	KC(0x15),			KS_5,		KS_percent,
	KC(0x16),			KS_6,		KS_asciicircum,
	KC(0x17),			KS_7,		KS_ampersand,
	KC(0x18),			KS_8,		KS_asterisk,
	KC(0x19),			KS_9,		KS_parenleft,
	KC(0x1a),			KS_0,		KS_parenright,
	KC(0x1b),			KS_minus,	KS_underscore,
	KC(0x1c),			KS_equal,	KS_plus,
	KC(0x1d),			KS_sterling,	KS_currency,
	KC(0x1e),			KS_BackSpace,
	KC(0x1f),			KS_Insert,
	KC(0x20),			KS_Home,
	KC(0x21),			KS_Prior,
	KC(0x22),			KS_Num_Lock,
	KC(0x23),			KS_KP_Divide,
	KC(0x24),			KS_KP_Multiply,
	KC(0x25),			KS_KP_Numbersign,

	KC(0x26),			KS_Tab,
	KC(0x27),			KS_q,
	KC(0x28),			KS_w,
	KC(0x29),			KS_e,
	KC(0x2a),			KS_r,
	KC(0x2b),			KS_t,
	KC(0x2c),			KS_y,
	KC(0x2d),			KS_u,
	KC(0x2e),			KS_i,
	KC(0x2f),			KS_o,
	KC(0x30),			KS_p,
	KC(0x31),			KS_bracketleft,	KS_braceleft,
	KC(0x32),			KS_bracketright,KS_braceright,
	KC(0x33),			KS_backslash,	KS_bar,
	KC(0x34),			KS_Delete,
	KC(0x35),			KS_End, /* really Copy */
	KC(0x36),			KS_Next,
	KC(0x37),			KS_KP_7,
	KC(0x38),			KS_KP_8,
	KC(0x39),			KS_KP_9,
	KC(0x3a),			KS_KP_Subtract,

	KC(0x3b),	KS_Cmd1,	KS_Control_L,
	KC(0x3c),			KS_a,
	KC(0x3d),			KS_s,
	KC(0x3e),			KS_d,
	KC(0x3f),			KS_f,
	KC(0x40),			KS_g,
	KC(0x41),			KS_h,
	KC(0x42),			KS_j,
	KC(0x43),			KS_k,
	KC(0x44),			KS_l,
	KC(0x45),			KS_semicolon,	KS_colon,
	KC(0x46),			KS_apostrophe,	KS_quotedbl,
	KC(0x47),			KS_Return,
	KC(0x48),			KS_KP_4,
	KC(0x49),			KS_KP_5,
	KC(0x4a),			KS_KP_6,
	KC(0x4b),			KS_KP_Add,

	KC(0x4c),			KS_Shift_L,
	KC(0x4e),			KS_z,
	KC(0x4f),			KS_x,
	KC(0x50),			KS_c,
	KC(0x51),			KS_v,
	KC(0x52),			KS_b,
	KC(0x53),			KS_n,
	KC(0x54),			KS_m,
	KC(0x55),			KS_comma,	KS_less,
	KC(0x56),			KS_period,	KS_greater,
	KC(0x57),			KS_slash,	KS_question,
	KC(0x58),			KS_Shift_R,
	KC(0x59),			KS_Up,
	KC(0x5a),			KS_KP_1,
	KC(0x5b),			KS_KP_2,
	KC(0x5c),			KS_KP_3,

	KC(0x5d),			KS_Caps_Lock,
	KC(0x5e),	KS_Cmd2,	KS_Alt_L,
	KC(0x5f),			KS_space,
	KC(0x60),			KS_Alt_R,
	KC(0x61),			KS_Control_R,
	KC(0x62),			KS_Left,
	KC(0x63),			KS_Down,
	KC(0x64),			KS_Right,
	KC(0x65),			KS_KP_0,
	KC(0x66),			KS_KP_Decimal,
	KC(0x67),			KS_KP_Enter,

	/* Mouse buttons */
	KC(0x70),			KS_voidSymbol, /* left */
	KC(0x71),			KS_voidSymbol, /* middle */
	KC(0x72),			KS_voidSymbol, /* right */
};

static const keysym_t arckbd_keydesc_no[] = {
/* Based on uk */
	KC(0x12),	KS_2,		KS_quotedbl,	KS_apostrophe,
	KC(0x14),	KS_4,		KS_dollar,	KS_currency,
	KC(0x16),	KS_6,		KS_ampersand,
	KC(0x17),	KS_7,		KS_slash,	KS_braceleft,
	KC(0x18),	KS_8,		KS_parenleft,	KS_bracketleft,
	KC(0x19),	KS_9,		KS_parenright,	KS_bracketright,
	KC(0x1a),	KS_0,		KS_equal,	KS_braceright,
	KC(0x1b),	KS_plus,	KS_question,
	KC(0x1c),	KS_less,	KS_greater,	KS_bar,
	KC(0x1d),	KS_at,		KS_sterling,
	KC(0x31),	KS_aring,
	KC(0x32),	KS_asciicircum,	KS_asterisk,
	KC(0x45),	KS_oslash,
	KC(0x46),	KS_ae,
	KC(0x4d),	KS_less,	KS_greater,
	KC(0x55),	KS_comma,	KS_semicolon,
	KC(0x56),	KS_period,	KS_colon,
	KC(0x57),	KS_minus,	KS_underscore,
};

static const keysym_t arckbd_keydesc_se[] = {
/* Based on no */
/*	pos		normal		shifted 	altgred */
	KC(0x32),	KS_udiaeresis,
	KC(0x33),	KS_asciicircum,	KS_asterisk,	KS_backslash,
	KC(0x45),	KS_odiaeresis,
	KC(0x46),	KS_adiaeresis,
};

static const keysym_t arckbd_keydesc_de[] = {
/* Based on se */
/*	pos		normal		shifted 	altgred */
	KC(0x10),	KS_asciicircum,	KS_degree,
	KC(0x12),	KS_2,		KS_quotedbl,
	KC(0x13),	KS_3,		KS_section,
	KC(0x14),	KS_4,		KS_dollar,
	KC(0x1b),	KS_ssharp,	KS_question,	KS_backslash,
	KC(0x1c),	KS_dead_acute,	KS_dead_grave,
	KC(0x27),	KS_q,		KS_Q,		KS_at,
	KC(0x2c),	KS_z,
	KC(0x31),	KS_udiaeresis,
	KC(0x32),	KS_plus,	KS_asterisk,	KS_asciitilde,
	KC(0x33),	KS_numbersign,	KS_apostrophe,
	KC(0x4e),	KS_y,
	KC(0x54),	KS_m,		KS_M,		KS_mu,
};

#define KBD_MAP(name, base, map) \
			{ name, base, sizeof(map)/sizeof(keysym_t), map }

static const struct wscons_keydesc arckbd_keydesctab[] = {
	KBD_MAP(KB_UK,			0,	arckbd_keydesc_uk),
	KBD_MAP(KB_NO,			KB_UK,	arckbd_keydesc_no),
	KBD_MAP(KB_SV,			KB_NO,	arckbd_keydesc_se),
	KBD_MAP(KB_DE,			KB_SV,	arckbd_keydesc_de),
	{0},
};

const struct wskbd_mapdata arckbd_mapdata_default = {
	arckbd_keydesctab, KB_UK
};

const struct arckbd_kbidtab arckbd_kbidtab[] = {
	{ ARCKBD_KBID_UK,	KB_UK },
	{ ARCKBD_KBID_NO,	KB_NO },
	{ ARCKBD_KBID_SE,	KB_SV },
	{ ARCKBD_KBID_DE,	KB_DE },
	{ -1 },
};
