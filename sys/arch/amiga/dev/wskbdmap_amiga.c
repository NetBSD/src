/*	$NetBSD: wskbdmap_amiga.c,v 1.5 2002/05/30 20:02:24 thorpej Exp $ */

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

/*
 * Amiga keyboard map for wscons.
 *
 * Modified from wskbdmap_mfii.c
 * The parts which have not been converted to use Amiga keycodes have
 * been commented out.
 *
 * US/DE keymaps are the ones that are supposed to work at least
 * partially.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wskbdmap_amiga.c,v 1.5 2002/05/30 20:02:24 thorpej Exp $");

#include <sys/types.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/pckbc/wskbdmap_mfii.h>

#define KC(n) KS_KEYCODE(n)

static const keysym_t amikbd_keydesc_us[] = {
/*  pos      command		normal		shifted */
    KC(69),   KS_Cmd_Debugger,	KS_Escape,
    KC(1),                      KS_1,		KS_exclam,
    KC(2),                      KS_2,		KS_at,
    KC(3),                      KS_3,		KS_numbersign,
    KC(4),                      KS_4,		KS_dollar,
    KC(5),                      KS_5,		KS_percent,
    KC(6),                  	KS_6,		KS_asciicircum,
    KC(7),                 	KS_7,		KS_ampersand,
    KC(8),               	KS_8,		KS_asterisk,
    KC(9),                 	KS_9,		KS_parenleft,
    KC(10),                	KS_0,		KS_parenright,
    KC(11), 			KS_minus,	KS_underscore,
    KC(12), 			KS_equal,	KS_plus,
    KC(65),  KS_Cmd_ResetEmul,	KS_Delete,
    KC(66), 			KS_Tab,
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
    KC(68), 			KS_Return,
    KC(99),  KS_Cmd1,		KS_Control_L,
    KC(32), 			KS_a,
    KC(33), 			KS_s,
    KC(34), 			KS_d,
    KC(35), 			KS_f,
    KC(36), 			KS_g,
    KC(37), 			KS_h,
    KC(38), 			KS_j,
    KC(39), 			KS_k,
    KC(40), 			KS_l,
    KC(41), 			KS_semicolon,	KS_colon,
    KC(42), 			KS_apostrophe,	KS_quotedbl,
    KC(0), 			KS_grave,	KS_asciitilde,
    KC(96), 			KS_Shift_L,
    KC(13), 			KS_backslash,	KS_bar,
    KC(48),                     KS_less,        KS_greater,
    KC(49), 			KS_z,
    KC(50), 			KS_x,
    KC(51), 			KS_c,
    KC(52), 			KS_v,
    KC(53), 			KS_b,
    KC(54), 			KS_n,
    KC(55), 			KS_m,
    KC(56), 			KS_comma,	KS_less,
    KC(57), 			KS_period,	KS_greater,
    KC(58), 			KS_slash,	KS_question,
    KC(97), 			KS_Shift_R,
    KC(100),  KS_Cmd2,		KS_Alt_L,
    KC(64), 			KS_space,
/*    KC(58), 			KS_Caps_Lock,*/
    KC(80),  KS_Cmd_Screen0,	KS_f1,
    KC(81),  KS_Cmd_Screen1,	KS_f2,
    KC(82),  KS_Cmd_Screen2,	KS_f3,
    KC(83),  KS_Cmd_Screen3,	KS_f4,
    KC(84),  KS_Cmd_Screen4,	KS_f5,
    KC(85),  KS_Cmd_Screen5,	KS_f6,
    KC(86),  KS_Cmd_Screen6,	KS_f7,
    KC(87),  KS_Cmd_Screen7,	KS_f8,
    KC(88),  KS_Cmd_Screen8,	KS_f9,
    KC(89),  KS_Cmd_Screen9,	KS_f10,

/*    KC(70), 			KS_Hold_Screen,*/

    /* numeric keypad */

    KC(61), 			KS_KP_7,        KS_KP_Home,
    KC(62), 			KS_KP_8,        KS_KP_Up,
    KC(63), 			KS_KP_9,        KS_KP_Prior,
    KC(74), 			KS_KP_Subtract,
    KC(45), 			KS_KP_4,        KS_KP_Left,
    KC(46), 			KS_KP_5,        KS_KP_Begin,
    KC(47), 			KS_KP_6,        KS_KP_Right,
    KC(94), 			KS_KP_Add,
    KC(29), 			KS_KP_1,        KS_KP_End,
    KC(30), 			KS_KP_2,        KS_KP_Down,
    KC(31), 			KS_KP_3,        KS_KP_Next,
    KC(15), 			KS_KP_0,        KS_KP_Insert,
    KC(60), 			KS_KP_Decimal,  KS_KP_Delete,
    KC(67), 			KS_KP_Enter,
    KC(90), 			KS_bracketleft,	KS_braceleft,
    KC(91), 			KS_bracketright,KS_braceright,
    KC(92), 			KS_KP_Divide,
    KC(93), 			KS_KP_Multiply,

/*    KC(87), 			KS_f11,*/
/*    KC(88), 			KS_f12,*/

