# $NetBSD: Makefile,v 1.1.2.2 2007/11/06 23:12:43 matt Exp $

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
