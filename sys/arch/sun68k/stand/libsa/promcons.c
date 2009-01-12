/*	$NetBSD: promcons.c,v 1.4 2009/01/12 07:00:59 tsutsui Exp $	*/


#include <sys/types.h>
#include <machine/mon.h>
#include <lib/libsa/stand.h>

#include "libsa.h"

int 
getchar(void)
{
	return ( (*romVectorPtr->getChar)() );
}

int 
peekchar(void)
{
	return ( (*romVectorPtr->mayGet)() );
}

void 
putchar(int c)
{
	if (c == '\n')
		(*romVectorPtr->putChar)('\r');
	(*romVectorPtr->putChar)(c);
}

