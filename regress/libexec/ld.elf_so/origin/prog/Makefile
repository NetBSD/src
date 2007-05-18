
.include <bsd.own.mk>

NOMAN=1
PROG=origin
LIBFOO != cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
LDFLAGS+=-Wl,-R'$${ORIGIN}' -Wl,-L${LIBFOO} -lfoo
CPPFLAGS+=-I${.CURDIR}/../lib

${PROG}: libfoo.so.0

libfoo.so.0:
	cp ${LIBFOO}/libfoo.so.0 .

CLEANFILES+=libfoo.so

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
