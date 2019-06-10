/*	$NetBSD: wskbdmap_atari.c,v 1.4.88.1 2019/06/10 22:05:58 christos Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: wskbdmap_atari.c,v 1.4.88.1 2019/06/10 22:05:58 christos Exp $");

#include <sys/types.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#define KC(n) KS_KEYCODE(n)

static const keysym_t atarikbd_keydesc_us[] = {
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
    KC(71), 			KS_KP_Home,
    KC(72), 			KS_KP_Up,	KS_KP_8,
    KC(74), 			KS_KP_Subtract,
    KC(75), 			KS_KP_Left,	KS_KP_4,
    KC(77), 			KS_KP_Right,	KS_KP_6,
    KC(78), 			KS_KP_Add,
    KC(80), 			KS_KP_Down,	KS_KP_2,
    KC(82), 			KS_KP_Insert,	KS_KP_0,
    KC(83), 			KS_KP_Delete,	KS_KP_Decimal,
    KC(97), 			KS_Pause,	/* should be KS_Help. but this is not available */
    KC(98), 			KS_Help,
    KC(99), 			KS_parenleft,	/* should be KS_KP_parenleft */
    KC(100), 			KS_parenright,	/* should be KS_KP_parenright */
    KC(101), 			KS_slash,	/* should be KS_KP_slash */
    KC(102), 			KS_KP_Multiply,
    KC(103),			KS_KP_7,
    KC(104),			KS_KP_8,
    KC(105),			KS_KP_9,
    KC(106),			KS_KP_4,
    KC(107),			KS_KP_5,
    KC(108),			KS_KP_6,
    KC(109),			KS_KP_1,
    KC(110),			KS_KP_2,
    KC(111),			KS_KP_3,
    KC(112),			KS_KP_0,
    KC(113),			KS_KP_Decimal,
    KC(114),			KS_KP_Enter,
};

static const keysym_t atarikbd_keydesc_de[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(3),   KS_2,		KS_quotedbl,
    KC(4),   KS_3,		KS_section,
    KC(7),   KS_6,		KS_ampersand,
    KC(8),   KS_7,		KS_slash,
    KC(9),   KS_8,		KS_parenleft,
    KC(10),  KS_9,		KS_parenright,
    KC(11),  KS_0,		KS_equal,
    KC(12),  KS_ssharp,		KS_question,
    KC(13),  KS_dead_acute,	KS_dead_grave,
    KC(16),  KS_q,		KS_Q,	
    KC(21),  KS_z,
    KC(26),  KS_udiaeresis,	KS_Udiaeresis,	KS_at,		KS_backslash,
    KC(27),  KS_plus,		KS_asterisk,
    KC(39),  KS_odiaeresis,	KS_Odiaeresis,	KS_bracketleft,	KS_braceleft,
    KC(40),  KS_adiaeresis,	KS_Adiaeresis,	KS_bracketright,KS_braceright,
    KC(41),  KS_numbersign,	KS_dead_circumflex,
    KC(43),  KS_dead_tilde,	KS_bar,
    KC(44),  KS_y,
    KC(51),  KS_comma,		KS_semicolon,
    KC(52),  KS_period,		KS_colon,
    KC(53),  KS_minus,		KS_underscore,
    KC(56),  KS_Mode_switch, 	KS_Multi_key,
    KC(96),  KS_less,		KS_greater,
};

static const keysym_t atarikbd_keydesc_de_nodead[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(13),  KS_apostrophe,	KS_grave,
    KC(41),  KS_numbersign,	KS_asciicircum,
    KC(43),  KS_asciitilde,	KS_bar,
};

static const keysym_t atarikbd_keydesc_dk[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(3),   KS_2,		KS_quotedbl,	KS_at,
    KC(4),   KS_3,		KS_numbersign,	KS_sterling,
    KC(5),   KS_4,		KS_currency,	KS_dollar,
    KC(7),   KS_6,		KS_ampersand,
    KC(8),   KS_7,		KS_slash,	KS_braceleft,
    KC(9),   KS_8,		KS_parenleft,	KS_bracketleft,
    KC(10),  KS_9,		KS_parenright,	KS_bracketright,
    KC(11),  KS_0,		KS_equal,	KS_braceright,
    KC(12),  KS_plus,		KS_question,
    KC(13),  KS_dead_acute,	KS_dead_grave,	KS_bar,
    KC(26),  KS_aring,
    KC(27),  KS_dead_diaeresis,	KS_dead_circumflex, KS_dead_tilde,
    KC(39),  KS_ae,
    KC(40),  KS_oslash,
    KC(41),  KS_onehalf,	KS_paragraph,
    KC(43),  KS_apostrophe,	KS_asterisk,
    KC(51),  KS_comma,		KS_semicolon,
    KC(52),  KS_period,		KS_colon,
    KC(53),  KS_minus,		KS_underscore,
    KC(86),  KS_less,		KS_greater,	KS_backslash,
    KC(184), KS_Mode_switch,	KS_Multi_key,
};

