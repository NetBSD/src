#	$NetBSD: Makefile,v 1.2 2003/07/25 17:19:24 mrg Exp $

PROG=		f771
SRCS=		${G_F77_OBJS:.o=.c} main.c

CPPFLAGS+=	-I${DIST}/gcc/f -I.
HOST_CPPFLAGS+=	-I${GCCARCH} -DUSE_HCONFIG= \
		${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}

fini: fini.c
	${HOST_LINK.c} -o $@ $> ${HOSTLIBIBERTY}
CLEANFILES+=	fini

.for f in 1t 2t fo io nq op ot
CLEANFILES+=	str-${f}.h str-${f}.j

str-${f}.h str-${f}.j: str-${f}.fin fini
	./fini ${>:M*.fin} str-${f}.j str-${f}.h

${OBJS} .depend: str-${f}.h
.endfor

CLEANFILES+=	main.c

.include "../Makefile.backend"

main.c: ${DIST}/gcc/main.c
	rm -f main.c
	ln -s $> main.c

.PATH: ${DIST}/gcc/f ${DIST}/libiberty
