/*	$NetBSD: wskbdmap_amiga.c,v 1.7 2004/01/04 19:20:43 jandberg Exp $ */

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
 * Amiga keyboard maps for wscons.
 */

/*
 * Keymaps: us, de, fr, es, sv converted from the old ite keymaps 
 * which were created by Markus Wild, Eric Delcamp, Inaki Saez 
 * and Stefan Pedersen.
 *
 * Copyright (c) 1993 Markus Wild
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Markus Wild.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wskbdmap_amiga.c,v 1.7 2004/01/04 19:20:43 jandberg Exp $");

#include <sys/types.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#define KC(n) KS_KEYCODE(n)

static const keysym_t amikbd_keydesc_us[] = {
/*  keycode  normal		shift		alt		shift-alt */
    KC(0),   KS_grave,		KS_asciitilde,
    KC(1),   KS_1,		KS_exclam,	KS_onesuperior,	KS_exclam,
    KC(2),   KS_2,		KS_at,		KS_twosuperior,	KS_at,
    KC(3),   KS_3,		KS_numbersign,	KS_threesuperior,KS_numbersign,
    KC(4),   KS_4,		KS_dollar,	KS_cent,	KS_dollar,
    KC(5),   KS_5,		KS_percent,	KS_onequarter,	KS_percent,
    KC(6),   KS_6,		KS_asciicircum,	KS_onehalf,	KS_asciicircum,
    KC(7),   KS_7,		KS_ampersand,	KS_threequarters,KS_ampersand,
    KC(8),   KS_8,		KS_asterisk,	KS_periodcentered,KS_asterisk,
    KC(9),   KS_9,		KS_parenleft,	KS_guillemotleft,KS_parenleft,
    KC(10),  KS_0,		KS_parenright,KS_guillemotright,KS_parenright,
    KC(11),  KS_minus,		KS_underscore,
    KC(12),  KS_equal,		KS_plus,
    KC(13),  KS_backslash,	KS_bar,
    KC(15),  KS_KP_0,
    KC(16),  KS_q,		KS_Q,		KS_aring,	KS_Aring,
    KC(17),  KS_w,		KS_W,		KS_degree,
    KC(18),  KS_e,		KS_E,		KS_copyright,
    KC(19),  KS_r,		KS_R,		KS_registered,
    KC(20),  KS_t,		KS_T,		KS_thorn,
    KC(21),  KS_y,		KS_Y,		KS_currency,	KS_yen,
    KC(22),  KS_u,		KS_U,		KS_mu,
    KC(23),  KS_i,		KS_I,		KS_exclamdown, 	KS_brokenbar,
    KC(24),  KS_o,		KS_O,		KS_oslash,	KS_Ooblique,
    KC(25),  KS_p,		KS_P,		KS_paragraph,
    KC(26),  KS_bracketleft,	KS_braceleft,
    KC(27),  KS_bracketright,	KS_braceright,
    KC(29),  KS_KP_1,		KS_KP_End,
    KC(30),  KS_KP_2,		KS_KP_Down,
    KC(31),  KS_KP_3,		KS_KP_Next,
    KC(32),  KS_a,		KS_A,		KS_ae,		KS_AE,
    KC(33),  KS_s,		KS_S,		KS_ssharp,	KS_section,
    KC(34),  KS_d,		KS_D,		KS_eth,		KS_ETH,
    KC(35),  KS_f,		KS_F,		KS_dead_acute,
    KC(36),  KS_g,		KS_G,		KS_dead_grave,
    KC(37),  KS_h,		KS_H,		KS_dead_circumflex,
    KC(38),  KS_j,		KS_J,		KS_dead_tilde,
    KC(39),  KS_k,		KS_K,		KS_dead_diaeresis,
    KC(40),  KS_l,		KS_L,		KS_sterling,
    KC(41),  KS_semicolon,	KS_colon,
    KC(42),  KS_apostrophe,	KS_quotedbl,
    KC(45),  KS_KP_4,		KS_KP_Left,
    KC(46),  KS_KP_5,		KS_KP_Begin,
    KC(47),  KS_KP_6,		KS_KP_Right,
    KC(49),  KS_z,		KS_Z,		KS_plusminus,
    KC(50),  KS_x,		KS_X,		KS_multiply,
    KC(51),  KS_c,		KS_C,		KS_ccedilla,
    KC(52),  KS_v,		KS_V,		KS_ordfeminine,
    KC(53),  KS_b,		KS_B,		KS_masculine,
    KC(54),  KS_n,		KS_N,		KS_hyphen,
    KC(55),  KS_m,		KS_M,		KS_cedilla,
    KC(56),  KS_comma,		KS_less,
    KC(57),  KS_period,		KS_greater,
    KC(58),  KS_slash,		KS_question,
    KC(60),  KS_KP_Decimal,	KS_KP_Delete,
    KC(61),  KS_KP_7,		KS_KP_Home,
    KC(62),  KS_KP_8,		KS_KP_Up,
    KC(63),  KS_KP_9,		KS_KP_Prior,
    KC(64),  KS_space,		KS_space,	KS_nobreakspace,KS_nobreakspace,