static const keysym_t atarikbd_keydesc_dk_nodead[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(13),  KS_apostrophe,	KS_grave,	KS_bar,
    KC(27),  KS_diaeresis,	KS_asciicircum,	KS_asciitilde,
};

static const keysym_t atarikbd_keydesc_sv[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(12),  KS_plus,		KS_question,	KS_backslash,
    KC(27),  KS_dead_diaeresis,	KS_dead_circumflex, KS_dead_tilde,
    KC(39),  KS_odiaeresis,
    KC(40),  KS_adiaeresis,
    KC(41),  KS_paragraph,	KS_onehalf,
    KC(86),  KS_less,		KS_greater,	KS_bar,
    KC(184), KS_Mode_switch,	KS_Multi_key,
};

static const keysym_t atarikbd_keydesc_sv_nodead[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(13),  KS_apostrophe,	KS_grave,	KS_bar,
    KC(27),  KS_diaeresis,	KS_asciicircum,	KS_asciitilde,
};

static const keysym_t atarikbd_keydesc_no[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(13),  KS_backslash,	KS_dead_grave,	KS_dead_acute,
    KC(27),  KS_dead_diaeresis,	KS_dead_circumflex, KS_dead_tilde,
    KC(39),  KS_oslash,
    KC(40),  KS_ae,
    KC(41),  KS_bar,		KS_paragraph,
    KC(86),  KS_less,		KS_greater,
};

static const keysym_t atarikbd_keydesc_no_nodead[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(13),  KS_backslash,	KS_grave,	KS_acute,
    KC(27),  KS_diaeresis,	KS_asciicircum,	KS_asciitilde,
};

static const keysym_t atarikbd_keydesc_fr[] = {
/*  pos	     normal		shifted		altgr		shift-altgr */
    KC(2),   KS_ampersand,	KS_1,
    KC(3),   KS_eacute,		KS_2,		KS_asciitilde,
    KC(4),   KS_quotedbl,	KS_3,		KS_numbersign,
    KC(5),   KS_apostrophe,	KS_4,		KS_braceleft,
    KC(6),   KS_parenleft,	KS_5,		KS_bracketleft,
    KC(7),   KS_minus,		KS_6,		KS_bar,
    KC(8),   KS_egrave,		KS_7,		KS_grave,
    KC(9),   KS_underscore,	KS_8,		KS_backslash,
    KC(10),  KS_ccedilla,	KS_9,		KS_asciicircum,
    KC(11),  KS_agrave,		KS_0,		KS_at,
    KC(12),  KS_parenright,	KS_degree,	KS_bracketright,
    KC(13),  KS_equal,		KS_plus,	KS_braceright,
    KC(16),  KS_a,
    KC(17),  KS_z,
    KC(26),  KS_dead_circumflex, KS_dead_diaeresis, 
    KC(27),  KS_dollar,		KS_sterling,	KS_currency,
    KC(30),  KS_q,
    KC(39),  KS_m,
    KC(40),  KS_ugrave,		KS_percent,
    KC(41),  KS_twosuperior,	KS_asciitilde,
    KC(43),  KS_asterisk,	KS_mu,
    KC(44),  KS_w,
    KC(50),  KS_comma,		KS_question,
    KC(51),  KS_semicolon,	KS_period,
    KC(52),  KS_colon,		KS_slash,
    KC(53),  KS_exclam,		KS_section,
    KC(86),  KS_less,		KS_greater,
    KC(184), KS_Mode_switch,	KS_Multi_key,
};

