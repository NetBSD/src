/*	$NetBSD: promcons.c,v 1.2 1998/01/05 07:03:39 perry Exp $	*/


#include <stdarg.h>
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

