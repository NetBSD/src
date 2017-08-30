/* $NetBSD: linux_keymap.c,v 1.1 2017/08/30 00:39:27 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: linux_keymap.c,v 1.1 2017/08/30 00:39:27 jmcneill Exp $");

#include <sys/types.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/wscons/linux_keymap.h>

#define KC(n) KS_KEYCODE(n)

static const keysym_t linux_keymap_keydesc_us[] = {
/*  pos      command		normal		shifted */
    KC(1),   KS_Cmd_Debugger,	KS_Escape,
    KC(2),  			KS_1,		KS_exclam,
    KC(3),  			KS_2,		KS_at,
    KC(4),  			KS_3,		KS_numbersign,
    KC(5),  			KS_4,		KS_dollar,
    KC(6),  			KS_5,		KS_percent,
    KC(7),  			KS_6,		KS_asciicircum,
    KC(8),  			KS_7,		KS_ampersand,
    KC(9),  			KS_8,		KS_asterisk,
    KC(10), 			KS_9,		KS_parenleft,
    KC(11), 			KS_0,		KS_parenright,
    KC(12), 			KS_minus,	KS_underscore,
    KC(13), 			KS_equal,	KS_plus,
    KC(14),  KS_Cmd_ResetEmul,	KS_Delete,
    KC(15), 			KS_Tab,
    KC(16), 			KS_q,
    KC(17), 			KS_w,
    KC(18), 			KS_e,
    KC(19), 			KS_r,
    KC(20), 			KS_t,
    KC(21), 			KS_y,
    KC(22), 			KS_u,
    KC(23), 			KS_i,
    KC(24), 			KS_o,
    KC(25), 			KS_p,
    KC(26), 			KS_bracketleft,	KS_braceleft,
    KC(27), 			KS_bracketright, KS_braceright,
    KC(28), 			KS_Return,
    KC(29),  KS_Cmd1,		KS_Control_L,
    KC(30), 			KS_a,
    KC(31), 			KS_s,
    KC(32), 			KS_d,
    KC(33), 			KS_f,
    KC(34), 			KS_g,
    KC(35), 			KS_h,
    KC(36), 			KS_j,
    KC(37), 			KS_k,
    KC(38), 			KS_l,
    KC(39), 			KS_semicolon,	KS_colon,
    KC(40), 			KS_apostrophe,	KS_quotedbl,
    KC(41), 			KS_grave,	KS_asciitilde,
    KC(42), 			KS_Shift_L,
    KC(43), 			KS_backslash,	KS_bar,
    KC(44), 			KS_z,
    KC(45), 			KS_x,
    KC(46), 			KS_c,
    KC(47), 			KS_v,
    KC(48), 			KS_b,
    KC(49), 			KS_n,
    KC(50), 			KS_m,
    KC(51), 			KS_comma,	KS_less,
    KC(52), 			KS_period,	KS_greater,
    KC(53), 			KS_slash,	KS_question,
    KC(54), 			KS_Shift_R,
    KC(55), 			KS_KP_Multiply,
    KC(56),  KS_Cmd2,		KS_Alt_L,
    KC(57), 			KS_space,
    KC(58), 			KS_Caps_Lock,
    KC(59),  KS_Cmd_Screen0,	KS_f1,
    KC(60),  KS_Cmd_Screen1,	KS_f2,
    KC(61),  KS_Cmd_Screen2,	KS_f3,
    KC(62),  KS_Cmd_Screen3,	KS_f4,
    KC(63),  KS_Cmd_Screen4,	KS_f5,
    KC(64),  KS_Cmd_Screen5,	KS_f6,
    KC(65),  KS_Cmd_Screen6,	KS_f7,
    KC(66),  KS_Cmd_Screen7,	KS_f8,
    KC(67),  KS_Cmd_Screen8,	KS_f9,
    KC(68),  KS_Cmd_Screen9,	KS_f10,
    KC(69), 			KS_Num_Lock,
    KC(70), 			KS_Hold_Screen,
    KC(71), 			KS_KP_Home,	KS_KP_7,
    KC(72), 			KS_KP_Up,	KS_KP_8,
    KC(73), KS_Cmd_ScrollFastUp, KS_KP_Prior,	KS_KP_9,
    KC(74), 			KS_KP_Subtract,
    KC(75), 			KS_KP_Left,	KS_KP_4,
    KC(76), 			KS_KP_Begin,	KS_KP_5,
    KC(77), 			KS_KP_Right,	KS_KP_6,
    KC(78), 			KS_KP_Add,
    KC(79), 			KS_KP_End,	KS_KP_1,
    KC(80), 			KS_KP_Down,	KS_KP_2,
    KC(81), KS_Cmd_ScrollFastDown, KS_KP_Next,	KS_KP_3,
    KC(82), 			KS_KP_Insert,	KS_KP_0,
    KC(83), 			KS_KP_Delete,	KS_KP_Decimal,
    KC(87), 			KS_f11,
    KC(88), 			KS_f12,
    KC(96),			KS_KP_Enter,
    KC(97),			KS_Control_R,
    KC(98),			KS_KP_Divide,
    KC(99),			KS_Print_Screen,
    KC(100),			KS_Alt_R,	KS_Multi_key,
    KC(102),			KS_Home,
    KC(103),			KS_Up,
    KC(104), KS_Cmd_ScrollFastUp, KS_Prior,
    KC(105),			KS_Left,
    KC(106),			KS_Right,
    KC(107),			KS_End,
    KC(108),			KS_Down,
    KC(109), KS_Cmd_ScrollFastDown, KS_Next,
    KC(110),			KS_Insert,
    KC(111),			KS_Delete,
    KC(113),			KS_Cmd_VolumeToggle,
    KC(114),			KS_Cmd_VolumeDown,
    KC(115),			KS_Cmd_VolumeUp,
    KC(119),			KS_Pause, /* Break */
    KC(125),			KS_Meta_L,
    KC(126),			KS_Meta_R,
    KC(128),			KS_Stop,
    KC(129),			KS_Again,
    KC(130),			KS_Props,
    KC(131),			KS_Undo,
    KC(132),			KS_Front,
    KC(133),			KS_Copy,
    KC(134),			KS_Open,
    KC(135),			KS_Paste,
    KC(136),			KS_Find,
    KC(137),			KS_Cut,
    KC(138),			KS_Help,
    KC(139),			KS_Menu,
};

#define KBD_MAP(name, base, map) \
			{ name, base, sizeof(map)/sizeof(keysym_t), map }

const struct wscons_keydesc linux_keymap_keydesctab[] = {
	KBD_MAP(KB_US,			0,	linux_keymap_keydesc_us),

	{0, 0, 0, 0}
};

#undef KBD_MAP
#undef KC
