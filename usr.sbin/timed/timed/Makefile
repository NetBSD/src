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
.endif

cksum.i386.c:
	@if [ -d ${.CURDIR}/obj ]; then \
		/bin/rm -rf ${.CURDIR}/obj/cksum.i386.c ; \
		ln -s ${.CURDIR}/cksum.tahoe.c ${.CURDIR}/obj/cksum.i386.c ; \
	else \
		true; \
		/bin/rm -rf ${.CURDIR}/cksum.i386.c ; \
		ln -s ${.CURDIR}/cksum.tahoe.c ${.CURDIR}/cksum.i386.c ; \
	fi

.include "../../Makefile.inc"
.include <bsd.prog.mk>
