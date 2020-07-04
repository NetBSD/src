/*	$NetBSD: pfilter.c,v 1.4 2020/07/04 05:18:37 lukem Exp $	*/

#include <stdio.h>
#include <unistd.h>
#include <blocklist.h>

#include "pfilter.h"

static struct blocklist *blstate;

void
pfilter_open(void)
{
	if (blstate == NULL)
		blstate = blocklist_open();
}

void
pfilter_notify(int what, const char *msg)
{
	pfilter_open();

	if (blstate == NULL)
		return;

	blocklist_r(blstate, what, STDIN_FILENO, msg);
}
