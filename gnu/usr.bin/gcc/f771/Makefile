#	$NetBSD: Makefile,v 1.3 2001/10/08 23:18:11 tv Exp $

PROG=		f771
SRCS=		${G_F77_OBJS:.o=.c}

CPPFLAGS+=	-I${DIST}/gcc/f -I.
HOST_CPPFLAGS+=	-I${GCCARCH} -DUSE_HCONFIG= \
		${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}

.for f in 1t 2t fo io nq op ot
CLEANFILES+=	str-${f}.h str-${f}.j
.endfor

.include "../Makefile.backend"

fini: fini.c proj.c
	${HOST_LINK.c} -o $@ $>

.if !target(.BEGIN) && !make(fini)
.BEGIN:
	@rm -f fini
.endif

.for f in 1t 2t fo io nq op ot
str-${f}.h str-${f}.j: str-${f}.fin fini
	./fini ${>:M*.fin} str-${f}.j str-${f}.h

${OBJS} .depend: str-${f}.h
.endfor

.PATH: ${DIST}/gcc/f