/*  keycode  command		normal */
    KC(65),  KS_Cmd_ResetEmul,	KS_Delete,
    KC(66), 			KS_Tab,
    KC(67), 			KS_KP_Enter,
    KC(68), 			KS_Return,
    KC(69),  KS_Cmd_Debugger,	KS_Escape,
    KC(70),			KS_BackSpace,
    KC(74), 			KS_KP_Subtract,
    KC(76),			KS_Up,
    KC(77),			KS_Down,
    KC(78),			KS_Right,
    KC(79),			KS_Left,
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
    KC(90), 			KS_parenleft,
    KC(91), 			KS_parenright,
    KC(92), 			KS_KP_Divide,
    KC(93), 			KS_KP_Multiply,
    KC(94), 			KS_KP_Add,
    KC(95), 			KS_Help,
    KC(96), 			KS_Shift_L,
    KC(97), 			KS_Shift_R,
    KC(99),  KS_Cmd1,		KS_Control_L,
    KC(100), KS_Cmd2,		KS_Mode_switch,
    KC(101),			KS_Mode_switch,
    KC(102), KS_Cmd,		KS_Alt_L,
    KC(103), 			KS_Multi_key,
};

static const keysym_t amikbd_keydesc_de[] = {
/*  keycode  normal		shift		alt		shift-alt */
    KC(2),   KS_2,		KS_quotedbl,	KS_at,		KS_twosuperior,
    KC(3),   KS_3,		KS_section,	KS_threesuperior,KS_numbersign,
    KC(4),   KS_4,		KS_dollar,	KS_degree,	KS_cent,
    KC(6),   KS_6,		KS_ampersand,	KS_onehalf,	KS_asciicircum,
    KC(7),   KS_7,		KS_slash,	KS_threequarters,KS_ampersand,
    KC(8),   KS_8,		KS_parenleft,	KS_periodcentered,KS_asterisk,
    KC(9),   KS_9,		KS_parenright,	KS_guillemotleft,KS_parenleft,
    KC(10),  KS_0,		KS_equal,	KS_guillemotright,KS_parenright,
    KC(11),  KS_ssharp,		KS_question,	KS_minus,	KS_underscore,
    KC(12),  KS_dead_acute,	KS_dead_grave,	KS_equal,	KS_plus,
    KC(21),  KS_z,		KS_Z,		KS_currency,	KS_yen,
    KC(26),  KS_udiaeresis,	KS_Udiaeresis,	KS_bracketleft,	KS_braceleft,
    KC(27),  KS_plus,		KS_asterisk,	KS_bracketright,KS_braceright,
    KC(41),  KS_odiaeresis,	KS_Odiaeresis,	KS_semicolon,	KS_colon,
    KC(42),  KS_adiaeresis,	KS_Adiaeresis,	KS_apostrophe,	KS_quotedbl,
    KC(43),  KS_numbersign,	KS_asciicircum, 
    KC(48),  KS_less,		KS_greater,
    KC(49),  KS_y,		KS_Y,		KS_plusminus,	KS_notsign,
    KC(50),  KS_x,		KS_X,		KS_multiply,	KS_division,
    KC(51),  KS_c,		KS_C,		KS_ccedilla,	KS_Ccedilla,
    KC(54),  KS_n,		KS_N,		KS_hyphen,	KS_macron,
    KC(55),  KS_m,		KS_M,		KS_cedilla,	KS_questiondown,
    KC(56),  KS_comma,		KS_semicolon,	KS_comma,	KS_less,
    KC(57),  KS_period,		KS_colon,	KS_period,	KS_greater,
    KC(58),  KS_minus,		KS_underscore,	KS_slash,	KS_question,
    KC(90),  KS_bracketleft,	KS_braceleft,
    KC(91),  KS_bracketright,	KS_braceright,
};

