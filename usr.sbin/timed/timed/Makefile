#	@(#)Makefile	5.11 (Berkeley) 6/29/90
#
# optional flags are: MEASURE TESTING DEBUG

PROG=	timed
SRCS=	acksend.c candidate.c correct.c master.c networkdelta.c readmsg.c \
	slave.c timed.c byteorder.c measure.c cksum.${MACHINE}.c
DPADD=	${LIBUTIL}
LDADD=	-lutil
MAN8=	timed.0

.if ${MACHINE} == "i386"
CLEANFILES+=cksum.i386.c
.elif ${MACHINE} == "hp300"
CLEANFILES+=cksum.hp300.c
.endif

cksum.hp300.c cksum.i386.c:
	@if [ -d ${.CURDIR}/obj ]; then \
		/bin/rm -rf ${.CURDIR}/obj/$@ ; \
		ln -s ${.CURDIR}/cksum.tahoe.c ${.CURDIR}/obj/$@ ; \
	else \
		true; \
		/bin/rm -rf ${.CURDIR}/$@ ; \
		ln -s ${.CURDIR}/cksum.tahoe.c ${.CURDIR}/$@ ; \
	fi

.include "../../Makefile.inc"
.include <bsd.prog.mk>
