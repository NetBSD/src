/*	$NetBSD: cckbdjpkeymap.h,v 1.1 1999/11/21 06:48:02 uch Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/* XXX tentative driver XXX */

static const struct keydesc {
	u_int16_t normal;
	u_int16_t shift;
	u_int16_t alt;
}  __cckbd_keydesc_jp[16][9] = {
	[15] = {
		{0, 0, 0},
		{KS_k,	KS_K, 0},
		{KS_j,	KS_J, 0},
		{KS_h,	KS_H, 0},
		{KS_g,	KS_G, 0},
		{KS_f,	KS_F, 0},
		{KS_d,	KS_D, 0},
		{KS_s,	KS_S, 0},
		{KS_a,	KS_A, 0},
	},
	[14] = {
		{0, 0, 0},
		{KS_i,	KS_I, 0},
		{KS_u,	KS_U, 0},
		{KS_y,	KS_Y, 0},
		{KS_t,	KS_T, 0},
		{KS_r,	KS_R, 0},
		{KS_e,	KS_E, 0},
		{KS_w,	KS_W, 0},
		{KS_q,	KS_Q, 0},
	},
	[13] = {
		{0, 0, 0},
		{KS_8,	KS_parenleft, 0},
		{KS_7,	KS_apostrophe, 0},
		{KS_6,	KS_ampersand, 0},
		{KS_5,	KS_percent, 0},
		{KS_4,	KS_dollar, 0},
		{KS_3,	KS_numbersign, 0},
		{KS_2,	KS_quotedbl, 0},
		{KS_1,	KS_exclam, 0},
	}, 
	[12] = {
		{0, 0, 0},
		{KS_BackSpace,	KS_Delete, 0},
		{KS_bracketleft,KS_braceleft, 0},
		{KS_minus,	KS_equal, 0},
		{KS_0,		KS_asciitilde, 0},
		{KS_9,		KS_parenright, 0},
		{KS_Tab,	0, 0},
		{KS_Escape,	0, 0},
		{KS_yen,	KS_bar, 0},
	},

	[11] = {
		{KS_Meta_L, 0, 0}, /* XXX Windows Key */
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
	},

	[10] = {
		{KS_Cmd, 0, 0}, /* XXX Fn key */
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
	},

	[9] = {
		{KS_Control_L, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
	},

	[8] = {
		{KS_Shift_L, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
	},

	[7] = {
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
	},

	[6] = {
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0}, /* Light */
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
	},

	[5] = {
		{KS_Shift_R, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
	},

	[4] = {
		{0, 0, 0},
		{KS_o, KS_O, 0},
		{KS_p, KS_P, 0},
		{KS_colon,		KS_asterisk,	0},
		{0, 0, 0},
		{KS_bracketright,	KS_braceright,	0},
		{KS_at,			KS_apostrophe,	0},
		{KS_backslash,		KS_underscore,	0},
		{KS_Caps_Lock,		0,		0},
	},

	[3] = {
		{0, 0, 0},
		{KS_Right, 0, 0},
		{KS_Left, 0, 0},
		{KS_Down, 0, 0},
		{KS_Up, 0, 0},
		{KS_semicolon,	KS_plus		, 0},
		{KS_slash, KS_question, 0},
		{KS_period, KS_less, 0},
		{KS_comma, KS_greater, 0},
	},

	[2] = {
		{0, 0, 0},
		{KS_asciicircum, KS_asciitilde, 0},
		{0, 0, 0},
		{KS_Hiragana_Katakana, 0, 0},
		{KS_Henkan, 0, 0},
		{KS_Muhenkan, 0, 0},
		{KS_Zenkaku_Hankaku, 0, 0},
		{KS_Return, 0, 0},
		{KS_space, 0, 0},
	},

	[1] = {
		{KS_Alt_L, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
		{0, 0, 0},
	},

	[0] = {
		{0, 0, 0},
		{KS_l, KS_L, 0},
		{KS_m, KS_M, 0},
		{KS_n, KS_N, 0},
		{KS_b, KS_B, 0},
		{KS_v, KS_V, 0},
		{KS_c, KS_C, 0},
		{KS_x, KS_X, 0},
		{KS_z, KS_z, 0},
	}
};
