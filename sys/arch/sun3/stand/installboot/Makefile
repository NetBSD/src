#	$NetBSD: Makefile,v 1.7.8.2 2001/03/12 13:29:37 bouyer Exp $

.include <bsd.own.mk>

PROG=	installboot
MKMAN= no
NOMAN= yes
BINDIR=/usr/mdec
LIBSA=${.CURDIR}/../../../../lib/libsa
COPTS+= -Wall -Wstrict-prototypes -Wmissing-prototypes
CPPFLAGS+= -I${LIBSA}

# Need this to work in the miniroot
LDSTATIC?=	-static

.PATH.c: ${LIBSA}

SRCS=	installboot.c loadfile.c

.include <bsd.prog.mk>
