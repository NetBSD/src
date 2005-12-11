/*	$NetBSD: promcons.c,v 1.3 2005/12/11 12:19:29 christos Exp $	*/


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

