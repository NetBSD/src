/*	$NetBSD: promcons.c,v 1.1.24.1 2005/01/24 08:35:03 skrll Exp $	*/


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

