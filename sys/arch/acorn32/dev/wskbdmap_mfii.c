/*	$NetBSD: wskbdmap_mfii.c,v 1.1 2001/10/05 22:27:48 reinoud Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk
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
 *
 * Created the RiscPC keyboard table on base of the XT table origionaly
 * created by Juergen Hannken-Illjes.
 */

#include <sys/types.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <arm/iomd/wskbdmap_mfii.h>

#define KC(n) KS_KEYCODE(n)

static const keysym_t rpckbd_keydesc_uk[] = {
/*  pos      command		normal		shifted		altgr		shift-altgr */
    KC(118), KS_Cmd_Debugger,	KS_Escape,

    KC(22),			KS_1,		KS_exclam,	KS_plusminus,	KS_exclamdown,
    KC(30),			KS_2,		KS_quotedbl,	KS_twosuperior,	KS_cent,
    KC(38),			KS_3,		KS_sterling,	KS_threesuperior,
    KC(37),			KS_4,		KS_dollar,	KS_acute,	KS_currency,
    KC(46),			KS_5,		KS_percent,	KS_mu,		KS_yen,
    KC(54),			KS_6,		KS_asciicircum,	KS_paragraph,
    KC(61),			KS_7,		KS_ampersand,	KS_periodcentered,
    KC(62),			KS_8,		KS_asterisk,	KS_cedilla,	KS_ordfeminine,
    KC(70),			KS_9,		KS_parenleft,	KS_onesuperior,	KS_diaeresis,
    KC(69),			KS_0,		KS_parenright,	KS_masculine,	KS_copyright,
    KC(78),			KS_minus,	KS_underscore,	KS_hyphen,	KS_ssharp,
    KC(85),			KS_equal,	KS_plus,	KS_onehalf,	KS_guillemotleft,
    KC(14),			KS_grave,	KS_notsign,	KS_brokenbar,
    KC(102), KS_Cmd_ResetEmul,	KS_Delete,
    KC(13), 			KS_Tab,
    KC(21), 			KS_q,
    KC(29), 			KS_w,
    KC(36), 			KS_e,
    KC(45), 			KS_r,
    KC(44), 			KS_t,
    KC(53), 			KS_y,
    KC(60), 			KS_u,
    KC(67), 			KS_i,
    KC(68), 			KS_o,
    KC(77), 			KS_p,
    KC(84), 			KS_bracketleft,	KS_braceleft,
    KC(91), 			KS_bracketright, KS_braceright,
    KC(90), 			KS_Return,
    KC(20),  KS_Cmd1,		KS_Control_L,
    KC(28), 			KS_a,
    KC(27), 			KS_s,
    KC(35), 			KS_d,
    KC(43), 			KS_f,
    KC(52), 			KS_g,
    KC(51), 			KS_h,
    KC(59), 			KS_j,
    KC(66), 			KS_k,
    KC(75), 			KS_l,
    KC(76), 			KS_semicolon,	KS_colon,
    KC(82),			KS_apostrophe,	KS_at,		KS_section,	KS_Agrave,
    KC(93),			KS_numbersign,	KS_asciitilde,	KS_sterling,	KS_thorn,
    KC(18), 			KS_Shift_L,
    KC(97),			KS_backslash,	KS_bar,
    KC(26), 			KS_z,
    KC(34), 			KS_x,
    KC(33), 			KS_c,
    KC(42), 			KS_v,
    KC(50), 			KS_b,
    KC(49), 			KS_n,
    KC(58), 			KS_m,
    KC(65), 			KS_comma,	KS_less,
    KC(73), 			KS_period,	KS_greater,
    KC(74), 			KS_slash,	KS_question,
    KC(89), 			KS_Shift_R,
    KC(124), 			KS_KP_Multiply,
    KC(17),  KS_Cmd2,		KS_Alt_L,	KS_Meta_L,
    KC(41), 			KS_space,
    KC(88), 			KS_Caps_Lock,
    KC(5),   KS_Cmd_Screen0,	KS_f1,
    KC(6),   KS_Cmd_Screen1,	KS_f2,
    KC(4),   KS_Cmd_Screen2,	KS_f3,
    KC(12),  KS_Cmd_Screen3,	KS_f4,
    KC(3),   KS_Cmd_Screen4,	KS_f5,
    KC(11),  KS_Cmd_Screen5,	KS_f6,
    KC(131), KS_Cmd_Screen6,	KS_f7,
    KC(10),  KS_Cmd_Screen7,	KS_f8,
    KC(1),   KS_Cmd_Screen8,	KS_f9,
    KC(9),   KS_Cmd_Screen9,	KS_f10,
    KC(119), 			KS_Num_Lock,
    KC(126), 			KS_Hold_Screen,
    KC(108), 			KS_KP_Home,	KS_KP_7,
    KC(117), 			KS_KP_Up,	KS_KP_8,
    KC(125), 			KS_KP_Prior,	KS_KP_9,
    KC(123), 			KS_KP_Subtract,
    KC(107), 			KS_KP_Left,	KS_KP_4,
    KC(115), 			KS_KP_Begin,	KS_KP_5,
    KC(116), 			KS_KP_Right,	KS_KP_6,
    KC(121), 			KS_KP_Add,
    KC(105), 			KS_KP_End,	KS_KP_1,
    KC(114), 			KS_KP_Down,	KS_KP_2,
    KC(122), 			KS_KP_Next,	KS_KP_3,
    KC(112), 			KS_KP_Insert,	KS_KP_0,
    KC(113), 			KS_KP_Delete,	KS_KP_Decimal,
    KC(120), 			KS_f11,
    KC(7), 			KS_f12,
    KC(127),			KS_Pause, /* Break */		/* XXX fixme XXX */
    KC(346),			KS_KP_Enter,
    KC(276),			KS_Control_R,
    KC(380),			KS_Print_Screen,
    KC(330),			KS_KP_Divide,
    KC(380),			KS_Print_Screen,
/*  pos      command		normal		shifted		altgr		shift-altgr */
    KC(273),			KS_Mode_switch,	KS_Mode_switch,	KS_Mode_switch, KS_Multi_key,
#if 0
    KC(198),  KS_Cmd_ResetClose, /* CTL-Break */		/* XXX fixme XXX */
#endif
    KC(364),			KS_Home,
    KC(373),			KS_Up,
    KC(381),			KS_Prior,
    KC(363),			KS_Left,
    KC(372),			KS_Right,
    KC(361),			KS_End,
    KC(370),			KS_Down,
    KC(378),			KS_Next,
    KC(368),			KS_Insert,
    KC(369),			KS_Delete,
#if 0	/* dunno what to map this one too */
    KC(221),			KS_Menu,
#endif
};


#define KBD_MAP(name, base, map) \
			{ name, base, sizeof(map)/sizeof(keysym_t), map }
/* KBD_NULLMAP generates a entry for machine native variant.
   the entry will be modified by machine dependent keyboard driver. */
#define KBD_NULLMAP(name, base) { name, base, 0, 0 }

const struct wscons_keydesc rpckbd_keydesctab[] = {
	KBD_MAP(KB_UK,			0,	rpckbd_keydesc_uk),
	{0, 0, 0, 0}
};

#undef KBD_MAP
#undef KC

