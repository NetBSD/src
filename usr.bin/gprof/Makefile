#	$NetBSD: Makefile,v 1.23 1998/02/23 10:09:32 jonathan Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/29/93

.include <bsd.own.mk>			# for OBJECT_FMT definition

.if (${OBJECT_FMT} == "a.out")
PROG=	gprof
.endif

SRCS=	gprof.c arcs.c dfn.c lookup.c ${MACHINE_ARCH}.c hertz.c \
	printgprof.c printlist.c
CPPFLAGS+= -D${MACHINE_ARCH}

FILES=	${.CURDIR}/gprof.flat ${.CURDIR}/gprof.callg
FILESDIR=/usr/share/misc

.if make(install)
SUBDIR+=PSD.doc
.endif

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
