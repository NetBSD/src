/*	$NetBSD: promcons.c,v 1.3.86.1 2009/01/19 13:16:57 skrll Exp $	*/


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

