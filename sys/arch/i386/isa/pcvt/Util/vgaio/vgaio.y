/*	$NetBSD: vgaio.y,v 1.2 1994/10/27 04:20:50 cgd Exp $	*/

%{
/*
 * Copyright (c) 1994 Joerg Wunsch
 *
 * All rights reserved.
 *
 * This program is free software.
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
 *	This product includes software developed by Joerg Wunsch
 * 4. The name of the developer may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPERS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE DEVELOPERS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ident "$NetBSD: vgaio.y,v 1.2 1994/10/27 04:20:50 cgd Exp $"

#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <machine/cpufunc.h>
#include <machine/console.h>

#include "vgaio.h"

void setreg(struct reg r, int val);
void getreg(struct reg r);
void yyerror(const char *msg);

#define YYDEBUG 1

unsigned short vgabase;

%}

%union {
	int num;
	struct reg reg;
}

%token GR CR SR AR NEWLINE
%token <num> NUM

%type <num> reggroup
%type <reg> register

%%

interpret:	lines ;

lines:		line
		| lines line
		;

line:		statements NEWLINE
		| NEWLINE
		| error	NEWLINE		{ fprintf(stderr, "bing!\n"); }
		;

statements:	statement
		| statements ';' statement
		;

statement:	register '?'		{ getreg($1); }
		| register '=' NUM	{ setreg($1, $3); }
		| /* lambda */
		;

register:	reggroup NUM 		{ $$.num = $2; $$.group = $1; }
		;

reggroup:	GR		{ $$ = GR; }
		| CR		{ $$ = CR; }
		| SR		{ $$ = SR; }
		| AR		{ $$ = AR; }
		;

%%

static struct {
	int id;
	const char *name;
} regnames[] = {
	{GR, "gr"}, {CR, "cr"}, {SR, "sr"}, {AR, "ar"},
	{0, 0}
};

const char *getname(struct reg r) {
	int idx;
	for(idx = 0; regnames[idx].id; idx++)
		if(regnames[idx].id == r.group)
			return regnames[idx].name;
	return "??";
}	

void getreg(struct reg r) {
	unsigned char val;

	switch(r.group) {
	case GR:
		outb(0x3ce, r.num);
		val = inb(0x3cf);
		break;

	case AR:
		r.num &= 0x1f;
		(void)inb(vgabase + 0x0a);
		outb(0x3c0, r.num + 0x20);
		val = inb(0x3c1);
		break;

	case CR:
		outb(vgabase + 4, r.num);
		val = inb(vgabase + 5);
		break;

	case SR:
		outb(0x3c4, r.num);
		val = inb(0x3c5);
		break;
	}
		
	printf("%s%x = 0x%x\n", getname(r), r.num, val);
}

void setreg(struct reg r, int val) {
	switch(r.group) {
	case GR:
		outb(0x3ce, r.num);
		outb(0x3cf, val);
		break;

	case AR:
		r.num &= 0x1f;
		(void)inb(vgabase + 0x0a);
		outb(0x3c0, r.num);
		outb(0x3c0, val);
		outb(0x3c0, r.num + 0x20);
		break;

	case CR:
		outb(vgabase + 4, r.num);
		outb(vgabase + 5, val);
		break;

	case SR:
		outb(0x3c4, r.num);
		outb(0x3c5, val);
		break;
	}
		
	printf("%s%x set to 0x%x now\n", getname(r), r.num, val);
}

void yyerror(const char *msg) {
	fprintf(stderr, "yyerror: %s\n", msg);
}

int main(int argc, char **argv) {
	int fd;

	if(argc > 1) yydebug = 1;

	if((fd = open("/dev/console", O_RDONLY)) < 0)
		fd = 0;

	if(ioctl(fd, KDENABIO, 0) < 0) {
		perror("ioctl(KDENABIO)");
		return 1;
	}
	vgabase = (inb(0x3cc) & 1)? 0x3d0: 0x3b0;
	yyparse();

	(void)ioctl(fd, KDDISABIO, 0);
	return 0;
}
