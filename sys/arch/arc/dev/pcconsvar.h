/*	$NetBSD: pcconsvar.h,v 1.1 2001/06/13 15:05:43 soda Exp $	*/
/*	$OpenBSD: pccons.c,v 1.22 1999/01/30 22:39:37 imp Exp $	*/
/*	NetBSD: pccons.c,v 1.89 1995/05/04 19:35:20 cgd Exp	*/

/*-
 * Copyright (c) 1993, 1994, 1995 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
 *
 * Copyright (c) 1994 Charles M. Hannum.
 * Copyright (c) 1992, 1993 Erik Forsberg.
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
 *	@(#)pccons.c	5.11 (Berkeley) 5/21/91
 */

struct pccons_config {
	bus_addr_t pc_mono_iobase, pc_mono_memaddr;
	bus_addr_t pc_cga_iobase, pc_cga_memaddr;
	bus_addr_t pc_kbd_cmdp, pc_kbd_datap;

	void (*pc_init) __P((void));
};

struct pccons_kbd_context {
	bus_space_tag_t pkc_iot;
	bus_space_handle_t pkc_cmd_ioh, pkc_data_ioh;

	int pkc_initialized;
};

struct pccons_context {
	struct pccons_kbd_context pc_pkc;

	bus_space_tag_t pc_crt_iot, pc_crt_memt;

	bus_space_handle_t pc_6845_ioh, pc_crt_memh;

	bus_space_handle_t pc_mono_ioh, pc_mono_memh;
	bus_space_handle_t pc_cga_ioh, pc_cga_memh;

	struct pccons_config *pc_config;

	int pc_initialized;
};

struct tty;

struct pc_softc {
	struct	device sc_dev;
	struct	tty *sc_tty;

#if 0
	struct	pccons_context *sc_pc;	/* pccons device software context */
#endif
};

#define kbd_cmd_read_1() \
	bus_space_read_1(pccons_console_context.pc_pkc.pkc_iot, \
	    pccons_console_context.pc_pkc.pkc_cmd_ioh, 0)
#define kbd_data_read_1() \
	bus_space_read_1(pccons_console_context.pc_pkc.pkc_iot, \
	    pccons_console_context.pc_pkc.pkc_data_ioh, 0)
#define kbd_cmd_write_1(cmd) \
	bus_space_write_1(pccons_console_context.pc_pkc.pkc_iot, \
	    pccons_console_context.pc_pkc.pkc_cmd_ioh, 0, cmd)
#define kbd_data_write_1(data) \
	bus_space_write_1(pccons_console_context.pc_pkc.pkc_iot, \
	    pccons_console_context.pc_pkc.pkc_data_ioh, 0, data)

extern struct pccons_context pccons_console_context;

void kbd_context_init __P((bus_space_tag_t, struct pccons_config *));
int kbc_put8042cmd __P((u_char));
void kbd_flush_input __P((void));

int pccons_common_match __P((bus_space_tag_t,
	bus_space_tag_t, bus_space_tag_t, struct pccons_config *));
void pccons_common_attach __P((struct pc_softc *, bus_space_tag_t,
	bus_space_tag_t, bus_space_tag_t, struct pccons_config *));
void pccons_common_cnattach __P((bus_space_tag_t, bus_space_tag_t,
	bus_space_tag_t, struct pccons_config *));
int pcintr __P((void *));
