/*	$NetBSD: promcons.c,v 1.5 1998/02/05 04:57:14 gwr Exp $	*/


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