/*    KC(127),			KS_Pause,*/ /* Break */
/*    KC(157),			KS_Control_R,*/
/*    KC(170),			KS_Print_Screen,*/
/*   KC(183),			KS_Print_Screen,*/
    KC(101),			KS_Alt_R,	KS_Multi_key,
/*    KC(199),			KS_Home,*/
    KC(76),			KS_Up,
/*    KC(90),			KS_Prior,*/
    KC(79),			KS_Left,
    KC(78),			KS_Right,
/*    KC(207),			KS_End,*/
    KC(77),			KS_Down,
/*    KC(91),			KS_Next,*/
/*    KC(210),			KS_Insert,*/
/*    KC(211),			KS_KP_Delete,*/
    KC(102),	KS_Cmd,		KS_Meta_L,
    KC(103),	KS_Cmd,		KS_Meta_R,
/*    KC(221),			KS_Menu,*/
};

static const keysym_t amikbd_keydesc_de[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(2),   KS_2,		KS_quotedbl,	KS_twosuperior,
    KC(3),   KS_3,		KS_section,	KS_threesuperior,
    KC(6),   KS_6,		KS_ampersand,
    KC(7),   KS_7,		KS_slash,	KS_braceleft,
    KC(8),   KS_8,		KS_parenleft,	KS_bracketleft,
    KC(9),   KS_9,		KS_parenright,	KS_bracketright,
    KC(10),  KS_0,		KS_equal,	KS_braceright,
    KC(11),  KS_ssharp,		KS_question,	KS_backslash,
    KC(12),  KS_dead_acute,	KS_dead_grave,
    KC(16),  KS_q,		KS_Q,		KS_at,
    KC(21),  KS_z,
    KC(26),  KS_udiaeresis,
    KC(27),  KS_plus,		KS_asterisk,	KS_dead_tilde,
    KC(41),  KS_odiaeresis,
    KC(42),  KS_adiaeresis,
    KC(13),  KS_dead_circumflex,KS_dead_abovering,
    KC(43),  KS_numbersign,	KS_apostrophe,
    KC(49),  KS_y,
    KC(55),  KS_m,		KS_M,		KS_mu,
    KC(56),  KS_comma,		KS_semicolon,
    KC(57),  KS_period,		KS_colon,
    KC(58),  KS_minus,		KS_underscore,
    KC(48),  KS_less,		KS_greater,	KS_bar,		KS_brokenbar,
    KC(101), KS_Mode_switch,	KS_Multi_key,
};

static const keysym_t amikbd_keydesc_de_nodead[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(12),  KS_apostrophe,	KS_grave,
    KC(27),  KS_plus,		KS_asterisk,	KS_asciitilde,
    KC(0),   KS_asciicircum,	KS_degree,
};

static const keysym_t pckbd_keydesc_fr[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(1),   KS_ampersand,	KS_1,
    KC(2),   KS_eacute,		KS_2,		KS_2,		KS_at,
    KC(3),   KS_quotedbl,	KS_3,		KS_3,		KS_numbersign,
    KC(4),   KS_apostrophe,	KS_4,
    KC(5),   KS_parenleft,	KS_5,
    KC(6),   KS_section,	KS_6,
    KC(7),   KS_egrave,		KS_7,
    KC(8),   KS_exclam,		KS_8,
    KC(9),   KS_ccedilla,	KS_9,
    KC(10),  KS_agrave,		KS_0,
    KC(11),  KS_parenright,	KS_degree,
    KC(12),  KS_minus,		KS_underscore,
    KC(16),  KS_a,
    KC(17),  KS_z,
    KC(26),  KS_dead_circumflex, KS_dead_diaeresis,
    KC(27),  KS_dollar,		KS_asterisk,
    KC(32),  KS_q,
    KC(41),  KS_m,
    KC(42),  KS_ugrave,		KS_percent,
    KC(43),  KS_mu,		KS_sterling,
    KC(49),  KS_w,
    KC(55),  KS_comma,		KS_question,
    KC(56),  KS_semicolon,	KS_period,
    KC(57),  KS_colon,		KS_slash,
    KC(58),  KS_equal,		KS_plus,
};

static const keysym_t amikbd_keydesc_dk[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(2),   KS_2,		KS_quotedbl,	KS_at,
    KC(3),   KS_3,		KS_numbersign,	KS_sterling,
    KC(4),   KS_4,		KS_currency,	KS_dollar,
    KC(6),   KS_6,		KS_ampersand,
    KC(7),   KS_7,		KS_slash,	KS_braceleft,
    KC(8),   KS_8,		KS_parenleft,	KS_bracketleft,
    KC(9),   KS_9,		KS_parenright,	KS_bracketright,
    KC(10),  KS_0,		KS_equal,	KS_braceright,
    KC(11),  KS_plus,		KS_question,
    KC(12),  KS_dead_acute,	KS_dead_grave,	KS_bar,
    KC(26),  KS_aring,
    KC(27),  KS_dead_diaeresis,	KS_dead_circumflex, KS_dead_tilde,
    KC(41),  KS_ae,
    KC(42),  KS_oslash,
    KC(43),  KS_apostrophe,	KS_asterisk,
    KC(56),  KS_comma,		KS_semicolon,
    KC(57),  KS_period,		KS_colon,
    KC(58),  KS_minus,		KS_underscore,
    KC(48),  KS_less,		KS_greater,	KS_backslash,
    KC(101), KS_Mode_switch,	KS_Multi_key,
};

