/*	$NetBSD: skbdkeymap.h,v 1.1 1999/12/08 15:49:18 uch Exp $ */

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
#define UNK	-1	/* unknown */
#define IGN	-2	/* ignore */

const u_int8_t default_keymap[] = {
/*      0    1    2    3    4    5    6    7 */       
/* 0 */	UNK, UNK, UNK, UNK, UNK, UNK, UNK, UNK,
/* 1 */	UNK, UNK, UNK, UNK, UNK, UNK, UNK, UNK,
/* 2 */	UNK, UNK, UNK, UNK, UNK, UNK, UNK, UNK,
/* 3 */	UNK, UNK, UNK, UNK, UNK, UNK, UNK, UNK,
/* 4 */	UNK, UNK, UNK, UNK, UNK, UNK, UNK, UNK,
/* 5 */	UNK, UNK, UNK, UNK, UNK, UNK, UNK, UNK,
/* 6 */	UNK, UNK, UNK, UNK, UNK, UNK, UNK, UNK,
/* 7 */	UNK, UNK, UNK, UNK, UNK, UNK, UNK, UNK,
/* 8 */	UNK, UNK, UNK, UNK, UNK, UNK, UNK, UNK,
/* 9 */	UNK, UNK, UNK, UNK, UNK, UNK, UNK, UNK,
/*10 */	UNK, UNK, UNK, UNK, UNK, UNK, UNK, UNK,
/*11 */	UNK, UNK, UNK, UNK, UNK, UNK, UNK, UNK,
/*12 */	UNK, UNK, UNK, UNK, UNK, UNK, UNK, UNK,
/*13 */	UNK, UNK, UNK, UNK, UNK, UNK, UNK, UNK,
/*14 */	UNK, UNK, UNK, UNK, UNK, UNK, UNK, UNK,
/*15 */	UNK, UNK, UNK, UNK, UNK, UNK, UNK, UNK
};

const u_int8_t p7416_compaq_c_jp_keymap[] = {
/*      0    1    2    3    4    5    6    7 */       
/* 0 */	38,  50,  49,  48,  47,  46,  45,  44, 
/* 1 */	56,  IGN, IGN, IGN, IGN, IGN, IGN, IGN,
/* 2 */	13,  IGN, 112, 121, 123, 41,  28,  57, 
/* 3 */	77,  75,  80,  72,  39,  53,  52,  51,
/* 4 */	24,  25,  40,  IGN, 43,  26,  115, 58,
/* 5 */	54,  IGN, IGN, IGN, IGN, IGN, IGN, IGN,
/* 6 */	IGN, IGN, IGN, 70,  IGN, IGN, IGN, IGN,
/* 7 */	IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN,
/* 8 */	42,  IGN, IGN, IGN, IGN, IGN, IGN, IGN,
/* 9 */	29,  IGN, IGN, IGN, IGN, IGN, IGN, IGN,
/*10 */	221, IGN, IGN, IGN, IGN, IGN, IGN, IGN,
/*11 */	221, IGN, IGN, IGN, IGN, IGN, IGN, IGN,
/*12 */	14,  27,  12,  11,  10,  15,  1,   125,
/*13 */	9,   8,   7,   6,   5,   4,   3,   2,
/*14 */	23,  22,  21,  20,  19,  18,  17,  16,
/*15 */	37,  36,  35,  34,  33,  32,  31,  30
};

const u_int8_t m38813c_keymap[] = {
/*      0    1    2    3    4    5    6    7 */       
/* 0 */	0,   1,   2,   3,   4,   5,   6,   7,
/* 1 */	8,   9,   10,  11,  12,  13,  14,  15,
/* 2 */	16,  17,  18,  19,  20,  21,  22,  23,
/* 3 */	24,  25,  26,  27,  28,  29,  30,  31,
/* 4 */	32,  33,  34,  35,  36,  37,  38,  39,
/* 5 */	40,  41,  42,  43,  44,  45,  46,  47,
/* 6 */	48,  49,  50,  51,  52,  53,  54,  55,
/* 7 */	56,  57,  58,  59,  60,  61,  62,  63,
/* 8 */	64,  65,  66,  67,  68,  69,  70,  71,
/* 9 */	72,  73,  74,  75,  76,  77,  78,  79,
/*10 */	80,  81,  82,  83,  84,  85,  86,  87,
/*11 */	88,  89,  90,  91,  92,  93,  94,  95,
/*12 */	96,  97,  98,  99,  100, 101, 102, 103,
/*13 */	104, 105, 106, 107, 108, 109, 110, 111,
/*14 */	112, 113, 114, 115, 116, 117, 118, 119,
/*15 */	120, 121, 122, 123, 124, 125, 126, 127
};

const struct skbd_keymap_table {
	platid_t	st_platform;
	const u_int8_t	*st_keymap;
	kbd_t		st_layout;
} skbd_keymap_table[] = {
	{{{PLATID_WILD, PLATID_MACH_COMPAQ_C}},
	 p7416_compaq_c_jp_keymap, KB_JP},

	{{{PLATID_WILD, PLATID_MACH_VICTOR_INTERLINK}},
	 m38813c_keymap, KB_JP},

	{{{0, 0}}, NULL, 0}
};
