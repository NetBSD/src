/*   $NetBSD: keymap.h,v 1.1.2.1 2007/01/21 12:05:54 blymn Exp $ */

/*
 * Copyright (c) 2005 The NetBSD Foundation Inc.
 * All rights reserved.
 *
 * This code is derived from code donated to the NetBSD Foundation
 * by Ruibiao Qiu <ruibiao@arl.wustl.edu,ruibiao@gmail.com>.
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _KEYMAP_H_
#define _KEYMAP_H_

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: keymap.h,v 1.1.2.1 2007/01/21 12:05:54 blymn Exp $");
#endif                          /* not lint */

/* keymap related stuff */
/*
 * moved here by Ruibiao Qiu
 * because it is needed by both getch() and get_wch()
 *
 * Keyboard input handler.  Do this by snarfing
 * all the info we can out of the termcap entry for TERM and putting it
 * into a set of keymaps.  A keymap is an array the size of all the possible
 * single characters we can get, the contents of the array is a structure
 * that contains the type of entry this character is (i.e. part/end of a
 * multi-char sequence or a plain char) and either a pointer which will point
 * to another keymap (in the case of a multi-char sequence) OR the data value
 * that this key should return.
 *
 */

/* private data structures for holding the key definitions */
typedef struct key_entry key_entry_t;

struct key_entry {
	short   type;		/* type of key this is */
	bool    enable;         /* true if the key is active */
	union {
		keymap_t *next;	/* next keymap is key is multi-key sequence */
		wchar_t   symbol;	/* key symbol if key is a leaf entry */
	} value;
};
/* Types of key structures we can have */
#define KEYMAP_MULTI  1		/* part of a multi char sequence */
#define KEYMAP_LEAF   2		/* key has a symbol associated with it, either
				 * it is the end of a multi-char sequence or a
				 * single char key that generates a symbol */

/* allocate this many key_entry structs at once to speed start up must
 * be a power of 2.
 */
#define KEYMAP_ALLOC_CHUNK 4

/* The max number of different chars we can receive */
#define MAX_CHAR 256

/*
 * Unused mapping flag.
 */
#define MAPPING_UNUSED (0 - MAX_CHAR) /* never been used */

struct keymap {
	int	count;	       /* count of number of key structs allocated */
	short	mapping[MAX_CHAR]; /* mapping of key to allocated structs */
	key_entry_t **key;     /* dynamic array of keys */
};

#define INC_POINTER(ptr)  do {	\
	(ptr)++;		\
	(ptr) %= INBUF_SZ;	\
} while(/*CONSTCOND*/0)

static short	state;		/* state of the inkey function */

#define INKEY_NORM	   0 /* no key backlog to process */
#define INKEY_ASSEMBLING   1 /* assembling a multi-key sequence */
#define INKEY_BACKOUT	   2 /* recovering from an unrecognised key */
#define INKEY_TIMEOUT	   3 /* multi-key sequence timeout */
#ifdef HAVE_WCHAR
#define INKEY_WCASSEMBLING 4 /* assembling a wide char sequence */
#endif /* HAVE_WCHAR */

/* The termcap data we are interested in and the symbols they map to */
struct tcdata {
	const char	*name;	/* name of termcap entry */
	wchar_t	symbol;		/* the symbol associated with it */
};