static const keysym_t amikbd_keydesc_dk_nodead[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(12),  KS_apostrophe,	KS_grave,	KS_bar,
    KC(27),  KS_diaeresis,	KS_asciicircum,	KS_asciitilde,
};

static const keysym_t amikbd_keydesc_sv[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(11),  KS_plus,		KS_question,	KS_backslash,
    KC(27),  KS_dead_diaeresis,	KS_dead_circumflex, KS_dead_tilde,
    KC(41),  KS_odiaeresis,
    KC(42),  KS_adiaeresis,
    KC(48),  KS_less,		KS_greater,	KS_bar,
    KC(101), KS_Mode_switch,	KS_Multi_key,
};

static const keysym_t amikbd_keydesc_sv_nodead[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(12),  KS_apostrophe,	KS_grave,	KS_bar,
    KC(27),  KS_diaeresis,	KS_asciicircum,	KS_asciitilde,
};

static const keysym_t amikbd_keydesc_no[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(12),  KS_backslash,	KS_dead_grave,	KS_dead_acute,
    KC(27),  KS_dead_diaeresis,	KS_dead_circumflex, KS_dead_tilde,
    KC(41),  KS_oslash,
    KC(42),  KS_ae,
    KC(48),  KS_less,		KS_greater,
};

static const keysym_t amikbd_keydesc_no_nodead[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(12),  KS_backslash,	KS_grave,	KS_acute,
    KC(27),  KS_diaeresis,	KS_asciicircum,	KS_asciitilde,
};

static const keysym_t pckbd_keydesc_it[] __attribute__((__unused__)) = {
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

static const keysym_t pckbd_keydesc_uk[] __attribute__((__unused__)) = {
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

static const keysym_t pckbd_keydesc_jp[] __attribute__((__unused__)) = {
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

static const keysym_t pckbd_keydesc_es[] __attribute__((__unused__)) = {
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
    KC(12),  KS_grave,		KS_question,
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

static const keysym_t pckbd_keydesc_us_declk[] __attribute__((__unused__)) = {
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

static const keysym_t pckbd_keydesc_us_dvorak[] __attribute__((__unused__)) = {
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

static const keysym_t pckbd_keydesc_swapctrlcaps[]
    __attribute__((__unused__)) = {
/*  pos      command		normal		shifted */
    KC(29), 			KS_Caps_Lock,
    KC(58),  KS_Cmd1,		KS_Control_L,
};

#define KBD_MAP(name, base, map) \
			{ name, base, sizeof(map)/sizeof(keysym_t), map }

const struct wscons_keydesc amigakbd_keydesctab[] = {
	KBD_MAP(KB_US,			0,	amikbd_keydesc_us),
	KBD_MAP(KB_DE,			KB_US,	amikbd_keydesc_de),
	KBD_MAP(KB_DE | KB_NODEAD,	KB_DE,	amikbd_keydesc_de_nodead),
	KBD_MAP(KB_FR,                  KB_US,  pckbd_keydesc_fr),
	KBD_MAP(KB_DK,			KB_US,	amikbd_keydesc_dk),
	KBD_MAP(KB_DK | KB_NODEAD,	KB_DK,	amikbd_keydesc_dk_nodead),
/*	KBD_MAP(KB_IT,			KB_US,	pckbd_keydesc_it),
	KBD_MAP(KB_UK,			KB_US,	pckbd_keydesc_uk),
	KBD_MAP(KB_JP,			KB_US,	pckbd_keydesc_jp),*/
	KBD_MAP(KB_SV,			KB_DK,	amikbd_keydesc_sv),
	KBD_MAP(KB_SV | KB_NODEAD,	KB_SV,	amikbd_keydesc_sv_nodead),
	KBD_MAP(KB_NO,			KB_DK,	amikbd_keydesc_no),
	KBD_MAP(KB_NO | KB_NODEAD,	KB_NO,	amikbd_keydesc_no_nodead),
/*	KBD_MAP(KB_US | KB_DECLK,	KB_US,	pckbd_keydesc_us_declk),
	KBD_MAP(KB_US | KB_DVORAK,	KB_US,	pckbd_keydesc_us_dvorak),
	KBD_MAP(KB_US | KB_SWAPCTRLCAPS, KB_US,	pckbd_keydesc_swapctrlcaps),
	KBD_MAP(KB_JP | KB_SWAPCTRLCAPS, KB_JP, pckbd_keydesc_swapctrlcaps),
	KBD_MAP(KB_US | KB_DVORAK | KB_SWAPCTRLCAPS,	KB_US | KB_DVORAK,
		pckbd_keydesc_swapctrlcaps),
	KBD_MAP(KB_ES ,			KB_US,	pckbd_keydesc_es),
*/
	{0, 0, 0, 0}
};

#undef KBD_MAP
#undef KC
