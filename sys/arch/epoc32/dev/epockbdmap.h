/*	$NetBSD: epockbdmap.h,v 1.1 2013/04/28 12:11:25 kiyohara Exp $	*/
/*
 * Copyright (c) 2013 KIYOHARA Takashi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#define KC(n) KS_KEYCODE(n)

static const keysym_t epockbd_keysym_us[] = {
/*  pos      normal		shifted		altgr		shift+altgr  */
    KC(1),   KS_6,		KS_asciicircum,	KS_greater,
    KC(2),   KS_5,		KS_percent,	KS_less,
    KC(3),   KS_4,		KS_dollar,	KS_slash,	KS_Cmd_Screen3,
    KC(4),   KS_3,		KS_numbersign,	KS_backslash,	KS_Cmd_Screen2,
    KC(5),   KS_2,		KS_at,		KS_asciitilde,	KS_Cmd_Screen1,
    KC(6),   KS_1,		KS_exclam,	KS_underscore,	KS_Cmd_Screen0,
//  KC(7),   REC
    KC(9),   KS_colon,		KS_quotedbl,	KS_semicolon,
    KC(10),  KS_Delete,		KS_BackSpace,
    KC(11),  KS_0,		KS_parenright,	KS_braceright,
    KC(12),  KS_9,		KS_parenleft,	KS_braceleft,
    KC(13),  KS_8,		KS_asterisk,	KS_bracketright,
    KC(14),  KS_7,		KS_ampersand,	KS_bracketleft,
//  KC(15),  Play
    KC(17),  KS_y,		KS_voidSymbol,	KS_asterisk,
    KC(18),  KS_t,
    KC(19),  KS_r,
    KC(20),  KS_e,
    KC(21),  KS_w,
    KC(22),  KS_q,
    KC(23),  KS_Escape,
    KC(25),  KS_Return,
    KC(26),  KS_l,
    KC(27),  KS_p,		KS_P,		KS_equal,
    KC(28),  KS_o,		KS_O,		KS_minus,
    KC(29),  KS_i,		KS_I,		KS_plus,
    KC(30),  KS_u,		KS_U,		KS_slash,
    KC(31),  KS_Menu,
    KC(33),  KS_g,
    KC(34),  KS_f,
    KC(35),  KS_d,
    KC(36),  KS_s,
    KC(37),  KS_a,
    KC(38),  KS_Tab,		KS_Tab,		KS_Caps_Lock,
    KC(39),  KS_Control_L,
    KC(41),  KS_Down,
    KC(42),  KS_period,		KS_apostrophe,	KS_plus,
    KC(43),  KS_m,		KS_M,		KS_minus,
    KC(44),  KS_k,
    KC(45),  KS_j,
    KC(46),  KS_h,
    KC(47),  KS_Mode_switch,	KS_Multi_key,
    KC(49),  KS_n,
    KC(50),  KS_b,
    KC(51),  KS_v,
    KC(52),  KS_c,
    KC(53),  KS_x,
    KC(54),  KS_z,
    KC(55),  KS_Shift_R,
    KC(57),  KS_Right,		KS_Right,	KS_End,
    KC(58),  KS_Left,		KS_Left,	KS_Home,
    KC(59),  KS_comma,		KS_question,	KS_Help,
    KC(60),  KS_Up,
    KC(61),  KS_space,		KS_space,	KS_Cmd_BacklightToggle,
//  KC(62),  Stop
    KC(63),  KS_Shift_L,
};

static const keysym_t epockbd_keysym_uk[] = {
/*  pos      normal		shifted		altgr  */
    KC(3),   KS_4,		KS_dollar,	KS_at,
    KC(4),   KS_3,		KS_sterling,	KS_backslash,
    KC(5),   KS_2,		KS_quotedbl,	KS_numbersign,
    KC(9),   KS_apostrophe,	KS_asciitilde,	KS_colon,
    KC(26),  KS_l,		KS_L,		KS_semicolon,
    KC(42),  KS_period,		KS_question,	KS_plus,
    KC(59),  KS_comma,		KS_slash,	KS_Help,
};

#define KBD_MAP(name, base, map) \
			{ name, base, sizeof(map)/sizeof(keysym_t), map }

struct wscons_keydesc epockbd_keydesctab[] = {
	KBD_MAP(KB_UK,		KB_US,		epockbd_keysym_uk),
	KBD_MAP(KB_US,		0,		epockbd_keysym_us),
//	KBD_MAP(KB_DE,		KB_?,		epockbd_keysym_de),
//	KBD_MAP(KB_FR,		KB_?,		epockbd_keysym_fr),

	{ 0, 0, 0, NULL }
};
