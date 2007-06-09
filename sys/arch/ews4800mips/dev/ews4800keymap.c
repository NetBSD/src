/*	$NetBSD: ews4800keymap.c,v 1.1.32.1 2007/06/09 23:55:02 ad Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ews4800keymap.c,v 1.1.32.1 2007/06/09 23:55:02 ad Exp $");

#include <sys/param.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <ews4800mips/dev/ews4800keymap.h>

#define	KC(n) KS_KEYCODE(n)

const keysym_t ews4800kbd_keydesc[] = {
	KC(0x00), KS_0,
	KC(0x01), KS_1, KS_exclam,
	KC(0x02), KS_2, KS_quotedbl,
	KC(0x03), KS_3, KS_numbersign,
	KC(0x04), KS_4, KS_dollar,
	KC(0x05), KS_5, KS_percent,
	KC(0x06), KS_6, KS_ampersand,
	KC(0x07), KS_7, KS_apostrophe,
	KC(0x08), KS_8, KS_parenleft,
	KC(0x09), KS_9, KS_parenright,
	KC(0x0a), KS_minus, KS_equal,
	KC(0x0b), KS_asciicircum, KS_grave,
	KC(0x0c), KS_backslash, KS_bar,
	KC(0x0d), KS_colon, KS_asterisk,
	KC(0x0e), KS_period, KS_greater,
	KC(0x0f), KS_slash, KS_question,
	KC(0x10), KS_at, KS_asciitilde,
	KC(0x11), KS_a,
	KC(0x12), KS_b,
	KC(0x13), KS_c,
	KC(0x14), KS_d,
	KC(0x15), KS_e,
	KC(0x16), KS_f,
	KC(0x17), KS_g,
	KC(0x18), KS_h,
	KC(0x19), KS_i,
	KC(0x1a), KS_j,
	KC(0x1b), KS_k,
	KC(0x1c), KS_l,
	KC(0x1d), KS_m,
	KC(0x1e), KS_n,
	KC(0x1f), KS_o,
	KC(0x20), KS_p,
	KC(0x21), KS_q,
	KC(0x22), KS_r,
	KC(0x23), KS_s,
	KC(0x24), KS_t,
	KC(0x25), KS_u,
	KC(0x26), KS_v,
	KC(0x27), KS_w,
	KC(0x28), KS_x,
	KC(0x29), KS_y,
	KC(0x2a), KS_z,
	KC(0x2b), KS_bracketleft, KS_braceleft,
	KC(0x2c), KS_comma, KS_less,
	KC(0x2d), KS_bracketright, KS_braceright,
	KC(0x2e), KS_semicolon, KS_plus,
	KC(0x2f), KS_underscore, KS_underscore,
	KC(0x30), KS_KP_0,
	KC(0x31), KS_KP_1,
	KC(0x32), KS_KP_2,
	KC(0x33), KS_KP_3,
	KC(0x34), KS_KP_4,
	KC(0x35), KS_KP_5,
	KC(0x36), KS_KP_6,
	KC(0x37), KS_KP_7,
	KC(0x38), KS_KP_8,
	KC(0x39), KS_KP_9,
	KC(0x3a), KS_space,
	KC(0x3b), KS_comma,
	KC(0x3e), KS_Hiragana_Katakana, /* KANA */
	KC(0x40), KS_Return,
	KC(0x41), KS_Return,
	KC(0x42), KS_Prior,
	KC(0x43), KS_Next,
	KC(0x44), KS_KP_Enter,
	KC(0x46), KS_minus,
	KC(0x47), KS_period,
	KC(0x48), KS_Left,
	KC(0x48), KS_Return,
	KC(0x49), KS_Right,
	KC(0x50), KS_BackSpace,
	KC(0x52), KS_Clear, KS_Home,
	KC(0x53), KS_Delete,
	KC(0x54), KS_Insert,
	KC(0x56), KS_Cmd_Screen0, KS_f1,
	KC(0x57), KS_Cmd_Screen1, KS_f2,
	KC(0x58), KS_Cmd_Screen2, KS_f3,
	KC(0x59), KS_Cmd_Screen3, KS_f4,
	KC(0x5a), KS_Cmd_Screen4, KS_f5,
	KC(0x5b), KS_Cmd_Screen5, KS_f6,
	KC(0x5c), KS_Cmd_Screen6, KS_f7,
	KC(0x5d), KS_Cmd_Screen7, KS_f8,
	KC(0x5e), KS_Cmd_Screen8, KS_f9,
	KC(0x5f), KS_Cmd_Screen9, KS_f10,
	KC(0x60), KS_f11, /* No key top print */
	KC(0x61), KS_f12, /* No key top print */
	KC(0x62), KS_f13, /* No key top print */
	KC(0x63), KS_f14, /* No key top print */
	KC(0x64), KS_f15, /* No key top print */
	KC(0x67), KS_Muhenkan, /* NFER */
	KC(0x68), KS_Henkan, /* XFER */
	KC(0x6a), KS_Up,
	KC(0x6b), KS_Down,
	KC(0x6d), KS_Meta_L, /* GRAPH */
	KC(0x70), KS_KP_Add,
	KC(0x71), KS_KP_Multiply,
	KC(0x72), KS_slash,
	KC(0x73), KS_Escape,
	KC(0x74), KS_Help,
	KC(0x75), KS_KP_Equal,
	KC(0x76), KS_Tab,
	KC(0x78), KS_Cmd1, KS_Control_L,
	KC(0x79), KS_Caps_Lock,
	KC(0x7b), KS_Shift_L,
	KC(0x7c), KS_Shift_R,
	KC(0x7e), KS_Cmd2, KS_Alt_L,
	KC(0x7f), KS_Cmd_Debugger, KS_Cmd1, /* FNC */
};

const struct wscons_keydesc ews4800kbd_keydesctab[] = {
	{ KB_JP, 0, sizeof(ews4800kbd_keydesc) / sizeof(keysym_t),
	    ews4800kbd_keydesc, },
	{ 0, 0, 0, NULL },
};
