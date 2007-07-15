/*	$NetBSD: zkbdmap.h,v 1.2.12.1 2007/07/15 13:17:24 ad Exp $	*/
/* $OpenBSD: zaurus_kbdmap.h,v 1.19 2005/05/10 08:26:12 espie Exp $ */

/*
 * Copyright (c) 2005 Dale Rahn <drahn@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define KC(n) KS_KEYCODE(n)

static const keysym_t zkbd_keydesc_us[] = {
    KC(0),	KS_Control_L,
    KC(2),	KS_Tab,		KS_Tab,		KS_Caps_Lock,
    KC(3),	KS_Cmd_Screen1,	KS_f2,				/* Addr, */
    KC(4),	KS_Cmd_Screen0,	KS_f1,				/* Cal, */
    KC(5),	KS_Cmd_Screen2,	KS_f3,				/* Mail, */
    KC(6),	KS_Cmd_Screen3,	KS_f4,				/* Home, */
    KC(8),	KS_1,		KS_exclam,
    KC(9),	KS_2,		KS_quotedbl,
    KC(10),	KS_q,
    KC(11),	KS_w,		KS_W,		KS_asciicircum,
    KC(12),	KS_a,
    KC(13),	KS_z,
    KC(14),	KS_Cmd,		KS_Alt_L,
    KC(16),	KS_Cmd_BrightnessDown,	KS_3,	KS_numbersign,
    KC(17),	KS_Cmd_BrightnessUp,	KS_4,	KS_dollar,	
    KC(18),	KS_e,		KS_E,		KS_equal,
    KC(19),	KS_s,
    KC(20),	KS_d,		KS_D,		KS_grave,
    KC(21),	KS_x,
    /* KC(22),	^/t (right japanese) */
    KC(24),	KS_5,		KS_percent,
    KC(25),	KS_r,		KS_R,		KS_plus,
    KC(26),	KS_t,		KS_T,		KS_bracketleft,
    KC(27),	KS_f,		KS_F,		KS_backslash,
    KC(28),	KS_c,
    KC(29),	KS_minus,	KS_minus,	KS_at,
    KC(30),	KS_Cmd_Debugger,KS_Escape,	/* Cancel */
    KC(32),	KS_6,		KS_ampersand,
    KC(33),	KS_y,		KS_Y,		KS_bracketright,
    KC(34),	KS_g,		KS_G,		KS_semicolon,
    KC(35),	KS_v,
    KC(36),	KS_b,		KS_B,		KS_underscore,
    KC(37),	KS_space,
    KC(38),	KS_KP_Enter,	/* ok */
    KC(40),	KS_7,		KS_apostrophe,
    KC(41),	KS_8,		KS_parenleft,
    KC(42),	KS_u,		KS_U,		KS_braceleft,	
    KC(43),	KS_h,		KS_H,		KS_colon,
    KC(44),	KS_n,
    KC(45),	KS_comma,	KS_slash,	KS_less,
    KC(46),	KS_Cmd_Screen4,	KS_f5,				/* Menu, */
    KC(48),	KS_9,		KS_parenright,
    KC(49),	KS_i,		KS_I,		KS_braceright,
    KC(50),	KS_j,		KS_J,		KS_asterisk,
    KC(51),	KS_m,
    KC(52),	KS_period,	KS_question,	KS_greater,
    KC(54),	KS_KP_Left,	KS_KP_Left,	KS_Home, /* left, */
    KC(56),	KS_0,		KS_asciitilde,
    KC(57),	KS_o,
    KC(58),	KS_k,
    KC(59),	KS_l,		KS_L,		KS_bar,
    KC(61),	KS_KP_Up,	KS_KP_Up,	KS_Prior, /* up, */
    KC(62),	KS_KP_Down,	KS_KP_Down,	KS_Next, /* down, */
    KC(64),	KS_Delete,	KS_BackSpace,
    KC(65),	KS_p,
    KC(68),	KS_Return,
    KC(70),	KS_KP_Right,	KS_KP_Right,	KS_End, /* right, */
    KC(80),	KS_KP_Right, /* OK, (ext) */
    KC(81),	KS_KP_Down, /* tog left, */
    KC(83),	KS_Shift_R,
    KC(84),	KS_Shift_L,
    KC(88),	KS_KP_Left, /* cancel (ext), */
    KC(89),	KS_KP_Up, /* tog right, */
    KC(93),	KS_Mode_switch /* Fn */
};

