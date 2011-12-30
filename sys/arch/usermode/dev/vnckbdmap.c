/* $NetBSD: vnckbdmap.c,v 1.2 2011/12/30 12:14:44 jmcneill Exp $ */

/*
 * Copyright (c) 1999,2001,2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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
__KERNEL_RCSID(0, "$NetBSD: vnckbdmap.c,v 1.2 2011/12/30 12:14:44 jmcneill Exp $");

#include <sys/types.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#define KC(n)		KS_KEYCODE(n)

static const keysym_t vnckbd_keydesc_us[] = {
/*  pos      command		normal		shifted */

    /* X11 TTY function keys */
    KC(0xf08),			KS_BackSpace,
    KC(0xf09),			KS_Tab,
    KC(0xf0a),			KS_Linefeed,
    KC(0xf0b),			KS_Clear,
    KC(0xf0d),			KS_Return,
    KC(0xf13),			KS_Pause,
    KC(0xf14),			KS_Hold_Screen,
//  KC(0xf15),			KS_SysReq,
    KC(0xf1b),	KS_Cmd_Debugger, KS_Escape,
    KC(0xfff),			KS_Delete,

    /* X11 cursor control & motion */
    KC(0xf50),			KS_Home,
    KC(0xf51),			KS_Left,
    KC(0xf52),			KS_Up,
    KC(0xf53),			KS_Right,
    KC(0xf54),			KS_Down,
    KC(0xf55),	KS_Cmd_ScrollFastUp, KS_Prior,
    KC(0xf56),	KS_Cmd_ScrollFastDown, KS_Next,
    KC(0xf57),			KS_End,
//  KC(0xf58),			KS_Begin,

    /* X11 misc functions */
    KC(0xf60),			KS_Select,
    KC(0xf61),			KS_Print_Screen,
    KC(0xf62),			KS_Execute,
    KC(0xf63),			KS_Insert,
    KC(0xf65),			KS_Undo,
    KC(0xf66),			KS_Again,
    KC(0xf67),			KS_Menu,
    KC(0xf68),			KS_Find,
//  KC(0xf69),			KS_Cancel,
    KC(0xf6a),			KS_Help,
//  KC(0xf6b),			KS_Break,
    KC(0xf7e),			KS_Mode_switch,
    KC(0xf7f),			KS_Num_Lock,

    /* X11 keypad functions */
    /* ... */

    /* X11 auxiliary functions */
    KC(0xfbe),	KS_Cmd_Screen0,	KS_F1,
    KC(0xfbf),	KS_Cmd_Screen1,	KS_F2,
    KC(0xfc0),	KS_Cmd_Screen2,	KS_F3,
    KC(0xfc1),	KS_Cmd_Screen3,	KS_F4,
    KC(0xfc2),	KS_Cmd_Screen4,	KS_F5,
    KC(0xfc3),	KS_Cmd_Screen5,	KS_F6,
    KC(0xfc4),	KS_Cmd_Screen6,	KS_F7,
    KC(0xfc5),	KS_Cmd_Screen7,	KS_F8,
    KC(0xfc6),	KS_Cmd_Screen8,	KS_F9,
    KC(0xfc7),	KS_Cmd_Screen9,	KS_F10,
    KC(0xfc8),			KS_F11,
    KC(0xfc9),			KS_F12,
    KC(0xfca),			KS_F13,
    KC(0xfcb),			KS_F14,
    KC(0xfcc),			KS_F15,
    KC(0xfcd),			KS_F16,
    KC(0xfce),			KS_F17,
    KC(0xfcf),			KS_F18,
    KC(0xfd0),			KS_F19,
    KC(0xfd1),			KS_F20,
    /* ... F21 - F35 */

    /* X11 modifiers */
    KC(0xfe1),			KS_Shift_L,
    KC(0xfe2),			KS_Shift_R,
    KC(0xfe3),	KS_Cmd1,	KS_Control_L,
    KC(0xfe4),			KS_Control_R,
    KC(0xfe5),			KS_Caps_Lock,
    KC(0xfe6),			KS_Shift_Lock,
    KC(0xfe7),	KS_Cmd2,	KS_Meta_L,
    KC(0xfe8),			KS_Meta_R,
    KC(0xfe9),			KS_Alt_L,
    KC(0xfea),			KS_Alt_R,
