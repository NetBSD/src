#	$NetBSD: Makefile,v 1.2 2001/07/18 13:59:17 tv Exp $

PROG=		f771
SRCS=		${G_F77_OBJS:.o=.c}

CPPFLAGS+=	-I${DIST}/gcc/f -I.
HOST_CPPFLAGS+=	-I${GCCARCH} -DUSE_HCONFIG= \
		${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}

.include "../Makefile.backend"

# See comment in ../backend/Makefile for the reason for
# this ${MAKE} dance.

fini: fini.c proj.c
	${HOST_LINK.c} -o $@ $>

.if !target(.BEGIN) && !make(fini)
.BEGIN:
	@rm -f fini
.endif

.for f in 1t 2t fo io nq op ot
CLEANFILES+=	str-${f}.h str-${f}.j

str-${f}.h str-${f}.j: str-${f}.fin fini.c proj.c
	@cd ${.CURDIR} && ${MAKE} fini
	./fini ${>:M*.fin} str-${f}.j str-${f}.h

${OBJS} .depend: str-${f}.h
.endfor

.PATH: ${DIST}/gcc/f
