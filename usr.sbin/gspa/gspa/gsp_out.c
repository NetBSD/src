/*	$NetBSD: gsp_out.c,v 1.7 2002/05/27 21:11:56 wiz Exp $	*/
/*
 * GSP assembler - binary & listing output
 *
 * Copyright (c) 1993 Paul Mackerras.
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
 *      This product includes software developed by Paul Mackerras.
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
#ifndef lint
__RCSID("$NetBSD: gsp_out.c,v 1.7 2002/05/27 21:11:56 wiz Exp $");
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gsp_ass.h"

u_int16_t codes[5];
unsigned ncode;
unsigned code_idx;
short show_pc;
short show_val;
int32_t val_to_show;
extern unsigned line_pc;

unsigned obj_addr = 0;

extern FILE *objfile, *listfile;
extern char line[];

extern char *c_name;
u_int16_t c_buf[4096];
u_int32_t c_bufptr, c_binads;

void c_checkbuf(void);
void c_dumpbuf(void);

struct error {
	struct error *next;
	char	string[1];
};

struct error *error_list, *error_last;

void do_list_pc(void);
void do_show_val(int32_t);
void listing_line(void);
void put1code(u_int16_t);
void show_errors(void);

void
putcode(u_int16_t *v, int n)
{
	for( ; n > 0; --n )
		put1code(*v++);
}

void
put1code(u_int16_t v)
{
	if( code_idx >= 3 )
		listing_line();
	codes[code_idx] = v;
	if( objfile != NULL ){
		if( pc != obj_addr ){
			if (c_name) {
				if (c_bufptr > 0)
					c_dumpbuf();
				c_binads = pc;
				c_bufptr = 0;
			} else {
				/* expect this only when ncode == 0 */
				if (ncode % 8 != 0)
					fprintf(objfile, "\n");
				fprintf(objfile, "@%x\n", pc);
			}
			obj_addr = pc;
		} else {
			if((ncode % 8 != 0) && !c_name)
				fprintf(objfile, " ");
		}
		if (c_name) {
			c_checkbuf();
			c_buf[c_bufptr++] = v;
		} else
			fprintf(objfile, "%.4X", v & 0xFFFF);
		obj_addr += 0x10;
		if((ncode % 8 == 7) && !c_name)
			fprintf(objfile, "\n");
	}
	++ncode;
	++code_idx;
	pc += 0x10;
	show_pc = TRUE;
}

void
c_checkbuf() 
{
	if (c_bufptr > (sizeof(c_buf)/sizeof(*c_buf)))
		c_dumpbuf();
}

void
c_dumpbuf()
{
	int i;

	fprintf(objfile, "\n\n\t%d, 0x%04x, 0x%04x, /* new block */",
	    c_bufptr, (int)(c_binads >> 16), (int)(c_binads & 0xffff));

	for (i=0; i < c_bufptr; ++i) {
		if (i%8 == 0)
			fprintf(objfile, "\n\t");
		fprintf(objfile, "0x%04x, ", c_buf[i]);
	}
	c_binads += c_bufptr;
	c_bufptr = 0;
}

void
start_at(u_int32_t val)
{
	if( objfile != NULL ) {
		if (c_name) {
			c_checkbuf();
			fprintf(objfile,
			    "\n\n\t2, 0xffff, 0xfee0, 0x%04x, 0x%04x,"
			      "\n\t2, 0xffff, 0xffe0, 0x%04x, 0x%04x,\n",
			    val & 0xffff, val >> 16, val & 0xffff, val >> 16);
		} else
			fprintf(objfile, ":%lX\n", (long)val);
	}
}

void
do_list_pc()
{
	if( pass2 )
		show_pc = TRUE;
}

void
do_show_val(int32_t v)
{
	if( ncode == 0 ){
		val_to_show = v;
		show_val = TRUE;
		show_pc = FALSE;
	}
}

void
list_error(char *string)
{
	struct error *p;
	int l;

	if( listfile == NULL )
		return;
	l = strlen(string);
	p = (struct error *) alloc(sizeof(struct error) + l);
	strcpy(p->string, string);
	p->next = NULL;
	if( error_list == NULL )
		error_list = p;
	else
		error_last->next = p;
	error_last = p;
}

void
show_errors()
{
	struct error *p, *q;

	for( p = error_list; p != NULL; p = q ){
		if( listfile != NULL )
			fprintf(listfile, "\t\t\t%s\n", p->string);
		q = p->next;
		free(p);
	}
	error_list = error_last = NULL;
}

void
listing()
{
	if( objfile != NULL && ncode % 8 != 0 && !c_name)
		fprintf(objfile, "\n");
	listing_line();
	show_errors();
	ncode = 0;
	show_pc = FALSE;
}

void
listing_line()
{
	int i;

	if( listfile == NULL ){
		code_idx = 0;
		return;
	}
	if( show_pc )
		fprintf(listfile, "%.8X", line_pc);
	else
		fprintf(listfile, "        ");
	if( show_val ){
		fprintf(listfile, "  %.8X", val_to_show);
		i = 2;
	} else {
		for( i = 0; i < code_idx; ++i )
			fprintf(listfile, " %.4X", codes[i]);
	}
	if( ncode <= 3 ){
		for( ; i < 3; ++i )
			fprintf(listfile, "     ");
		fprintf(listfile, " %s", line);
	} else
		fprintf(listfile, "\n");
	line_pc += code_idx << 4;
	code_idx = 0;
	show_val = FALSE;
}