#ifdef WSDISPLAY_COMPAT_RAWKBD
static const char xt_keymap[] = {
    /* KC(0), */	0x1d, /* KS_Control_L, */
    /* KC(1), */	0x00, /* NC */
    /* KC(2), */	0x0f, /* KS_Tab,	KS_Tab,		KS_Caps_Lock, */
    /* KC(3), */	0x3c, /* KS_Cmd_Screen1,	KS_f2,		Addr, */
    /* KC(4), */	0x3b, /* KS_Cmd_Screen0,	KS_f1,		Cal, */
    /* KC(5), */	0x3d, /* KS_Cmd_Screen2,	KS_f3,		Mail, */
    /* KC(6), */	0x3e, /* KS_Cmd_Screen3,	KS_f4,		Home, */
    /* KC(7), */	0x00, /* NC */
    /* KC(8), */	0x02, /* KS_1,	KS_exclam, */
    /* KC(9), */	0x03, /* KS_2,	KS_quotedbl, */
    /* KC(10), */	0x10, /* KS_q, */
    /* KC(11), */	0x11, /* KS_w,	KS_W,	KS_asciicircum, */
    /* KC(12), */	0x1e, /* KS_a, */
    /* KC(13), */	0x2c, /* KS_z, */
    /* KC(14), */	0x38, /* KS_Cmd,	KS_Alt_L, */
    /* KC(15), */	0x00, /* NC */
    /* KC(16), */	0x04, /* KS_3,	KS_numbersign, */
    /* KC(17), */	0x05, /* KS_4,	KS_dollar, */
    /* KC(18), */	0x12, /* KS_e,	KS_E,	KS_equal, */
    /* KC(19), */	0x1f, /* KS_s, */
    /* KC(20), */	0x20, /* KS_d,	KS_D,	KS_grave, */
    /* KC(21), */	0x2d, /* KS_x, */
    /* KC(22), */	0x00, /* ^/t (right japanese) */
    /* KC(23), */	0x00, /* NC */
    /* KC(24), */	0x06, /* KS_5,	KS_percent, */
    /* KC(25), */	0x13, /* KS_r,	KS_R,	KS_plus, */
    /* KC(26), */	0x14, /* KS_t,	KS_T,	KS_bracketleft, */
    /* KC(27), */	0x21, /* KS_f,	KS_F,	KS_backslash, */
    /* KC(28), */	0x2e, /* KS_c, */
    /* KC(29), */	0x0c, /* KS_minus, KS_minus,	KS_at, */
    /* KC(30), */	0x01, /* KS_Escape, Cancel */
    /* KC(31), */	0x00, /* NC */
    /* KC(32), */	0x07, /* KS_6,	KS_ampersand, */
    /* KC(33), */	0x15, /* KS_y,	KS_Y,	KS_bracketright, */
    /* KC(34), */	0x22, /* KS_g,	KS_G,	KS_semicolon, */
    /* KC(35), */	0x2f, /* KS_v, */
    /* KC(36), */	0x30, /* KS_b,	KS_B,	KS_underscore, */
    /* KC(37), */	0x39, /* KS_space, */
    /* KC(38), */	0x9c, /* KS_KP_Enter,	ok */
    /* KC(39), */	0x00, /* NC */
    /* KC(40), */	0x08, /* KS_7,	KS_apostrophe, */
    /* KC(41), */	0x09, /* KS_8,	KS_parenleft, */
    /* KC(42), */	0x16, /* KS_u,	KS_U,	KS_braceleft, */
    /* KC(43), */	0x23, /* KS_h,	KS_H,	KS_colon, */
    /* KC(44), */	0x31, /* KS_n, */
    /* KC(45), */	0x33, /* KS_comma, KS_slash,	KS_less, */
    /* KC(46), */	0x3f, /* KS_Cmd_Screen4,	KS_f5,	Menu, */
    /* KC(47), */	0x00, /* NC */
    /* KC(48), */	0x0a, /* KS_9,	KS_parenright, */
    /* KC(49), */	0x17, /* KS_i,	KS_I,	KS_braceright, */
    /* KC(50), */	0x24, /* KS_j,	KS_J,	KS_asterisk, */
    /* KC(51), */	0x32, /* KS_m, */
    /* KC(52), */	0x34, /* KS_period, KS_question, KS_greater, */
    /* KC(53), */	0x00, /* NC */
    /* KC(54), */	0xcb, /* KS_KP_Left, left, */
    /* KC(55), */	0x00, /* NC */
    /* KC(56), */	0x0b, /* KS_0,	KS_asciitilde, */
    /* KC(57), */	0x18, /* KS_o, */
    /* KC(58), */	0x25, /* KS_k, */
    /* KC(59), */	0x26, /* KS_l,	KS_L,	KS_bar, */
    /* KC(60), */	0x00, /* NC */
    /* KC(61), */	0xc8, /* KS_KP_Up, up, */
    /* KC(62), */	0xd0, /* KS_KP_Down, down, */
    /* KC(63), */	0x00, /* NC */
    /* KC(64), */	0x0e, /* KS_Delete,	KS_BackSpace, */
    /* KC(65), */	0x19, /* KS_p, */
    /* KC(66), */	0x00, /* NC */
    /* KC(67), */	0x00, /* NC */
    /* KC(68), */	0x1c, /* KS_Return, */
    /* KC(69), */	0x00, /* NC */
    /* KC(70), */	0xcd, /* KS_KP_Right, right, */
    /* KC(71), */	0x00, /* NC */
    /* KC(72), */	0x00, /* NC */
    /* KC(73), */	0x00, /* NC */
    /* KC(74), */	0x00, /* NC */
    /* KC(75), */	0x00, /* NC */
    /* KC(76), */	0x00, /* NC */
    /* KC(77), */	0x00, /* NC */
    /* KC(78), */	0x00, /* NC */
    /* KC(79), */	0x00, /* NC */
    /* KC(80), */	0xcd, /* KS_KP_Right, OK, (ext) */
    /* KC(81), */	0xd0, /* KS_KP_Down, tog left, */
    /* KC(82), */	0x00, /* NC */
    /* KC(83), */	0x36, /* KS_Shift_R, */
    /* KC(84), */	0x2a, /* KS_Shift_L, */
    /* KC(85), */	0x00, /* NC */
    /* KC(86), */	0x00, /* NC */
    /* KC(87), */	0x00, /* NC */
    /* KC(88), */	0xcb, /* KS_KP_Left, cancel (ext), */
    /* KC(89), */	0xc8, /* KS_KP_Up, tog right, */
    /* KC(90), */	0x00, /* NC */
    /* KC(91), */	0x00, /* NC */
    /* KC(92), */	0x00, /* NC */
    /* KC(93), */	0xb8, /* KS_Mode_switch Fn */
};
#endif

#define KBD_MAP(name, base, map) \
			{ (name), (base), __arraycount(map), (map) }

static const struct wscons_keydesc zkbd_keydesctab[] = {
	KBD_MAP(KB_US,			0,	zkbd_keydesc_us),

	{0, 0, 0, 0}
};

#undef KBD_MAP
#undef KC