static const keysym_t atarikbd_keydesc_it[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(3),   KS_2,	    	KS_quotedbl,	KS_twosuperior,
    KC(4),   KS_3,	    	KS_sterling,	KS_threesuperior,
    KC(5),   KS_4,	    	KS_dollar,
    KC(6),   KS_5,	    	KS_percent,
    KC(7),   KS_6,	    	KS_ampersand,
    KC(8),   KS_7,	    	KS_slash,
    KC(9),   KS_8,	    	KS_parenleft,
    KC(10),  KS_9,	    	KS_parenright,
    KC(11),  KS_0,	    	KS_equal,
    KC(12),  KS_apostrophe,	KS_question,
    KC(13),  KS_igrave,	    	KS_asciicircum,
    KC(26),  KS_egrave,		KS_eacute,	KS_braceleft,	KS_bracketleft,
    KC(27),  KS_plus,		KS_asterisk,	KS_braceright,	KS_bracketright,
    KC(39),  KS_ograve,		KS_Ccedilla,	KS_at,
    KC(40),  KS_agrave,		KS_degree,	KS_numbersign,
    KC(41),  KS_backslash,	KS_bar,
    KC(43),  KS_ugrave,		KS_section,
    KC(51),  KS_comma,		KS_semicolon,
    KC(52),  KS_period,		KS_colon,
    KC(53),  KS_minus,		KS_underscore,
    KC(86),  KS_less,		KS_greater,
    KC(184), KS_Mode_switch,	KS_Multi_key,
};

static const keysym_t atarikbd_keydesc_uk[] = {
/*  pos      normal             shifted         altgr           shift-altgr */
    KC(2),   KS_1,              KS_exclam,      KS_plusminus,   KS_exclamdown,
    KC(3),   KS_2,              KS_quotedbl,    KS_twosuperior, KS_cent,
    KC(4),   KS_3,              KS_sterling,    KS_threesuperior,
    KC(5),   KS_4,              KS_dollar,      KS_acute,       KS_currency,
    KC(6),   KS_5,              KS_percent,     KS_mu,          KS_yen,
    KC(7),   KS_6,              KS_asciicircum, KS_paragraph,
    KC(8),   KS_7,              KS_ampersand,   KS_periodcentered, KS_brokenbar,
    KC(9),   KS_8,              KS_asterisk,    KS_cedilla,     KS_ordfeminine,
    KC(10),  KS_9,              KS_parenleft,   KS_onesuperior, KS_diaeresis,
    KC(11),  KS_0,              KS_parenright,  KS_masculine,   KS_copyright,
    KC(12),  KS_minus,          KS_underscore,  KS_hyphen,      KS_ssharp,
    KC(13),  KS_equal,          KS_plus,        KS_onehalf,    KS_guillemotleft,
    KC(40),  KS_apostrophe,     KS_at,          KS_section,     KS_Agrave,
    KC(41),  KS_grave,          KS_grave,       KS_agrave,      KS_agrave,
    KC(43),  KS_numbersign,     KS_asciitilde,  KS_sterling,    KS_thorn,
    KC(86),  KS_backslash,      KS_bar,         KS_Udiaeresis,
};

static const keysym_t atarikbd_keydesc_jp[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(3),   KS_2,              KS_quotedbl,
    KC(7),   KS_6,              KS_ampersand,
    KC(8),   KS_7,              KS_apostrophe,
    KC(9),   KS_8,              KS_parenleft,
    KC(10),  KS_9,              KS_parenright,
    KC(11),  KS_0,
    KC(12),  KS_minus,          KS_equal,
    KC(13),  KS_asciicircum,    KS_asciitilde,
    KC(26),  KS_at,             KS_grave,
    KC(27),  KS_bracketleft,    KS_braceleft,
    KC(39),  KS_semicolon,      KS_plus,
    KC(40),  KS_colon,          KS_asterisk,
    KC(41),  KS_Zenkaku_Hankaku, /* replace grave/tilde */
    KC(43),  KS_bracketright,   KS_braceright,
    KC(112), KS_Hiragana_Katakana,
    KC(115), KS_backslash,      KS_underscore,
    KC(121), KS_Henkan,
    KC(123), KS_Muhenkan,
    KC(125), KS_backslash,      KS_bar,
};

