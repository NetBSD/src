#	$NetBSD: Makefile,v 1.18.2.3 1998/11/09 07:57:20 cgd Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/29/93

WARNS=	0
# XXX not ELF?
.if (${MACHINE_ARCH} != "alpha" && ${MACHINE_ARCH} != "powerpc")
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