static const keysym_t amikbd_keydesc_de_nodead[] = {
/*  keycode  normal		shift		alt		shift-alt */
    KC(12),  KS_apostrophe,	KS_grave,	KS_equal,	KS_plus,
};

static const keysym_t amikbd_keydesc_pl[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(18),  KS_e,		KS_E,		KS_ecircumflex,	KS_Ecircumflex,
    KC(24),  KS_o,		KS_O,		KS_oacute,	KS_Oacute,
    KC(32),  KS_a,		KS_A,		KS_plusminus,	KS_exclamdown,
    KC(33),  KS_s,		KS_S,		KS_paragraph,	KS_brokenbar,
    KC(40),  KS_l,		KS_L,		KS_threesuperior, KS_sterling,
    KC(49),  KS_z,		KS_Z,		KS_questiondown, KS_macron,
    KC(50),  KS_x,		KS_X,		KS_onequarter,	KS_notsign,
    KC(51),  KS_c,		KS_C,		KS_ae,		KS_AE,
    KC(54),  KS_n,		KS_N,		KS_ntilde,	KS_Ntilde,
};

static const keysym_t amikbd_keydesc_fr[] = {
/*  keycode  normal		shift		alt		shift-alt */
    KC(1),   KS_ampersand,	KS_1,		KS_onesuperior,	KS_exclam,
    KC(2),   KS_eacute,		KS_2,		KS_twosuperior,	KS_at,
    KC(3),   KS_quotedbl,	KS_3,		KS_threesuperior,KS_numbersign,
    KC(4),   KS_apostrophe,	KS_4,		KS_cent,	KS_dollar,
    KC(5),   KS_parenleft,	KS_5,		KS_onequarter,	KS_percent,
    KC(6),   KS_section,	KS_6,		KS_onehalf,	KS_asciicircum,
    KC(7),   KS_egrave,		KS_7,		KS_threequarters,KS_ampersand,
    KC(8),   KS_exclam,		KS_8,		KS_periodcentered,KS_asterisk,
    KC(9),   KS_ccedilla,	KS_9,		KS_guillemotleft,KS_parenleft,
    KC(10),  KS_agrave,		KS_0,		KS_guillemotright,KS_parenright,
    KC(11),  KS_parenright,	KS_degree,	KS_minus,	KS_underscore,
    KC(12),  KS_minus,		KS_underscore,	KS_equal,	KS_plus,
    KC(16),  KS_a,		KS_A,		KS_aring,	KS_Aring,
    KC(17),  KS_z,		KS_Z,		KS_degree,	KS_degree,
    KC(26),  KS_dead_circumflex,KS_dead_diaeresis,KS_bracketleft,KS_braceleft,
    KC(27),  KS_dollar,		KS_asterisk,	KS_bracketright,KS_braceright,
    KC(32),  KS_q,		KS_Q,		KS_ae,		KS_AE,
    KC(41),  KS_m,		KS_M,		KS_semicolon,	KS_semicolon,
    KC(42),  KS_Ugrave,		KS_percent,	KS_apostrophe,	KS_quotedbl,
    KC(43),  KS_mu,		KS_sterling,	KS_mu,		KS_sterling,
    KC(48),  KS_less,		KS_greater,	KS_less,	KS_greater,
    KC(49),  KS_w,		KS_W,		KS_plusminus,	KS_notsign,
    KC(50),  KS_x,		KS_X,		KS_multiply,	KS_division,
    KC(54),  KS_n,		KS_N,		KS_hyphen,	KS_macron,
    KC(55),  KS_comma,		KS_question,	KS_cedilla,	KS_questiondown,
    KC(56),  KS_semicolon,	KS_period,	KS_comma,	KS_less,
    KC(57),  KS_colon,		KS_slash,	KS_period,	KS_greater,
    KC(58),  KS_equal,		KS_plus,	KS_slash,	KS_question,
    KC(90),  KS_bracketleft,	KS_braceleft,
    KC(91),  KS_bracketright,	KS_braceright,
};