static const keysym_t atarikbd_keydesc_es[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(2),   KS_1,		KS_exclam,	KS_bar,
    KC(3),   KS_2,		KS_quotedbl,	KS_at,
    KC(4),   KS_3,		KS_periodcentered, KS_numbersign,
    KC(5),   KS_4,		KS_dollar,	KS_asciitilde,
    KC(7),   KS_6,		KS_ampersand,
    KC(8),   KS_7,		KS_slash,
    KC(9),   KS_8,		KS_parenleft,
    KC(10),  KS_9,		KS_parenright,
    KC(11),  KS_0,		KS_equal,
    KC(12),  KS_apostrophe,	KS_question,
    KC(13),  KS_exclamdown,	KS_questiondown,
    KC(26),  KS_dead_grave,	KS_dead_circumflex, KS_bracketleft,
    KC(27),  KS_plus,		KS_asterisk,	KS_bracketright,
    KC(39),  KS_ntilde,
    KC(40),  KS_dead_acute,	KS_dead_diaeresis, KS_braceleft,
    KC(41),  KS_degree,		KS_ordfeminine,	KS_backslash,
    KC(43),  KS_ccedilla,	KS_Ccedilla,	KS_braceright,
    KC(51),  KS_comma,		KS_semicolon,
    KC(52),  KS_period,		KS_colon,
    KC(53),  KS_minus,		KS_underscore,
    KC(86),  KS_less,		KS_greater,
    KC(184), KS_Mode_switch,	KS_Multi_key,
};

static const keysym_t atarikbd_keydesc_us_declk[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(1),	KS_grave,	KS_asciitilde, /* replace escape */
    KC(41),	KS_less,	KS_greater, /* replace grave/tilde */
    KC(143),	KS_Multi_key, /* left compose */
    KC(157),	KS_Multi_key, /* right compose, replace right control */
    KC(87),	KS_Cmd_Debugger,	KS_Escape, /* replace F11 */
    KC(189),	KS_f13,
    KC(190),	KS_f14,
    KC(191),	KS_Help,
    KC(192),	KS_Execute,
    KC(193),	KS_f17,
    KC(183),	KS_f18,
    KC(70),	KS_f19, /* replace scroll lock */
    KC(127),	KS_f20, /* replace break */
    KC(69),	KS_KP_F1, /* replace num lock */
    KC(181),	KS_KP_F2, /* replace divide */
    KC(55),	KS_KP_F3, /* replace multiply */
    KC(74),	KS_KP_F4, /* replace subtract */

    /* keypad is numbers only - no num lock */
    KC(71), 	KS_KP_7,
    KC(72), 	KS_KP_8,
    KC(73), 	KS_KP_9,
    KC(75), 	KS_KP_4,
    KC(76), 	KS_KP_5,
    KC(77), 	KS_KP_6,
    KC(79), 	KS_KP_1,
    KC(80), 	KS_KP_2,
    KC(81), 	KS_KP_3,
    KC(82), 	KS_KP_0,
    KC(83), 	KS_KP_Decimal,

    KC(206),	KS_KP_Subtract,
    KC(78),	KS_KP_Separator, /* replace add */
    KC(199),	KS_Find, /* replace home */
    KC(207),	KS_Select, /* replace end */
};

static const keysym_t atarikbd_keydesc_us_dvorak[] = {
/*  pos      command		normal		shifted */
    KC(12), 			KS_bracketleft,	KS_braceleft,
    KC(13), 			KS_bracketright, KS_braceright,
    KC(16), 			KS_apostrophe, KS_quotedbl,
    KC(17), 			KS_comma, KS_less,
    KC(18), 			KS_period, KS_greater,
    KC(19), 			KS_p,
    KC(20), 			KS_y,
    KC(21), 			KS_f,
    KC(22), 			KS_g,
    KC(23), 			KS_c,
    KC(24), 			KS_r,
    KC(25), 			KS_l,
    KC(26), 			KS_slash, KS_question,
    KC(27), 			KS_equal, KS_plus,
    KC(31), 			KS_o,
    KC(32), 			KS_e,
    KC(33), 			KS_u,
    KC(34), 			KS_i,
    KC(35), 			KS_d,
    KC(36), 			KS_h,
    KC(37), 			KS_t,
    KC(38), 			KS_n,
    KC(39), 			KS_s,
    KC(40), 			KS_minus, KS_underscore,
    KC(44), 			KS_semicolon, KS_colon,
    KC(45), 			KS_q,
    KC(46), 			KS_j,
    KC(47), 			KS_k,
    KC(48), 			KS_x,
    KC(49), 			KS_b,
    KC(51), 			KS_w,
    KC(52), 			KS_v,
    KC(53), 			KS_z,
};

