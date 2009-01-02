#	$NetBSD: Makefile.inc,v 1.9 2009/01/02 00:20:21 tnozaki Exp $

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

