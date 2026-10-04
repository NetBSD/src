#	$NetBSD: Makefile.inc,v 1.13 2026/10/04 03:28:45 mrg Exp $

.PATH:	${.PARSEDIR}

SRCS+=	catclose.c catgets.c catopen.c 
MAN+=	catclose.3 catgets.3 catopen.3

CPPFLAGS.catopen.c+=	-I${LIBCDIR}/citrus

