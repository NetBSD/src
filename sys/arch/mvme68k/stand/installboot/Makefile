#	$NetBSD: Makefile,v 1.4.8.2 2000/12/08 09:28:42 bouyer Exp $

.include <bsd.own.mk>

PROG=	installboot
MKMAN=	no
BINDIR=/usr/mdec
LIBSA=${.CURDIR}/../../../../lib/libsa
COPTS+= -Wall -Wstrict-prototypes -Wmissing-prototypes
CPPFLAGS+= -I${LIBSA}
# Need this to work in the miniroot
LDSTATIC?=	-static

.PATH.c: ${LIBSA}

SRCS=	installboot.c loadfile.c

.include <bsd.prog.mk>
