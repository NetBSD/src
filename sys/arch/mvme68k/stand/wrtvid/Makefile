#	$NetBSD: Makefile,v 1.5 2000/12/04 21:24:34 scw Exp $

S=${.CURDIR}/../../../../
PROG=	wrtvid
MKMAN=	no
LIBSA=${.CURDIR}/../../../../lib/libsa
COPTS+= -Wall -Wstrict-prototypes -Wmissing-prototypes
CPPFLAGS+= -I${LIBSA}

.PATH.c: ${LIBSA}

SRCS=	wrtvid.c loadfile.c

# only needed during build
proginstall::

.include <bsd.prog.mk>
