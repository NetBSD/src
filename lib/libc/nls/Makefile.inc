#	$Id: Makefile.inc,v 1.5 1994/10/06 05:44:39 jtc Exp $

.PATH: ${.CURDIR}/nls

SRCS+=	catclose.c catgets.c catopen.c msgcat.c
MAN3+=	catclose.0 catgets.0 catopen.0