//  KC(0xfeb),			KS_Super_L,
//  KC(0xfec),			KS_Super_R,
//  KC(0xfed),			KS_Hyper_L,
//  KC(0xfee),			KS_Hyper_R,

    /* Latin 1 */
    KC(0x20),			KS_space,
    KC(0x21),			KS_exclam,
    KC(0x22),			KS_quotedbl,
    KC(0x23),			KS_numbersign,
    KC(0x24),			KS_dollar,
    KC(0x25),			KS_percent,
    KC(0x26),			KS_ampersand,
    KC(0x27),			KS_apostrophe,
    KC(0x28),			KS_parenleft,
    KC(0x29),			KS_parenright,
    KC(0x2a),			KS_asterisk,
    KC(0x2b),			KS_plus,
    KC(0x2c),			KS_comma,
    KC(0x2d),			KS_minus,
    KC(0x2e),			KS_period,
    KC(0x2f),			KS_slash,
    KC(0x30), 			KS_0,
    KC(0x31),  			KS_1,
    KC(0x32),  			KS_2,
    KC(0x33),  			KS_3,
    KC(0x34),  			KS_4,
    KC(0x35),  			KS_5,
    KC(0x36),  			KS_6,
    KC(0x37),  			KS_7,
    KC(0x38),  			KS_8,
    KC(0x39), 			KS_9,
    KC(0x3a),			KS_colon,
    KC(0x3b),			KS_semicolon,
    KC(0x3c),			KS_less,
    KC(0x3d),			KS_equal,
    KC(0x3e),			KS_greater,
    KC(0x3f),			KS_question,
    KC(0x40),			KS_at,
    KC(0x41),			KS_A,
    KC(0x42),			KS_B,
    KC(0x43),			KS_C,
    KC(0x44),			KS_D,
    KC(0x45),			KS_E,
    KC(0x46),			KS_F,
    KC(0x47),			KS_G,
    KC(0x48),			KS_H,
    KC(0x49),			KS_I,
    KC(0x4a),			KS_J,
    KC(0x4b),			KS_K,
    KC(0x4c),			KS_L,
    KC(0x4d),			KS_M,
    KC(0x4e),			KS_N,
    KC(0x4f),			KS_O,
    KC(0x50),			KS_P,
    KC(0x51),			KS_Q,
    KC(0x52),			KS_R,
    KC(0x53),			KS_S,
    KC(0x54),			KS_T,
    KC(0x55),			KS_U,
    KC(0x56),			KS_V,
    KC(0x57),			KS_W,
    KC(0x58),			KS_X,
    KC(0x59),			KS_Y,
    KC(0x5a),			KS_Z,
    KC(0x5b),			KS_bracketleft,
    KC(0x5c),			KS_backslash,
    KC(0x5d),			KS_bracketright,
    KC(0x5e),			KS_asciicircum,
    KC(0x5f),			KS_underscore,
    KC(0x60),			KS_grave,
    KC(0x61), 			KS_a,
    KC(0x62), 			KS_b,
    KC(0x63), 			KS_c,
    KC(0x64), 			KS_d,
    KC(0x65), 			KS_e,
    KC(0x66), 			KS_f,
    KC(0x67), 			KS_g,
    KC(0x68), 			KS_h,
    KC(0x69), 			KS_i,
    KC(0x6a), 			KS_j,
    KC(0x6b), 			KS_k,
    KC(0x6c), 			KS_l,
    KC(0x6d), 			KS_m,
    KC(0x6e), 			KS_n,
    KC(0x6f), 			KS_o,
    KC(0x70), 			KS_p,
    KC(0x71), 			KS_q,
    KC(0x72), 			KS_r,
    KC(0x73), 			KS_s,
    KC(0x74), 			KS_t,
    KC(0x75), 			KS_u,
    KC(0x76), 			KS_v,
    KC(0x77), 			KS_w,
    KC(0x78), 			KS_x,
    KC(0x79), 			KS_y,
    KC(0x7a), 			KS_z,
    KC(0x7b),			KS_braceleft,
    KC(0x7c),			KS_bar,
    KC(0x7d),			KS_braceright,
    KC(0x7e),			KS_asciitilde,
    KC(0x7f),			KS_Delete,
/* TODO:
    KC(0xa0),			KS_nobreakspace,
    KC(0xa1),			KS_exclamdown,
    KC(0xa2),			KS_cent,
    KC(0xa3),			KS_sterling,
    ...
 */

};

#define KBD_MAP(name, base, map) \
			{ name, base, sizeof(map)/sizeof(keysym_t), map }

const struct wscons_keydesc vnckbd_keydesctab[] = {
	KBD_MAP(KB_US,			0,	vnckbd_keydesc_us),
	{0, 0, 0, 0}
};

#undef KBD_MAP
#undef KC
