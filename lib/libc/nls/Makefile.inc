#	$NetBSD: Makefile.inc,v 1.10.8.1 2012/04/17 00:05:22 yamt Exp $

.PATH: ${.CURDIR}/nls

SRCS+=	catclose.c catgets.c catopen.c 
MAN+=	catclose.3 catgets.3 catopen.3

# indirect reference stubs, to be removed soon.
SRCS+=	_catclose.c _catgets.c _catopen.c

CPPFLAGS.catopen.c+=	-I${LIBCDIR}/citrus

