/*	$NetBSD: promcons.c,v 1.2 2005/01/22 15:36:11 chs Exp $	*/


#include <sys/types.h>
#include <machine/mon.h>

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

