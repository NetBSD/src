/*	$NetBSD: cmd.c,v 1.2.44.1 2017/12/03 11:36:13 jdolecek Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include "cmd.h"
#include "local.h"

struct cmd {
	char *name;
	int (*func)(int, char**, int);
} cmdtab[] = {
	{ "batch",	cmd_batch },
	{ "help",	cmd_help },
	{ "boot",	cmd_boot },
	{ "bootux",	cmd_boot_ux },
	{ "loadbin",	cmd_load_binary },
	{ "jump",	cmd_jump },
	{ "reboot",	cmd_reboot },
	{ "mem",	cmd_mem },
	{ "info",	cmd_info },

	{ "disklabel",	cmd_disklabel },
	{ "ls",		cmd_ls },
	{ "log_save",	cmd_log_save },

	{ "test",	cmd_test },
	{ "tlb",	cmd_tlb },
	{ "cop0",	cmd_cop0 },
	{ "ks",		cmd_kbd_scancode },
	{ "ether_test",	cmd_ether_test },
	{ "ga_test",	cmd_ga_test },
	{ 0, 0 }	/* terminate */
};

void
cmd_exec(const char *buf)
{
	char cmdbuf[CMDBUF_SIZE];
	char *argp[CMDARG_MAX];
	struct cmd *cmd;
	char *p;
	char *q;
	int i, argc, sep;

	strncpy(cmdbuf, buf, CMDBUF_SIZE);
	printf("%s\n", cmdbuf);

	/* find command */
	for (cmd = cmdtab; cmd->name; cmd++) {
		for (q = cmdbuf, p = cmd->name; *p; p++, q++) {
			if (*p != *q)
				break;
		}
		if (*p == 0 && (*q == '\0' || *q == ' '))
			goto found;
	}
	printf("***ERROR*** %s\n", cmdbuf);

	cmd_help(0, 0, 0);
	return;

 found:
	/* setup argument */
	p = cmdbuf;
	argc = 0;
	argp[argc++] = p;
	sep = 0;
	for (i = 0; (i < CMDBUF_SIZE) && (argc < CMDARG_MAX); i++, p++) {
		if (*p == ' ') {
			*p = 0;
			sep = 1;
		} else if (sep) {
			sep = 0;
			argp[argc++] = p;
		}
	}
	*--p = 0;

	cmd->func (argc, argp, 1);
}

int
cmd_batch(int argc, char *argp[], int interactive)
{
	struct cmd_batch_tab *p;
	char *args[CMDARG_MAX];
	int i;

	args[0] = "batch";
	for (p = cmd_batch_tab; p->func; p++) {
		for (i = 0; i < p->argc; i++)
			args[i + 1] = p->arg[i];

		if (p->func(p->argc + 1, args, interactive)) {
			printf("batch aborted.\n");
			return 1;
		}
	}

	return 0;
}

int
cmd_help(int argc, char *argp[], int interactive)
{
	struct cmd *cmd;

	printf("command: ");
	for (cmd = cmdtab; cmd->name; cmd++)
		printf("%s ", cmd->name);
	printf("\n");

	return 0;
}
