/*	$NetBSD: promcons.c,v 1.1.32.1 2005/04/29 11:28:27 kent Exp $	*/


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

