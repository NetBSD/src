#	$NetBSD: Makefile,v 1.10 2003/11/16 22:49:48 lukem Exp $

PROG=		f771
SRCS=		${G_F77_OBJS:.o=.c} main.c

CPPFLAGS+=	-I${DIST}/gcc/f -I.
HOST_CPPFLAGS+=	-I${GCCARCH} -I${BACKENDOBJ} -DUSE_HCONFIG= \
		${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}

.for f in 1t 2t fo io nq op ot
DPSRCS+=	str-${f}.h
.endfor

fini: fini.lo
	${_MKTARGET_LINK}
	${HOST_LINK.c} -o ${.TARGET} ${.ALLSRC} ${HOSTLIBIBERTY}
CLEANFILES+=	fini fini.lo

.include "../Makefile.backend"

.for f in 1t 2t fo io nq op ot
CLEANFILES+=	str-${f}.h str-${f}.j

str-${f}.h str-${f}.j: str-${f}.fin fini
	${_MKTARGET_CREATE}
	./fini ${>:M*.fin} str-${f}.j str-${f}.h
.endfor

CLEANFILES+=	main.c

main.c: ${DIST}/gcc/main.c
	rm -f main.c
	ln -s $> main.c

.PATH: ${DIST}/gcc/f ${DIST}/libiberty ${G_out_file:H}
