#	$NetBSD: Makefile,v 1.3.8.2 2000/12/08 09:28:45 bouyer Exp $

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