static const keysym_t amikbd_keydesc_fr_nodead[] = {
/*  keycode  normal		shift		alt		shift-alt */
    KC(26),  KS_asciicircum,	KS_diaeresis,	KS_bracketleft,	KS_braceleft,
};

static const keysym_t amikbd_keydesc_dk[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(3),   KS_3,		KS_numbersign,	KS_threesuperior,KS_numbersign,
    KC(4),   KS_4,		KS_currency,	KS_cent,	KS_dollar,
    KC(41),  KS_ae,		KS_AE,		KS_semicolon,	KS_colon,
    KC(42),  KS_oslash,		KS_Ooblique,	KS_apostrophe,	KS_quotedbl,
};

static const keysym_t amikbd_keydesc_sv[] = {
/*  keycode  normal		shift		alt		shift-alt */
    KC(2),   KS_2,		KS_quotedbl,    KS_twosuperior,	KS_at,
    KC(3),   KS_3,		KS_sterling,	KS_threesuperior,KS_numbersign,
    KC(6),   KS_6,		KS_ampersand,   KS_onehalf,	KS_asciicircum,
    KC(7),   KS_7,		KS_slash,	KS_threequarters,KS_ampersand,
    KC(8),   KS_8,		KS_parenleft,	KS_periodcentered,KS_asterisk,
    KC(9),   KS_9,		KS_parenright,	KS_guillemotleft,KS_parenleft,
    KC(10),  KS_0,		KS_equal,	KS_guillemotright,KS_parenright,
    KC(11),  KS_plus,		KS_question,	KS_minus,	KS_underscore,
    KC(12),  KS_dead_acute,	KS_dead_grave,	KS_equal,	KS_plus,
    KC(26),  KS_acircumflex,	KS_Acircumflex, KS_bracketleft,	KS_braceleft,
    KC(27),  KS_dead_diaeresis,KS_dead_circumflex,KS_bracketright,KS_braceright,
    KC(41),  KS_odiaeresis,	KS_Odiaeresis,	KS_semicolon,	KS_colon,
    KC(42),  KS_adiaeresis,	KS_Adiaeresis,	KS_apostrophe,	KS_quotedbl,
    KC(43),  KS_apostrophe,	KS_asterisk,
    KC(48),  KS_less,		KS_greater, 
    KC(56),  KS_comma,		KS_semicolon,	KS_comma,	KS_less,
    KC(57),  KS_period,		KS_colon,	KS_period,	KS_greater,
    KC(58),  KS_minus,		KS_underscore,	KS_slash,	KS_question,
    KC(90),  KS_bracketleft,	KS_braceleft,
    KC(91),  KS_bracketright,	KS_braceright,
};

static const keysym_t amikbd_keydesc_sv_nodead[] = {
/*  keycode  normal		shift		alt		shift-alt */
    KC(12),  KS_apostrophe,	KS_grave,	KS_equal,	KS_plus,
    KC(27),  KS_diaeresis,	KS_asciicircum,	KS_bracketleft,	KS_braceleft,
};

static const keysym_t amikbd_keydesc_no[] = {
/*  pos      normal		shifted		altgr		shift-altgr */
    KC(41),  KS_oslash,		KS_Ooblique,	KS_semicolon,	KS_colon,
    KC(42),  KS_ae,		KS_AE,		KS_apostrophe,	KS_quotedbl,
};

