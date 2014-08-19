#	$NetBSD: Makefile.inc,v 1.11.6.1 2014/08/20 00:02:15 tls Exp $

.PATH: ${.CURDIR}/nls

SRCS+=	catclose.c catgets.c catopen.c 
MAN+=	catclose.3 catgets.3 catopen.3

CPPFLAGS.catopen.c+=	-I${LIBCDIR}/citrus

