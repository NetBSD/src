#	$NetBSD: Makefile,v 1.6 2003/08/01 17:03:44 lukem Exp $

PROG=		f771
SRCS=		${G_F77_OBJS:.o=.c}

CPPFLAGS+=	-I${DIST}/gcc/f -I.
HOST_CPPFLAGS+=	-I${GCCARCH} -DUSE_HCONFIG= \
		${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}

.for f in 1t 2t fo io nq op ot
DPSRCS+=	str-${f}.h
CLEANFILES+=	str-${f}.h str-${f}.j
.endfor

.include "../Makefile.backend"

fini: fini.c proj.c
	${HOST_LINK.c} -o $@ $>
CLEANFILES+=	fini

.for f in 1t 2t fo io nq op ot
str-${f}.h str-${f}.j: str-${f}.fin fini
	./fini ${>:M*.fin} str-${f}.j str-${f}.h

.endfor

.PATH: ${DIST}/gcc/f
