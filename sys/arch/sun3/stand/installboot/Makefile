#	$NetBSD: Makefile,v 1.10 2001/03/21 10:58:54 tsutsui Exp $

.include <bsd.own.mk>

PROG=	installboot
MKMAN= no
NOMAN= yes
BINDIR=/usr/mdec
S= ${.CURDIR}/../../../..
LIBSA=${S}/lib/libsa
COPTS+= -Wall -Wstrict-prototypes -Wmissing-prototypes
CPPFLAGS+= -I${LIBSA} -I.

# Need this to work in the miniroot
LDSTATIC?=	-static

.PATH.c: ${LIBSA}

SRCS=	installboot.c loadfile.c

# for machine/loadfile_machdep.h
CLEANFILES+= machine m68k

.BEGIN: machine m68k
.NOPATH: machine m68k
realdepend realall: machine m68k

.include <bsd.prog.mk>
