#	$NetBSD: Makefile,v 1.18 1997/10/19 13:01:25 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/29/93

WARNS=	0
PROG=	gprof
SRCS=	gprof.c arcs.c dfn.c lookup.c ${MACHINE_ARCH}.c hertz.c \
	printgprof.c printlist.c
CFLAGS+= -D${MACHINE_ARCH}

FILES=	${.CURDIR}/gprof.flat ${.CURDIR}/gprof.callg
FILESDIR=/usr/share/misc

.if make(install)
SUBDIR+=PSD.doc
.endif

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
