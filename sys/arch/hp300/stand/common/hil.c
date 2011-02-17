/*	$NetBSD: hil.c,v 1.12.30.2 2011/02/17 11:59:41 bouyer Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 * from: Utah Hdr: hil.c 1.1 89/08/22
 *
 *	@(#)hil.c	8.1 (Berkeley) 6/10/93
 */

/*
 * HIL keyboard routines for the standalone ITE.
 */

#if defined(ITECONSOLE) && defined(HIL_KEYBOARD)

#include <sys/param.h>

#include <hp300/stand/common/hilreg.h>
#include <hp300/stand/common/kbdmap.h>
#include <hp300/stand/common/itevar.h>

#include <hp300/stand/common/samachdep.h>
#include <hp300/stand/common/kbdvar.h>

#ifndef SMALL

/*
 * HIL cooked keyboard keymaps.
 * Supports only unshifted, shifted and control keys.
 */
char hil_us_keymap[] = {
	'\0',	'`',	'\\',	ESC,	'\0',	DEL,	'\0',	'\0',  
	'\n',	'\t',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',  
	'\0',	'\n',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',  
	'\0',	'\t',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\b',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	ESC,	'\r',	'\0',	'\n',	'0',	'.',	',',	'+',
	'1',	'2',	'3',	'-',	'4',	'5',	'6',	'*',
	'7',	'8',	'9',	'/',	'E',	'(',	')',	'^',
	'1',	'2',	'3',	'4',	'5',	'6',	'7',	'8',
	'9',	'0',	'-',	'=',	'[',	']',	';',	'\'',
	',',	'.',	'/',	'\040',	'o',	'p',	'k',	'l',
	'q',	'w',	'e',	'r',	't',	'y',	'u',	'i',
	'a',	's',	'd',	'f',	'g',	'h',	'j',	'm',
	'z',	'x',	'c',	'v',	'b',	'n',	'\0',	'\0'
};

char hil_us_shiftmap[] = {
	'\0',	'~',	'|',	DEL,	'\0',	DEL,	'\0',	'\0',
	'\n',	'\t',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\n',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\t',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	DEL,	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	ESC,	'\r',	'\0',	'\n',	'0',	'.',	',',	'+',
	'1',	'2',	'3',	'-',	'4',	'5',	'6',	'*',
	'7',	'8',	'9',	'/',	'`',	'|',	'\\',	'~',
	'!',	'@',	'#',	'$',	'%',	'^',	'&',	'*',
	'(',	')',	'_',	'+',	'{',	'}',	':',	'\"',
	'<',	'>',	'?',	'\040',	'O',	'P',	'K',	'L',
	'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
	'A',	'S',	'D',	'F',	'G',	'H',	'J',	'M',
	'Z',	'X',	'C',	'V',	'B',	'N',	'\0',	'\0'
};

char hil_us_ctrlmap[] = {
	'\0',	'`',	'\034',	ESC,	'\0',	DEL,	'\0',	'\0',
	'\n',	'\t',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\n',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\t',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\b',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	ESC,	'\r',	'\0',	'\n',	'0',	'.',	',',	'+',
	'1',	'2',	'3',	'-',	'4',	'5',	'6',	'*',
	'7',	'8',	'9',	'/',	'E',	'(',	')',	'\036',
	'1',	'2',	'3',	'4',	'5',	'6',	'7',	'8',
	'9',	'0',	'-',	'=',	'\033',	'\035',	';',	'\'',
	',',	'.',	'/',	'\040',	'\017',	'\020',	'\013',	'\014',
	'\021',	'\027',	'\005',	'\022',	'\024',	'\031',	'\025',	'\011',
	'\001',	'\023',	'\004',	'\006',	'\007',	'\010',	'\012',	'\015',
	'\032',	'\030',	'\003',	'\026',	'\002',	'\016',	'\0',	'\0'
};

#ifdef UK_KEYBOARD
char hil_uk_keymap[] = {
	'\0',	'`',	'<',	ESC,	'\0',	DEL,	'\0',	'\0',  
	'\n',	'\t',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',  
	'\0',	'\n',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',  
	'\0',	'\t',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\b',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	ESC,	'\r',	'\0',	'\n',	'0',	'.',	',',	'+',
	'1',	'2',	'3',	'-',	'4',	'5',	'6',	'*',
	'7',	'8',	'9',	'/',	'E',	'(',	')',	'^',
	'1',	'2',	'3',	'4',	'5',	'6',	'7',	'8',
	'9',	'0',	'+',	'\'',	'[',	']',	'*',	'\\',
	',',	'.',	'-',	'\040',	'o',	'p',	'k',	'l',
	'q',	'w',	'e',	'r',	't',	'y',	'u',	'i',
	'a',	's',	'd',	'f',	'g',	'h',	'j',	'm',
	'z',	'x',	'c',	'v',	'b',	'n',	'\0',	'\0'
};

