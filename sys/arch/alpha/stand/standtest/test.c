/* $NetBSD: test.c,v 1.1 1999/04/11 03:38:51 cgd Exp $ */

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>
#include <machine/autoconf.h>
#include <machine/rpb.h>

#include "../common/common.h"

struct cmdtab {
	const char *cmd;
	void (*fn)(const char *buf);
};

int		done;
unsigned long	arg_pfn, arg_ptb, arg_bim, arg_bip, arg_biv;

const char *advance_past_space(const char *buf);
int	dispatch_cmd(const char *buf, const struct cmdtab *cmds);
#define		DISPATCH_CMD_NOCMD	0
#define		DISPATCH_CMD_MATCHED	1
#define		DISPATCH_CMD_NOMATCH	2
#define		DISPATCH_CMD_AMBIGUOUS	3
void	print_cmds(const struct cmdtab *cmds, const char *match,
	    size_t matchlen);
void	print_stringarray(const char *s, size_t maxlen);

void	toplevel_db(const char *buf);
void	toplevel_dl(const char *buf);
void	toplevel_dq(const char *buf);
void	toplevel_dw(const char *buf);
void	toplevel_halt(const char *buf);
void	toplevel_help(const char *buf);
void	toplevel_show(const char *buf);

void	show_args(const char *buf);
void	show_bootinfo(const char *buf);
void	show_pt(const char *buf);
void	show_rpb(const char *buf);

void
main(pfn, ptb, bim, bip, biv)
	unsigned long pfn;	/* first free PFN number */
	unsigned long ptb;	/* PFN of current level 1 page table */
	unsigned long bim;	/* bootinfo magic */
	unsigned long bip;	/* bootinfo pointer */
	unsigned long biv;	/* bootinfo version */
{
	char input_buf[512];
	const struct cmdtab toplevel_cmds[] = {
	    {	"?",		toplevel_help,	},
#if 0 /* XXX notyet */
	    {	"db",		toplevel_db,	},
	    {	"dl",		toplevel_dl,	},
	    {	"dq",		toplevel_dq,	},
	    {	"dw",		toplevel_dw,	},
#endif
	    {	"quit",		toplevel_halt,	},
	    {	"show",		toplevel_show,	},
	    {	NULL,				},
	};

	printf("\n");
	printf("NetBSD/alpha " NETBSD_VERS
	    " Standalone Test Program, Revision %s\n", bootprog_rev);
	printf("(%s, %s)\n", bootprog_maker, bootprog_date);
	printf("\n");

	arg_pfn = pfn;
	arg_ptb = ptb;
	arg_bim = bim;
	arg_bip = bip;
	arg_biv = biv;

	printf("Enter '?' for help.\n");
	printf("\n");

	do {
		printf("test> ");
		gets(input_buf);

		dispatch_cmd(input_buf, toplevel_cmds);
	} while (!done);

	printf("\n");
	printf("halting...\n");
	halt();
}

const char *
advance_past_space(const char *buf)
{

	/* advance past white space. */
	while (isspace(*buf))
		buf++;

	if (*buf == '\0')
		return NULL;
	return buf;
}

int
dispatch_cmd(const char *buf, const struct cmdtab *cmds)
{
	const struct cmdtab *try, *winner;
	size_t nonwhitespace, i;
	unsigned int nmatches;
	const char *pre, *post;
	int rv;

	/* advance past white space. */
	buf = advance_past_space(buf);
	if (buf == NULL)
		return (DISPATCH_CMD_NOCMD);

	/* find how much non-white space there is. */
	nonwhitespace = 0;
	while ((buf[nonwhitespace] != '\0') && !isspace(buf[nonwhitespace]))
		nonwhitespace++;

	/* at this point, nonwhitespace should always be non-zero */
	if (nonwhitespace == 0) {
		printf("assertion failed: dispatch_cmd: nonwhitespace == 0\n");
		halt();
	}

	/* see how many matches there were. */
	for (nmatches = 0, try = cmds;
	    try != NULL && try->cmd != NULL;
	    try++) {
		if (strncmp(buf, try->cmd, nonwhitespace) == 0) {
			winner = try;
			nmatches++;
		}
	}

	if (nmatches == 1) {
		(*winner->fn)(buf + nonwhitespace);
		return (DISPATCH_CMD_MATCHED);
	} else if (nmatches == 0) {
		pre = "invalid command word";
		post = "allowed words";
		rv = DISPATCH_CMD_NOMATCH;
	} else {
		pre = "ambiguous command word";
		post = "matches";
		rv = DISPATCH_CMD_AMBIGUOUS;
	}

	printf("%s \"", pre);
	print_stringarray(buf, nonwhitespace);
	printf("\", %s:\n", post);

	/* print commands.  if no match, print all commands. */
	print_cmds(cmds, buf, rv == DISPATCH_CMD_NOMATCH ? 0 : nonwhitespace);
	return (rv);
}

void
print_cmds(const struct cmdtab *cmds, const char *match, size_t matchlen)
{
	const struct cmdtab *try;

	printf("    ");
	for (try = cmds; try != NULL && try->cmd != NULL; try++) {
		if (strncmp(match, try->cmd, matchlen) == 0)
			printf("%s%s", try != cmds ? ", " : "", try->cmd);
	}
	printf("\n");
}

