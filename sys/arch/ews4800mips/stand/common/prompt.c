/*	$NetBSD: prompt.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <machine/param.h>
#include <machine/bfs.h>

#include "cmd.h"
#include "local.h"

#define	LOG_SIZE	2048
#define	LOG_MASK	(LOG_SIZE - 1)
uint8_t __log[LOG_SIZE];
int __log_cnt;

#define	CMDBUF_HISTORY_MAX	8
#define	PROMPT			">> "

struct cmd_buf {
	char buf[CMDBUF_SIZE];
	int cur, min, max;
} __cmd_buf[CMDBUF_HISTORY_MAX];

struct cmd_buf *__cmd;
int __cur_cmd;

void prompt_reset(void);
void prompt_input(int);
extern void __putchar(int);

void
prompt(void)
{
	int c;

	prompt_reset();
	while (/*CONSTCOND*/1)
		if ((c = getchar()) != 0)
			prompt_input(c);
	/* NOTREACHED */
}

void
prompt_input(int c)
{
	int i;

	switch (c) {
	case 32 ... 127:
		__cmd->buf[__cmd->cur] = c;
		__cmd->cur = (__cmd->cur >= CMDBUF_SIZE - 1) ? __cmd->cur :
		    __cmd->cur + 1;
		if (__cmd->cur >= __cmd->max)
			__cmd->max = __cmd->cur;
		putchar(c);
		break;
	case '\r':
		putchar('\n');
		if (__cmd->max > 0) {
			cmd_exec(__cmd->buf);
			prompt_reset();
		} else {
			printf(PROMPT);
		}
		break;
	case '\b':
		if (__cmd->cur > 0) {
			__cmd->buf[--__cmd->cur] = 0;
			putchar(c);
		}
		break;
	case 1:		/* Ctrl a */
		__cmd->cur = __cmd->min;
		putchar('\r');
		printf("%s", PROMPT);
		break;
	case 5:		/* Ctrl e */
		__cmd->cur = __cmd->max;
		goto redraw;
	case 2:		/* Ctrl b */
		if (__cmd->cur == __cmd->min)
			return;
		__cmd->cur--;
		putchar(c);
		break;
	case 6:		/* Ctrl f */
		if (__cmd->cur == __cmd->max)
			return;
		__cmd->cur++;
		putchar(c);
		break;
	case 4:		/* Ctrl d */
		if (__cmd->cur == __cmd->max)
			return;
		for (i = __cmd->cur; i < __cmd->max; i++)
			__cmd->buf[i] = __cmd->buf[i + 1];
		__cmd->buf[i] = '\0';
		__cmd->max--;
		goto redraw;
	case 11:	/* Ctrl k */
		for (i = __cmd->cur; i < __cmd->max; i++) {
			__cmd->buf[i] = 0;
			__cmd->max = __cmd->cur;
		}
		putchar(c);
		break;
	case 14:	/* Ctrl n */
		__cur_cmd = (__cur_cmd + 1) & 0x7;
		goto history_redraw;
	case 16:	/* Ctrl p */
		__cur_cmd = (__cur_cmd - 1) & 0x7;
	history_redraw:
		__cmd = &__cmd_buf[__cur_cmd];
		__cmd->cur = __cmd->max;
	case 12:	/* Ctrl l */
	redraw:
		putchar('\r');
		putchar(11);
		printf("%s%s", PROMPT, __cmd->buf);
		break;
	}
}

void
prompt_reset(void)
{

	__cur_cmd = (__cur_cmd + 1) & 0x7;
	__cmd = &__cmd_buf[__cur_cmd];
	memset(__cmd, 0, sizeof *__cmd);
	printf(PROMPT);
}

boolean_t
prompt_yesno(int interactive)
{
	int i;

	if (!interactive)
		return TRUE;
	/* block until user input */
	while (/*CONSTCOND*/1) {
		if ((i = getchar()) == 0)
			continue;
		if (i == 'N' || i == 'n')
			return FALSE;
		if (i == 'Y' || i == 'y')
			return TRUE;
	}
}

void
putchar(int c)
{

	__putchar(c);

	__log[__log_cnt++] = c;
	__log_cnt &= LOG_MASK;
}

int
cmd_log_save(int argc, char *argp[], int interactive)
{
	struct bfs *bfs;

	if (bfs_init(&bfs) != 0) {
		printf("no BFS partition.\n");
		return 1;
	}

	if (bfs_file_write(bfs, "boot.log", __log, LOG_SIZE) != 0)
		printf("BFS write failed.\n");

	bfs_fini(bfs);

	return 0;
}