static const keysym_t atarikbd_keydesc_swapctrlcaps[] = {
/*  pos      command		normal		shifted */
    KC(29), 			KS_Caps_Lock,
    KC(58),  KS_Cmd1,		KS_Control_L,
};

static const keysym_t atarikbd_keydesc_iopener[] = {
/*  pos      command		normal		shifted */
    KC(59),  KS_Cmd_Debugger,	KS_Escape,
    KC(60),  KS_Cmd_Screen0,	KS_f1,
    KC(61),  KS_Cmd_Screen1,	KS_f2,
    KC(62),  KS_Cmd_Screen2,	KS_f3,
    KC(63),  KS_Cmd_Screen3,	KS_f4,
    KC(64),  KS_Cmd_Screen4,	KS_f5,
    KC(65),  KS_Cmd_Screen5,	KS_f6,
    KC(66),  KS_Cmd_Screen6,	KS_f7,
    KC(67),  KS_Cmd_Screen7,	KS_f8,
    KC(68),  KS_Cmd_Screen8,	KS_f9,
    KC(87),  KS_Cmd_Screen9,	KS_f10,
    KC(88), 			KS_f11,
};

#define KBD_MAP(name, base, map) \
			{ name, base, sizeof(map)/sizeof(keysym_t), map }

const struct wscons_keydesc atarikbd_keydesctab[] = {
	KBD_MAP(KB_US,			0,	atarikbd_keydesc_us),
	KBD_MAP(KB_DE,			KB_US,	atarikbd_keydesc_de),
	KBD_MAP(KB_DE | KB_NODEAD,	KB_DE,	atarikbd_keydesc_de_nodead),
	KBD_MAP(KB_FR,                  KB_US,  atarikbd_keydesc_fr),
	KBD_MAP(KB_DK,			KB_US,	atarikbd_keydesc_dk),
	KBD_MAP(KB_DK | KB_NODEAD,	KB_DK,	atarikbd_keydesc_dk_nodead),
	KBD_MAP(KB_IT,			KB_US,	atarikbd_keydesc_it),
	KBD_MAP(KB_UK,			KB_US,	atarikbd_keydesc_uk),
	KBD_MAP(KB_JP,			KB_US,	atarikbd_keydesc_jp),
	KBD_MAP(KB_SV,			KB_DK,	atarikbd_keydesc_sv),
	KBD_MAP(KB_SV | KB_NODEAD,	KB_SV,	atarikbd_keydesc_sv_nodead),
	KBD_MAP(KB_NO,			KB_DK,	atarikbd_keydesc_no),
	KBD_MAP(KB_NO | KB_NODEAD,	KB_NO,	atarikbd_keydesc_no_nodead),
	KBD_MAP(KB_US | KB_DECLK,	KB_US,	atarikbd_keydesc_us_declk),
	KBD_MAP(KB_US | KB_DVORAK,	KB_US,	atarikbd_keydesc_us_dvorak),
	KBD_MAP(KB_US | KB_SWAPCTRLCAPS, KB_US,	atarikbd_keydesc_swapctrlcaps),
	KBD_MAP(KB_US | KB_IOPENER, KB_US,	atarikbd_keydesc_iopener),
	KBD_MAP(KB_JP | KB_SWAPCTRLCAPS, KB_JP, atarikbd_keydesc_swapctrlcaps),
	KBD_MAP(KB_FR | KB_SWAPCTRLCAPS, KB_FR, atarikbd_keydesc_swapctrlcaps),
	KBD_MAP(KB_US | KB_DVORAK | KB_SWAPCTRLCAPS,	KB_US | KB_DVORAK,
		atarikbd_keydesc_swapctrlcaps),
	KBD_MAP(KB_US | KB_IOPENER | KB_SWAPCTRLCAPS,	KB_US | KB_IOPENER,
		atarikbd_keydesc_swapctrlcaps),
	KBD_MAP(KB_ES ,			KB_US,	atarikbd_keydesc_es),
	{0, 0, 0, 0}
};

#undef KBD_MAP
#undef KC