void
print_stringarray(const char *s, size_t maxlen)
{
	size_t i;

	for (i = 0; (i < maxlen) && (*s != '\0'); i++, s++)
		putchar(*s);
}

void
warn_ignored_args(const char *buf, const char *cmd)
{

	if (advance_past_space(buf) != NULL)
		printf("WARNING: extra arguments to \"%s\" command ignored\n",
		    cmd);
}

/*
 * Top-level Commands
 */

void
toplevel_db(const char *buf)
{

	printf("\"db\" not yet implemented\n");
}

void
toplevel_dl(const char *buf)
{

	printf("\"dl\" not yet implemented\n");
}

void
toplevel_dq(const char *buf)
{

	printf("\"dq\" not yet implemented\n");
}

void
toplevel_dw(const char *buf)
{

	printf("\"dw\" not yet implemented\n");
}

void
toplevel_halt(const char *buf)
{

	warn_ignored_args(buf, "halt");

	done = 1;
}

void
toplevel_help(const char *buf)
{

	warn_ignored_args(buf, "?");

	printf("Standalone Test Program Commands:\n");
	printf("    ?                       print help\n");
	printf("    quit                    return to console\n");
#if 0 /* XXX notyet */
	printf("    db startaddr [count]    display memory (8-bit units)\n");
	printf("    dw startaddr [count]    display memory (16-bit units)\n");
	printf("    dl startaddr [count]    display memory (32-bit units)\n");
	printf("    dq startaddr [count]    display memory (64-bit units)\n");
#endif
	printf("    show args               show test program arguments\n");
	printf("    show bootinfo           show bootstrap bootinfo\n");
#if 0 /* XXX notyet */
	printf("    show pt [startaddr [endaddr]]\n");
	printf("                            show page tables\n");
	printf("    show rpb                show the HWRPB\n");
	printf("\n");
	printf("If optional \"count\" argument is omitted, 1 is used.\n");
	printf("If optional \"startaddr\" argument is omitted, "
	    "0x0 is used.\n");
	printf("If optional \"endaddr\" argument is omitted, "
	    "0xffffffffffffffff is used.\n");
#endif
}

void
toplevel_show(const char *buf)
{
	const struct cmdtab show_cmds[] = {
	    {	"args",		show_args,	},
	    {	"bootinfo",	show_bootinfo,	},
#if 0 /* XXX notyet */
	    {	"pt",		show_pt,	},
	    {	"rpb",		show_rpb,	},
#endif
	    {	NULL,				},
	};

	if (dispatch_cmd(buf, show_cmds) == DISPATCH_CMD_NOCMD) {
		printf("no subcommand given.  allowed subcommands:\n");
		print_cmds(show_cmds, NULL, 0);
	}
}


/*
 * Show Commands
 */

void
show_args(const char *buf)
{

	warn_ignored_args(buf, "show args");

	printf("first free page frame number:       0x%lx\n", arg_pfn);
	printf("page table base page frame number:  0x%lx\n", arg_ptb);
	printf("bootinfo magic number:              0x%lx\n", arg_bim);
	printf("bootinfo pointer:                   0x%lx\n", arg_bip);
	printf("bootinfo version:                   0x%lx\n", arg_biv);
}

void
show_bootinfo(const char *buf)
{
	u_long biv, bip;

	warn_ignored_args(buf, "show bootinfo");

	if (arg_bim != BOOTINFO_MAGIC) {
		printf("bootinfo magic number not present; no bootinfo\n");
		return;
	}

	bip = arg_bip;
	biv = arg_biv;
	if (biv == 0) {
		biv = *(u_long *)bip;
		bip += 8;
	}

	printf("bootinfo version: %d\n", biv);
	printf("bootinfo pointer: %p\n", (void *)bip);
	printf("bootinfo data:\n");

	switch (biv) {
	case 1: {
		const struct bootinfo_v1 *v1p;
		int i;

		v1p = (const struct bootinfo_v1 *)bip;
		printf("    ssym:          0x%lx\n", v1p->ssym);
		printf("    esym:          0x%lx\n", v1p->esym);
		printf("    boot flags:    \"");
		print_stringarray(v1p->boot_flags, sizeof v1p->boot_flags);
		printf("\"\n");
		printf("    booted kernel: \"", v1p->esym);
		print_stringarray(v1p->booted_kernel,
		    sizeof v1p->booted_kernel);
		printf("\"\n");
		printf("    hwrpb:         %p\n", v1p->hwrpb);
		printf("    hwrpbsize:     0x%lx\n", v1p->hwrpbsize);
		printf("    cngetc:        %p\n", v1p->cngetc);
		printf("    cnputc:        %p\n", v1p->cnputc);
		printf("    cnpollc:       %p\n", v1p->cnpollc);
		for (i = 0; i < (sizeof v1p->pad / sizeof v1p->pad[0]); i++) {
			printf("    pad[%d]:        0x%lx\n", i, v1p->pad[i]);
		}
		break;
	}
	default:
		printf("    unknown bootinfo version, cannot print data\n");
		break;
	}
}

void
show_pt(const char *buf)
{

	/* has additional args! */
	printf("\"show pt\" not yet implemented\n");
}

void
show_rpb(const char *buf)
{

	warn_ignored_args(buf, "show pt");

	printf("\"show rpb\" not yet implemented\n");
}
