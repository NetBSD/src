/*	$NetBSD: j720kbdmap.c,v 1.2 2001/07/20 04:28:50 hpeyerl Exp $	*/

/*-
 * Copyright (c) 1997, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/types.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/pckbc/wskbdmap_mfii.h>

#define KC(n) KS_KEYCODE(n)

static const keysym_t j720kbd_keydesc_us[] = {
/*  pos      command		normal		shifted */
    KC(1),   KS_Cmd_Debugger,	KS_Escape,
    KC(2),   KS_Cmd_Screen0,	KS_f1,
    KC(3),   KS_Cmd_Screen1,	KS_f2,
    KC(4),   KS_Cmd_Screen2,	KS_f3,
    KC(5),   KS_Cmd_Screen3,	KS_f4,
    KC(6),   KS_Cmd_Screen4,	KS_f5,
    KC(7),   KS_Cmd_Screen5,	KS_f6,
    KC(8),   KS_Cmd_Screen6,	KS_f7,
    KC(9),   KS_Cmd_Screen7,	KS_f8,
    KC(17),  			KS_1,		KS_exclam,
    KC(18),  			KS_2,		KS_at,
    KC(19),  			KS_3,		KS_numbersign,
    KC(20),  			KS_4,		KS_dollar,
    KC(21),  			KS_5,		KS_percent,
    KC(22),  			KS_6,		KS_asciicircum,
    KC(23),  			KS_7,		KS_ampersand,
    KC(24),  			KS_8,		KS_asterisk,
    KC(25), 			KS_9,		KS_parenleft,
    KC(26), 			KS_0,		KS_parenright,
    KC(27), 			KS_minus,	KS_underscore,
    KC(28), 			KS_equal,	KS_plus,
    KC(33), 			KS_q,
    KC(34), 			KS_w,
    KC(35), 			KS_e,
    KC(36), 			KS_r,
    KC(37), 			KS_t,
    KC(38), 			KS_y,
    KC(39), 			KS_u,
    KC(40), 			KS_i,
    KC(41), 			KS_o,
    KC(42), 			KS_p,
    KC(43), 			KS_backslash,	KS_bar,
    KC(44),			KS_BackSpace,
    KC(49), 			KS_a,
    KC(50), 			KS_s,
    KC(51), 			KS_d,
    KC(52), 			KS_f,
    KC(53), 			KS_g,
    KC(54), 			KS_h,
    KC(55), 			KS_j,
    KC(56), 			KS_k,
    KC(57), 			KS_l,
    KC(65), 			KS_z,
    KC(66), 			KS_x,
    KC(67), 			KS_c,
    KC(68), 			KS_v,
    KC(69), 			KS_b,
    KC(70), 			KS_n,
    KC(71), 			KS_m,
    KC(72), 			KS_comma,	KS_less,
    KC(73), 			KS_period,	KS_greater,
    KC(76), 			KS_Return,
    KC(81), 			KS_Tab,
    KC(83), 			KS_Shift_L,
    KC(92), 			KS_Shift_R,
    KC(101), KS_Cmd2,		KS_Alt_L,

    KC(113),			KS_Meta_L,
    KC(114),  KS_Cmd1,		KS_Control_L,
    KC(116), 			KS_space,
    KC(120), 			KS_slash,	KS_question,
    KC(121),  KS_Cmd_ResetEmul,	KS_Delete,

    /* below are guesses from Japanese model key layout */
    KC(59),			KS_bracketright, KS_braceright,
    KC(58), 			KS_semicolon,	KS_colon,
    KC(75), 			KS_apostrophe,	KS_quotedbl,

    KC(90),			KS_Up,
    KC(105),			KS_Left,
    KC(107),			KS_Right,
    KC(106),			KS_Down,
};

static const keysym_t j720kbd_keydesc_jp[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(18),  KS_2,              KS_quotedbl,
    KC(22),  KS_6,              KS_ampersand,
    KC(23),  KS_7,              KS_apostrophe,
    KC(24),  KS_8,              KS_parenleft,
    KC(25),  KS_9,              KS_parenright,
    KC(26),  KS_0,
    KC(27),  KS_minus,          KS_equal,
    KC(28),  KS_asciicircum,    KS_asciitilde,
    KC(43),  KS_at,             KS_grave,
    KC(59),  KS_bracketleft,    KS_braceleft,
    KC(58),  KS_semicolon,      KS_plus,
    KC(75),  KS_colon,          KS_asterisk,
    KC(12),  KS_Zenkaku_Hankaku, /* replace grave/tilde */
    KC(60),  KS_bracketright,   KS_braceright,
    KC(11),  KS_Hiragana_Katakana,
    KC(120), KS_backslash,      KS_underscore,

    KC(119), KS_Henkan,
    KC(102), KS_Muhenkan,
    KC(91),  KS_backslash,      KS_bar,
};

#define KBD_MAP(name, base, map) \
			{ name, base, sizeof(map)/sizeof(keysym_t), map }
/* KBD_NULLMAP generates a entry for machine native variant.
   the entry will be modified by machine dependent keyboard driver. */
#define KBD_NULLMAP(name, base) { name, base, 0, 0 }

const struct wscons_keydesc j720kbd_keydesctab[] = {
	KBD_MAP(KB_US,			0,	j720kbd_keydesc_us),
	KBD_MAP(KB_JP,			KB_US,	j720kbd_keydesc_jp),
	{0, 0, 0, 0}
};

#undef KBD_MAP
#undef KC
