/*	$NetBSD: promcons.c,v 1.1.8.2 2001/06/14 12:57:16 fredette Exp $	*/


#include <sys/types.h>
#include <machine/mon.h>

int
getchar()
{
	return ( (*romVectorPtr->getChar)() );
}

int
peekchar()
{
	return ( (*romVectorPtr->mayGet)() );
}

void
putchar(c)
	int c;
{
	if (c == '\n')
		(*romVectorPtr->putChar)('\r');
	(*romVectorPtr->putChar)(c);
}

