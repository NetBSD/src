/*	$NetBSD: skbdkeymap.h,v 1.6 2000/02/27 16:28:13 uch Exp $ */

/*
 * Copyright (c) 1999, 2000, by UCHIYAMA Yasushi
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
#define UNK	255	/* unknown */
#define IGN	254	/* ignore */
#define SPL	253	/* special key */

#define KEY_SPECIAL_OFF		0
#define KEY_SPECIAL_LIGHT	1

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

const int default_special_keymap[] = {
	[KEY_SPECIAL_OFF]	= -1,
	[KEY_SPECIAL_LIGHT]	= -1
};

const u_int8_t tc5165_mobilon_keymap[] = {
/*      0    1    2    3    4    5    6    7 */       
/* 0 */	37 , 45 , 44 , UNK, 9  , 51 , 23 , UNK,
/* 1 */	UNK, 56 , UNK, UNK, UNK, UNK, UNK, UNK,
/* 2 */	UNK, UNK, 29 , UNK, UNK, UNK, UNK, UNK,
/* 3 */	24 , 203, UNK, 38 , 10 , 27 , 13 , UNK,
/* 4 */	40 , UNK, UNK, 39 , 26 , 53 , 11 , 12 ,
/* 5 */	UNK, UNK, UNK, 53 , 25 , UNK, UNK, SPL, /* Light */
/* 6 */	208, UNK, UNK, UNK, 52 , UNK, 43 , 14 ,
/* 7 */	205, 200, UNK, UNK, SPL, UNK, UNK, 28 , /* Off key */
/* 8 */	UNK, 41 , 59 , 15 , 2  , UNK, UNK, UNK,
/* 9 */	63 , 64 , 1  , UNK, 65 , 16 , 17 , UNK,
/*10 */	60 , UNK, 61 , 62 , 3  , UNK, UNK, UNK,
/*11 */	UNK, UNK, UNK, 42 , 58 , UNK, UNK, UNK,
/*12 */	47 , 33 , 46 , 5  , 4  , 18 , 19 , UNK,
/*13 */	34 , 35 , 20 , 48 , 6  , 7  , 21 , 49 ,
/*14 */	22 , 31 , 32 , 36 , 8  , 30 , 50 , 57 ,
/*15 */	UNK, IGN, UNK, UNK, UNK, UNK, UNK, UNK /* Windows key */
};

const int tc5165_mobilon_special_keymap[] = {
	[KEY_SPECIAL_OFF]	= 60,
	[KEY_SPECIAL_LIGHT]	= 47
};

const u_int8_t tc5165_telios_jp_keymap[] = {
/*      0    1    2    3    4    5    6    7 */       
/* 0 */	58,  15,  IGN, 1,   IGN, IGN, IGN, IGN,
/* 1 */	IGN, IGN, IGN, IGN, 54,  42,  IGN, IGN,
/* 2 */	31,  18,  4,   IGN, IGN, 32,  45,  59,
/* 3 */	33,  19,  5,   61,  IGN, 46,  123, 60,
/* 4 */	35,  21,  8,   64,  IGN, 48,  49,  63,
/* 5 */	17,  16,  3,   IGN, 2,   30,  44,  41,
/* 6 */	IGN, IGN, IGN, IGN, IGN, IGN, IGN, IGN,
/* 7 */	IGN, IGN, IGN, IGN, IGN, IGN, 56,  IGN,
/* 8 */	34,  20,  7,   IGN, 6,   47,  57,  62,
/* 9 */	IGN, IGN, IGN, IGN, IGN, IGN, 29,  IGN,
/*10 */	27,  125, 13,  75,  80,  40,  115, 68,
/*11 */	39,  26,  25,  IGN, 12,  52,  53,  67,
/*12 */	37,  24,  11,  IGN, 10,  38,  51,  66,
/*13 */	23,  22,  9,   IGN, IGN, 36,  50,  65,
/*14 */	156, 43,  14,  72,  77,  IGN, IGN, 211,
/*15 */	IGN, IGN, IGN, IGN, IGN, IGN, 221, IGN
};

const u_int8_t tc5165_compaq_c_jp_keymap[] = {
/*      0    1    2    3    4    5    6    7 */       
/* 0 */	38,  50,  49,  48,  47,  46,  45,  44, 
/* 1 */	56,  IGN, IGN, IGN, IGN, IGN, IGN, IGN,
/* 2 */	13,  IGN, 112, 121, 123, 41,  28,  57, 
/* 3 */	77,  75,  80,  72,  39,  53,  52,  51,
/* 4 */	24,  25,  40,  IGN, 43,  26,  115, 58,
/* 5 */	54,  IGN, IGN, IGN, IGN, IGN, IGN, IGN,
/* 6 */	IGN, IGN, IGN, SPL, IGN, IGN, IGN, IGN, /* Light */
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

const int tc5165_compaq_c_jp_special_keymap[] = {
	[KEY_SPECIAL_OFF]	= -1, /* don't have off button */
	[KEY_SPECIAL_LIGHT]	= 51
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
	const int	*st_special;
	kbd_t		st_layout;
} skbd_keymap_table[] = {
	{{{PLATID_WILD, PLATID_MACH_COMPAQ_C}},
	 tc5165_compaq_c_jp_keymap, 
	 tc5165_compaq_c_jp_special_keymap,
	 KB_JP},

	{{{PLATID_WILD, PLATID_MACH_VICTOR_INTERLINK}},
	 m38813c_keymap, 
	 default_special_keymap,
	 KB_JP},

	{{{PLATID_WILD, PLATID_MACH_SHARP_TELIOS}},
	 tc5165_telios_jp_keymap, 
	 default_special_keymap,
	 KB_JP},

	{{{PLATID_WILD, PLATID_MACH_SHARP_MOBILON}},
	 tc5165_mobilon_keymap, 
	 tc5165_mobilon_special_keymap,
	 KB_US},

	{{{0, 0}}, NULL, 0}
};
