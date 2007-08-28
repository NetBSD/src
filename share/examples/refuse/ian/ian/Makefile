# $NetBSD: Makefile,v 1.1 2007/08/28 19:45:02 agc Exp $

.include <bsd.own.mk>

LIBFETCH=${.CURDIR}/../libfetch 
VIRTDIR=${.CURDIR}/../../virtdir

LIBFETCHOBJDIR!= cd ${LIBFETCH} && ${PRINTOBJDIR}

PROG=ian
SRCS=ian.c virtdir.c
CPPFLAGS+=-I${LIBFETCH} -g -I${VIRTDIR}
LDADD+= -L${LIBFETCHOBJDIR} -Wl,-R${LIBFETCHOBJDIR} -lrefuse -lfetch -lssl -lcrypto

MAN=ian.8
WARNS=4

.PATH: ${VIRTDIR}

.include <bsd.prog.mk>
