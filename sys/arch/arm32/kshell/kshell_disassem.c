/* $NetBSD: kshell_disassem.c,v 1.4 1996/10/11 00:07:08 christos Exp $ */

/*
 * Copyright (c) 1994 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * shell_disassem.c
 *
 * Debug / Monitor shell disassembler
 *
 * Created      : 09/10/94
 */

/* Include standard header files */

#include <sys/param.h>
#include <sys/systm.h>
#include <dev/cons.h>

#include <machine/katelib.h>

/* Declare external variables */

/* Local function prototypes */

u_int disassemble __P((u_char *));
u_int do_disassemble __P((u_char *));
int readhex __P((char *buf));

/* Now for the main code */

/* dis - disassembles memory */

void
shell_disassem(argc, argv)
	int argc;
	char *argv[];
{
	u_char *addr;

	if (argc < 2) {
		kprintf("Syntax: dis <addr>\n\r");
		return;
	}

/* Decode the one argument */

	addr = (u_char *)readhex(argv[1]);

	kprintf("Interactive disassembly\n\r");

	do_disassemble(addr);
}


u_int
do_disassemble(addr)
	u_char *addr;
{
	u_int result;
	int quit = 0;
	int key = 0;
	int count = 1;

	do {
		result = disassemble(addr);

		--count;

		if (count == 0) {
			count = 1;

			key = cngetc();

			switch (key) {
			case 'Q' :
			case 'q' :
			case 0x1b :
			case 0x03 :
			case 0x04 :
			case 0x10b :
				quit = 1;
				break;
			case 0x09 :
			case 'r' :
			case 'R' :
			case 0x103 :
				count = 16;
				addr += 4;
				break;

			case 0x102 :
				count = 16;
				addr -= 124;
				break;

			case 0x0d :
			case 0x101 :
				addr = addr + 4;
				break;

			case 'B' :
			case 'b' :
			case 0x100:
				addr = addr - 4;
				break;

			case '+' :
			case '=' :
			case 0x104 :
				addr = addr + 0x80;
				break;

			case '-' :
			case '_' :
			case 0x105 :
				addr = addr - 0x80;
				break;

			case ' ' :
				quit = do_disassemble((u_char *)result);
				break;

			case 'J' :
			case 'j' :
				addr = (u_char *)result;
				break;

			case '/' :
			case '?' :
				kprintf("\'\xe3\'  - Backwards 1 word\n\r");
				kprintf("\'\xe4\'  - Forwards 1 word\n\r");
				kprintf("\'\xe5\'  - Backwords 16 words\n\r");
				kprintf("\'\xe6\'  - Forwards 16 words\n\r");
				kprintf("\'Q\'  - Quit\n\r");
				kprintf("\'B\'  - Back a word\n\r");
				kprintf("\'R\'  - Disassemble 16 words\n\r");
				kprintf("\'J\'  - Jump to address\n\r");
				kprintf("\' \'  - Branch to address\n\r");
				kprintf("<BS> - Return from branch\n\r");
				kprintf("\'-\'  - Skip backwards 128 words\n\r");
				kprintf("\'+\'  - Skip forwards 128 words\n\r");
				break;

			}
		} else {
			addr += 4;
		}
	} while (!quit && key != 0x08);

	return(quit);
}

/* End of shell_disassem.c */
