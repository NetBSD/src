/*	$NetBSD: promcons.c,v 1.4.2.1 1998/01/27 02:35:36 gwr Exp $	*/


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

