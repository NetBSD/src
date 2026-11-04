#	$NetBSD: Makefile.inc,v 1.12.38.1 2026/11/04 23:19:21 perseant Exp $

.PATH:	${.PARSEDIR}

SRCS+=	catclose.c catgets.c catopen.c 
MAN+=	catclose.3 catgets.3 catopen.3

CPPFLAGS.catopen.c+=	-I${LIBCDIR}/citrus