char hil_uk_shiftmap[] = {
	'\0',	'~',	'>',	DEL,	'\0',	DEL,	'\0',	'\0',
	'\n',	'\t',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\n',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\t',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	DEL,	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	ESC,	'\r',	'\0',	'\n',	'0',	'.',	',',	'+',
	'1',	'2',	'3',	'-',	'4',	'5',	'6',	'*',
	'7',	'8',	'9',	'/',	'`',	'|',	'\\',	'~',
	'!',	'\"',	'#',	'$',	'%',	'&',	'^',	'(',
	')',	'=',	'?',	'/',	'{',	'}',	'@',	'|',
	';',	':',	'_',	'\040',	'O',	'P',	'K',	'L',
	'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
	'A',	'S',	'D',	'F',	'G',	'H',	'J',	'M',
	'Z',	'X',	'C',	'V',	'B',	'N',	'\0',	'\0'
};

char hil_uk_ctrlmap[] = {
	'\0',	'`',	'<',	ESC,	'\0',	DEL,	'\0',	'\0',
	'\n',	'\t',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\n',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\t',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\b',	'\0',
	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',	'\0',
	ESC,	'\r',	'\0',	'\n',	'0',	'.',	',',	'+',
	'1',	'2',	'3',	'-',	'4',	'5',	'6',	'*',
	'7',	'8',	'9',	'/',	'E',	'(',	')',	'\036',
	'1',	'2',	'3',	'4',	'5',	'6',	'7',	'8',
	'9',	'0',	'+',	'\'',	'\033',	'\035',	'*',	'\034',
	',',	'.',	'/',	'\040',	'\017',	'\020',	'\013',	'\014',
	'\021',	'\027',	'\005',	'\022',	'\024',	'\031',	'\025',	'\011',
	'\001',	'\023',	'\004',	'\006',	'\007',	'\010',	'\012',	'\015',
	'\032',	'\030',	'\003',	'\026',	'\002',	'\016',	'\0',	'\0'
};
#endif

/*
 * The keyboard map table.
 * Lookup is by hardware returned language code.
 */
struct kbdmap hilkbd_map[] = {
	{ KBD_US, "",
	    hil_us_keymap, hil_us_shiftmap, hil_us_ctrlmap, NULL, NULL},

#ifdef UK_KEYBOARD
	{ KBD_UK, "",
	    hil_uk_keymap, hil_uk_shiftmap, hil_uk_ctrlmap, NULL, NULL},
#endif

	{ 0, "",
	    NULL, NULL, NULL, NULL, NULL},
};

char	*hilkbd_keymap = hil_us_keymap;
char	*hilkbd_shiftmap = hil_us_shiftmap;
char	*hilkbd_ctrlmap = hil_us_ctrlmap;

int
hilkbd_getc(void)
{
	int status, c;
	struct hil_dev *hiladdr = HILADDR;

	status = hiladdr->hil_stat;
	if ((status & HIL_DATA_RDY) == 0)
		return 0;
	c = hiladdr->hil_data;
	switch ((status>>KBD_SSHIFT) & KBD_SMASK) {
	case KBD_SHIFT:
		c = hilkbd_shiftmap[c & KBD_CHARMASK];
		break;
	case KBD_CTRL:
		c = hilkbd_ctrlmap[c & KBD_CHARMASK];
		break;
	case KBD_KEY:
		c = hilkbd_keymap[c & KBD_CHARMASK];
		break;
	default:
		c = 0;
		break;
	}
	return c;
}
#endif /* SMALL */

void
hilkbd_nmi(void)
{
	struct hil_dev *hiladdr = HILADDR;

	HILWAIT(hiladdr);
	hiladdr->hil_cmd = HIL_CNMT;
	HILWAIT(hiladdr);
	hiladdr->hil_cmd = HIL_CNMT;
	HILWAIT(hiladdr);
}

int
hilkbd_init(void)
{
	struct hil_dev *hiladdr = HILADDR;
	struct kbdmap *km;
	u_char lang;

	/*
	 * Determine the existence of a HIL keyboard.
	 */
	HILWAIT(hiladdr);
	hiladdr->hil_cmd = HIL_READKBDSADR;
	HILDATAWAIT(hiladdr);
	lang = hiladdr->hil_data;
	if (lang == 0)
		return 0;

	HILWAIT(hiladdr);
	hiladdr->hil_cmd = HIL_SETARR;
	HILWAIT(hiladdr);
	hiladdr->hil_data = ar_format(KBD_ARR);
	HILWAIT(hiladdr);
	hiladdr->hil_cmd = HIL_READKBDLANG;
	HILDATAWAIT(hiladdr);
	lang = hiladdr->hil_data;
	for (km = hilkbd_map; km->kbd_code; km++) {
		if (km->kbd_code == lang) {
			hilkbd_keymap = km->kbd_keymap;
			hilkbd_shiftmap = km->kbd_shiftmap;
			hilkbd_ctrlmap = km->kbd_ctrlmap;
		}
	}
	HILWAIT(hiladdr);
	hiladdr->hil_cmd = HIL_INTON;
	return 1;
}
#endif /* ITECONSOLE && HIL_KEYBOARD */
