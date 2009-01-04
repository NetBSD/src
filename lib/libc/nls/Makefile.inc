#	$NetBSD: Makefile.inc,v 1.8.62.1 2009/01/04 17:02:20 christos Exp $

.PATH: ${.CURDIR}/nls

SRCS+=	catclose.c catgets.c catopen.c 
MAN+=	catclose.3 catgets.3 catopen.3

# indirect reference stubs, to be removed soon.
SRCS+=	_catclose.c _catgets.c _catopen.c

.if ${CITRUS} == "yes"
CPPFLAGS.catopen.c+=	-DHAVE_CITRUS -I${LIBCDIR}/citrus
.else
CPPFLAGS.catopen.c+=	-UHAVE_CITRUS -I${LIBCDIR}/locale
.endif