static const keysym_t amikbd_keydesc_es[] = {
/*  keycode  normal		shift		alt		shift-alt */
    KC(1),   KS_1,		KS_exclamdown,	KS_onesuperior, KS_exclam,
    KC(2),   KS_2,		KS_questiondown,KS_twosuperior,	KS_at,
    KC(4),   KS_4,		KS_dollar,	KS_cent,	KS_degree,
    KC(5),   KS_5,		KS_percent,	KS_onequarter,	KS_percent,
    KC(6),   KS_6,		KS_slash,	KS_onehalf,	KS_asciicircum,
    KC(26),  KS_dead_acute,	KS_dead_diaeresis,KS_bracketleft,KS_braceleft,
    KC(27),  KS_dead_grave,KS_dead_circumflex,KS_bracketright,KS_braceright,
    KC(41),  KS_ntilde,		KS_Ntilde,	KS_semicolon,	KS_colon,
    KC(42),  KS_semicolon,	KS_colon,	KS_apostrophe,	KS_quotedbl,
    KC(43),  KS_ccedilla,	KS_Ccedilla,	KS_ordfeminine,	KS_masculine,
    KC(48),  KS_less,		KS_greater,KS_guillemotleft,KS_guillemotright,
    KC(49),  KS_z,		KS_Z,		KS_plusminus,	KS_notsign,
    KC(50),  KS_x,		KS_X,		KS_multiply,	KS_division,
    KC(54),  KS_n,		KS_N,		KS_hyphen,	KS_macron,
    KC(55),  KS_m,		KS_M,		KS_cedilla,	KS_questiondown,
    KC(56),  KS_comma,		KS_question,	KS_comma,	KS_less,
    KC(57),  KS_period,		KS_exclam,	KS_period,	KS_greater,
    KC(58),  KS_apostrophe,	KS_quotedbl,	KS_slash,	KS_question,
    KC(90),  KS_bracketleft,	KS_braceleft,
    KC(91),  KS_bracketright,	KS_braceright,
};

static const keysym_t amikbd_keydesc_es_nodead[] = {
/*  keycode  normal		shift		alt		shift-alt */
    KC(26),  KS_apostrophe,	KS_diaeresis,	KS_bracketleft,	KS_braceleft,
    KC(27),  KS_grave,		KS_asciicircum,	KS_bracketright,KS_braceright,
};

#define KBD_MAP(name, base, map) \
			{ name, base, sizeof(map)/sizeof(keysym_t), map }

const struct wscons_keydesc amigakbd_keydesctab[] = {
	KBD_MAP(KB_US,			0,	amikbd_keydesc_us),
	KBD_MAP(KB_DE,			KB_US,	amikbd_keydesc_de),
	KBD_MAP(KB_DE | KB_NODEAD,	KB_DE,	amikbd_keydesc_de_nodead),
	KBD_MAP(KB_FR,			KB_US,	amikbd_keydesc_fr),
	KBD_MAP(KB_FR | KB_NODEAD,	KB_FR,	amikbd_keydesc_fr_nodead),
	KBD_MAP(KB_DK,			KB_SV,	amikbd_keydesc_dk),
	KBD_MAP(KB_DK | KB_NODEAD,	KB_DK,	amikbd_keydesc_sv_nodead),
	KBD_MAP(KB_PL,			KB_US,	amikbd_keydesc_pl),
	KBD_MAP(KB_SV,			KB_US,	amikbd_keydesc_sv),
	KBD_MAP(KB_SV | KB_NODEAD,	KB_SV,	amikbd_keydesc_sv_nodead),
	KBD_MAP(KB_NO,			KB_SV,	amikbd_keydesc_no),
	KBD_MAP(KB_NO | KB_NODEAD,	KB_NO,	amikbd_keydesc_sv_nodead),
	KBD_MAP(KB_ES,			KB_US,	amikbd_keydesc_es),
	KBD_MAP(KB_ES | KB_NODEAD,	KB_ES,	amikbd_keydesc_es_nodead),
	{ 0, 0, 0, 0}
};

#undef KBD_MAP
#undef KC
