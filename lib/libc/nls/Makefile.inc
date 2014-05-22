#	$NetBSD: Makefile.inc,v 1.10.8.2 2014/05/22 11:36:53 yamt Exp $

.PATH: ${.CURDIR}/nls

SRCS+=	catclose.c catgets.c catopen.c 
MAN+=	catclose.3 catgets.3 catopen.3

CPPFLAGS.catopen.c+=	-I${LIBCDIR}/citrus

