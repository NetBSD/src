/*	$NetBSD: keyconf.c,v 1.3 1999/02/15 04:36:34 hubertf Exp $	*/
/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Sony Corp. and Kazumasa Utashiro of Software Research Associates, Inc.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: $Hdr: keyconf.c,v 4.300 91/06/09 06:14:53 root Rel41 $ SONY
 *
 *	@(#)keyconf.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/keyboard.h>
#include <newsmips/dev/kbreg.h>

#define FN(n,name,addr,len1,str1,len2,str2) {addr, {len1, str1}, {len2, str2}}

Pfk_table pfk_init[] = {
	FN(00,	"",		0,	0,	NULL,	0,	NULL),
	FN(01,	"F1",		1,	3,  "\033OP",	0,	NULL),
	FN(02,	"F2",		2,	3,  "\033OQ",	0,	NULL),
	FN(03,	"F3",		3,	3,  "\033OR",	0,	NULL),
	FN(04,	"F4",		4,	3,  "\033OS",	0,	NULL),
	FN(05,	"F5",		5,	3,  "\033OT",	0,	NULL),
	FN(06,	"F6",		6,	3,  "\033OU",	0,	NULL),
	FN(07,	"F7",		7,	3,  "\033OV",	0,	NULL),
	FN(08,	"F8",		8,	3,  "\033OW",	0,	NULL),
	FN(09,	"F9",		9,	3,  "\033OX",	0,	NULL),
	FN(10,	"F10",		10,	3,  "\033OY",	0,	NULL),
	FN(11,	"PF1",		12,	0,	NULL,	0,	NULL),
	FN(12,	"PF2",		13,	0,	NULL,	0,	NULL),
	FN(13,	"PF3",		14,	0,	NULL,	0,	NULL),
	FN(14,	"PF4",		15,	0,	NULL,	0,	NULL),
	FN(15,	"PF5",		16,	0,	NULL,	0,	NULL),
	FN(16,	"PF6",		17,	0,	NULL,	0,	NULL),
	FN(17,	"PF7",		18,	0,	NULL,	0,	NULL),
	FN(18,	"PF8",		19,	0,	NULL,	0,	NULL),
	FN(19,	"PF9",		20,	0,	NULL,	0,	NULL),
	FN(20,	"PF10",		21,	0,	NULL,	0,	NULL),
	FN(21,	"PF11",		22,	0,	NULL,	0,	NULL),
	FN(22,	"PF12",		23,	0,	NULL,	0,	NULL),
	FN(23,	"ncnv",		69,	0,	NULL,	0,	NULL),
	FN(24,	"conv",		71,	0,	NULL,	0,	NULL),
	FN(25,	"enter",	74,	0,	NULL,	0,	NULL),
	FN(26,	"0",		87,	1,	"0",	1,	"0"),
	FN(27,	"1",		83,	1,	"1",	1,	"1"),
	FN(28,	"2",		84,	1,	"2",	1,	"2"),
	FN(29,	"3",		85,	1,	"3",	1,	"3"),
	FN(30,	"4",		79,	1,	"4",	1,	"4"),
	FN(31,	"5",		80,	1,	"5",	1,	"5"),
	FN(32,	"6",		81,	1,	"6",	1,	"6"),
	FN(33,	"7",		75,	1,	"7",	1,	"7"),
	FN(34,	"8",		76,	1,	"8",	1,	"8"),
	FN(35,	"9",		77,	1,	"9",	1,	"9"),
	FN(36,	".",		89,	1,	".",	1,	"."),
	FN(37,	"-",		78,	1,	"-",	1,	"/"),
	FN(38,	"+",		82,	1,	"+",	1,	"*"),
	FN(39,	",",		86,	1,	",",	1,	"="),
	FN(40,	"enter",	90,	1,	"\r",	1,	"\r"),
	FN(41,	"up",		88,	3,  "\033[A",	3,  "\033[A"),
	FN(42,	"down",		92,	3,  "\033[B",	3,  "\033[B"),
	FN(43,	"rignt",	93,	3,  "\033[C",	3,  "\033[C"),
	FN(44,	"left",		91,	3,  "\033[D",	3,  "\033[D"),
};
#undef FN

#define FN(n,flags,norm,shift,ctrl,alt,kana,kshift) \
	{flags, norm, shift, ctrl, alt, kana, kshift}

Key_table default_table[] = {
/*	 key_flags		normal	shift	ctrl	alt	kana	kshft */
  FN( 0, 0,			0,	0,	0,	0,	0,	0),
  FN( 1, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN( 2, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN( 3, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN( 4, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN( 5, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN( 6, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN( 7, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN( 8, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN( 9, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(10, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(11, N|S|C|K|J|O,		0x1b,	0x1b,	0x1b,	0,	0x1b,	0x1b),
  FN(12, N|S|K|L|ALT_FUNC,	'1',	'!',	0,	0,	0xc7,	0),
  FN(13, N|S|C|K|L|ALT_FUNC,	'2',	'@',	0x00,	0,	0xcc,	0),
  FN(14, N|S|C|K|J|L|ALT_FUNC,	'3',	'#',	0x1b,	0,	0xb1,	0xa7),
  FN(15, N|S|C|K|J|L|ALT_FUNC,	'4',	'$',	0x1c,	0,	0xb3,	0xa9),
  FN(16, N|S|C|K|J|L|ALT_FUNC,	'5',	'%',	0x1d,	0,	0xb4,	0xaa),
  FN(17, N|S|C|K|J|R|ALT_FUNC,	'6',	'^',	0x1e,	0,	0xb5,	0xab),
  FN(18, N|S|C|K|J|R|ALT_FUNC,	'7',	'&',	0x1f,	0,	0xd4,	0xac),
  FN(19, N|S|C|K|J|R|ALT_FUNC,	'8',	'*',	0x0f,	0,	0xd5,	0xad),
  FN(20, N|S|K|K|J|R|ALT_FUNC,	'9',	'(',	0,	0,	0xd6,	0xae),
  FN(21, N|S|K|K|J|R|ALT_FUNC,	'0',	')',	0,	0,	0xdc,	0xa6),
  FN(22, N|S|K|R|ALT_FUNC,	'-',	'_',	0,	0,	0xce,	0),
  FN(23, N|S|K|R|ALT_FUNC,	'=',	'+',	0,	0,	0xcd,	0),
  FN(24, N|S|C|L,		'\\',	'|',	0x1c,	0,	0,	0),
  FN(25, N|S|C|K|J|O,		'\b',	'\b',	'\b',	0,	'\b',	'\b'),
  FN(26, N|S|C|K|J|O,		'\t',	'\t',	'\t',	0,	'\t',	'\t'),
  FN(27, N|S|C|K|L|CAP_LOCK,	'q',	'Q',	0x11,	0,	0xc0,	0),
  FN(28, N|S|C|K|L|CAP_LOCK,	'w',	'W',	0x17,	0,	0xc3,	0),
  FN(29, N|S|C|K|L|J|CAP_LOCK,	'e',	'E',	0x05,	0,	0xb2,	0xa8),
  FN(30, N|S|C|K|L|CAP_LOCK,	'r',	'R',	0x12,	0,	0xbd,	0),
  FN(31, N|S|C|K|L|CAP_LOCK,	't',	'T',	0x14,	0,	0xb6,	0),
  FN(32, N|S|C|K|R|CAP_LOCK,	'y',	'Y',	0x19,	0,	0xdd,	0),
  FN(33, N|S|C|K|R|CAP_LOCK,	'u',	'U',	0x15,	0,	0xc5,	0),
  FN(34, N|S|C|K|R|CAP_LOCK,	'i',	'I',	'\t',	0,	0xc6,	0),
  FN(35, N|S|C|K|R|CAP_LOCK,	'o',	'O',	0x0f,	0,	0xd7,	0),
  FN(36, N|S|C|K|R|CAP_LOCK,	'p',	'P',	0x10,	0,	0xbe,	0),
  FN(37, N|S|C|K|R,		'[',	'{',	0x1b,	0,	0xde,	0),
  FN(38, N|S|C|K|R|J,		']',	'}',	0x1d,	0,	0xdf,	0xa2),
  FN(39, N|C|K|O,		0x7f,	0,	0x7f,	0,	0x7f,	0),
  FN(40, PSH_SHFT,		S_CTRL,	0,	0,	0,	0,	0),
  FN(41, N|S|C|K|L|CAP_LOCK,	'a',	'A',	0x01,	0,	0xc1,	0),
  FN(42, N|S|C|K|L|CAP_LOCK,	's',	'S',	0x13,	0,	0xc4,	0),
  FN(43, N|S|C|K|L|CAP_LOCK,	'd',	'D',	0x04,	0,	0xbc,	0),
  FN(44, N|S|C|K|L|CAP_LOCK,	'f',	'F',	0x06,	0,	0xca,	0),
  FN(45, N|S|C|K|L|CAP_LOCK,	'g',	'G',	0x07,	0,	0xb7,	0),
  FN(46, N|S|C|K|R|CAP_LOCK,	'h',	'H',	'\b',	0,	0xb8,	0),
  FN(47, N|S|C|K|R|CAP_LOCK,	'j',	'J',	'\n',	0,	0xcf,	0),
  FN(48, N|S|C|K|R|CAP_LOCK,	'k',	'K',	0x0b,	0,	0xc9,	0),
  FN(49, N|S|C|K|R|CAP_LOCK,	'l',	'L',	'\f',	0,	0xd8,	0),
  FN(50, N|S|K|R,		';',	':',	0,	0,	0xda,	0),
  FN(51, N|S|K|L,		'\'',	'"',	0,	0,	0xb9,	0),
  FN(52, N|S|C|K|L|J,		'`',	'~',	0x1e,	0,	0xd1,	0xa3),
  FN(53, N|S|C|K|J|O,		'\r',	'\r',	'\r',	0,	'\r',	'\r'),
  FN(54, PSH_SHFT,		S_LSHFT,0,	0,	0,	0,	0),
  FN(55, N|S|C|K|J|L|CAP_LOCK,	'z',	'Z',	0x1a,	0,	0xc2,	0xaf),
  FN(56, N|S|C|K|L|CAP_LOCK,	'x',	'X',	0x18,	0,	0xbb,	0),
  FN(57, N|S|C|K|L|CAP_LOCK,	'c',	'C',	0x03,	0,	0xbf,	0),
  FN(58, N|S|C|K|L|CAP_LOCK,	'v',	'V',	0x16,	0,	0xcb,	0),
  FN(59, N|S|C|K|L|CAP_LOCK,	'b',	'B',	0x02,	0,	0xba,	0),
  FN(60, N|S|C|K|R|CAP_LOCK,	'n',	'N',	0x0e,	0,	0xd0,	0),
  FN(61, N|S|C|K|R|CAP_LOCK,	'm',	'M',	'\r',	0,	0xd3,	0),
  FN(62, N|S|K|J|R,		',',	'<',	0,	0,	0xc8,	0xa4),
  FN(63, N|S|K|J|R,		'.',	'>',	0,	0,	0xd9,	0xa1),
  FN(64, N|S|C|K|J|R,		'/',	'?',	0x1f,	0,	0xd2,	0xa5),
  FN(65, K|J,			0,	0,	0,	0,	0xdb,	0xb0),
  FN(66, PSH_SHFT,		S_RSHFT,0,	0,	0,	0,	0),
  FN(67, PSH_SHFT|NOT_REPT,	S_ALT,	0,	0,	0,	0,	0),
  FN(68, PSH_SHFT|NOT_REPT,	S_CAPS,	0,	0,	0,	0,	0),
  FN(69, PRG_FUNC|NOT_REPT,	0,	0,	0,	0,	0,	0),
  FN(70, N|S|C|K|J|O,		' ',	' ',	0x00,	0,	' ',	' '),
  FN(71, PRG_FUNC|NOT_REPT,	0,	0,	0,	0,	0,	0),
  FN(72, SW_SHFT|NOT_REPT,	S_AN,	0,	0,	0,	0,	0),
  FN(73, SW_SHFT|NOT_REPT,	S_KANA,	0,	0,	0,	0,	0),
  FN(74, PRG_FUNC|NOT_REPT,	0,	0,	0,	0,	0,	0),
  FN(75, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(76, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(77, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(78, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(79, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(80, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(81, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(82, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(83, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(84, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(85, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(86, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(87, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(88, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(89, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(90, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(91, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(92, PRG_FUNC,		0,	0,	0,	0,	0,	0),
  FN(93, PRG_FUNC,		0,	0,	0,	0,	0,	0),
};
#undef FN

#define	PFK_SIZE	(sizeof(pfk_init) / sizeof(Pfk_table))
int N_Pfk = PFK_SIZE;
Pfk_table pfk_table[PFK_SIZE];

#define KEYTAB_SIZE	(sizeof(default_table) / sizeof(default_table[0]))
Key_table key_table[KEYTAB_SIZE];

int country = K_JAPANESE_J;

void
init_key_table()
{
	bcopy(default_table, key_table, sizeof(key_table));
}