static const struct tcdata tc[] = {
	{"!1", KEY_SSAVE},
	{"!2", KEY_SSUSPEND},
	{"!3", KEY_SUNDO},
	{"#1", KEY_SHELP},
	{"#2", KEY_SHOME},
	{"#3", KEY_SIC},
	{"#4", KEY_SLEFT},
	{"%0", KEY_REDO},
	{"%1", KEY_HELP},
	{"%2", KEY_MARK},
	{"%3", KEY_MESSAGE},
	{"%4", KEY_MOVE},
	{"%5", KEY_NEXT},
	{"%6", KEY_OPEN},
	{"%7", KEY_OPTIONS},
	{"%8", KEY_PREVIOUS},
	{"%9", KEY_PRINT},
	{"%a", KEY_SMESSAGE},
	{"%b", KEY_SMOVE},
	{"%c", KEY_SNEXT},
	{"%d", KEY_SOPTIONS},
	{"%e", KEY_SPREVIOUS},
	{"%f", KEY_SPRINT},
	{"%g", KEY_SREDO},
	{"%h", KEY_SREPLACE},
	{"%i", KEY_SRIGHT},
	{"%j", KEY_SRSUME},
	{"&0", KEY_SCANCEL},
	{"&1", KEY_REFERENCE},
	{"&2", KEY_REFRESH},
	{"&3", KEY_REPLACE},
	{"&4", KEY_RESTART},
	{"&5", KEY_RESUME},
	{"&6", KEY_SAVE},
	{"&7", KEY_SUSPEND},
	{"&8", KEY_UNDO},
	{"&9", KEY_SBEG},
	{"*0", KEY_SFIND},
	{"*1", KEY_SCOMMAND},
	{"*2", KEY_SCOPY},
	{"*3", KEY_SCREATE},
	{"*4", KEY_SDC},
	{"*5", KEY_SDL},
	{"*6", KEY_SELECT},
	{"*7", KEY_SEND},
	{"*8", KEY_SEOL},
	{"*9", KEY_SEXIT},
	{"@0", KEY_FIND},
	{"@1", KEY_BEG},
	{"@2", KEY_CANCEL},
	{"@3", KEY_CLOSE},
	{"@4", KEY_COMMAND},
	{"@5", KEY_COPY},
	{"@6", KEY_CREATE},
	{"@7", KEY_END},
	{"@8", KEY_ENTER},
	{"@9", KEY_EXIT},
	{"F1", KEY_F(11)},
	{"F2", KEY_F(12)},
	{"F3", KEY_F(13)},
	{"F4", KEY_F(14)},
	{"F5", KEY_F(15)},
	{"F6", KEY_F(16)},
	{"F7", KEY_F(17)},
	{"F8", KEY_F(18)},
	{"F9", KEY_F(19)},
	{"FA", KEY_F(20)},
	{"FB", KEY_F(21)},
	{"FC", KEY_F(22)},
	{"FD", KEY_F(23)},
	{"FE", KEY_F(24)},
	{"FF", KEY_F(25)},
	{"FG", KEY_F(26)},
	{"FH", KEY_F(27)},
	{"FI", KEY_F(28)},
	{"FJ", KEY_F(29)},
	{"FK", KEY_F(30)},
	{"FL", KEY_F(31)},
	{"FM", KEY_F(32)},
	{"FN", KEY_F(33)},
	{"FO", KEY_F(34)},
	{"FP", KEY_F(35)},
	{"FQ", KEY_F(36)},
	{"FR", KEY_F(37)},
	{"FS", KEY_F(38)},
	{"FT", KEY_F(39)},
	{"FU", KEY_F(40)},
	{"FV", KEY_F(41)},
	{"FW", KEY_F(42)},
	{"FX", KEY_F(43)},
	{"FY", KEY_F(44)},
	{"FZ", KEY_F(45)},
	{"Fa", KEY_F(46)},
	{"Fb", KEY_F(47)},
	{"Fc", KEY_F(48)},
	{"Fd", KEY_F(49)},
	{"Fe", KEY_F(50)},
	{"Ff", KEY_F(51)},
	{"Fg", KEY_F(52)},
	{"Fh", KEY_F(53)},
	{"Fi", KEY_F(54)},
	{"Fj", KEY_F(55)},
	{"Fk", KEY_F(56)},
	{"Fl", KEY_F(57)},
	{"Fm", KEY_F(58)},
	{"Fn", KEY_F(59)},
	{"Fo", KEY_F(60)},
	{"Fp", KEY_F(61)},
	{"Fq", KEY_F(62)},
	{"Fr", KEY_F(63)},
	{"K1", KEY_A1},
	{"K2", KEY_B2},
	{"K3", KEY_A3},
	{"K4", KEY_C1},
	{"K5", KEY_C3},
	{"Km", KEY_MOUSE},
	{"k0", KEY_F0},
	{"k1", KEY_F(1)},
	{"k2", KEY_F(2)},
	{"k3", KEY_F(3)},
	{"k4", KEY_F(4)},
	{"k5", KEY_F(5)},
	{"k6", KEY_F(6)},
	{"k7", KEY_F(7)},
	{"k8", KEY_F(8)},
	{"k9", KEY_F(9)},
	{"k;", KEY_F(10)},
	{"kA", KEY_IL},
	{"ka", KEY_CATAB},
	{"kB", KEY_BTAB},
	{"kb", KEY_BACKSPACE},
	{"kC", KEY_CLEAR},
	{"kD", KEY_DC},
	{"kd", KEY_DOWN},
	{"kE", KEY_EOL},
	{"kF", KEY_SF},
	{"kH", KEY_LL},
	{"kh", KEY_HOME},
	{"kI", KEY_IC},
	{"kL", KEY_DL},
	{"kl", KEY_LEFT},
	{"kM", KEY_EIC},
	{"kN", KEY_NPAGE},
	{"kP", KEY_PPAGE},
	{"kR", KEY_SR},
	{"kr", KEY_RIGHT},
	{"kS", KEY_EOS},
	{"kT", KEY_STAB},
	{"kt", KEY_CTAB},
	{"ku", KEY_UP}
};
/* Number of TC entries .... */
static const int num_tcs = (sizeof(tc) / sizeof(struct tcdata));
#endif /* _KEYMAP_H_ */
