# $NetBSD: Makefile,v 1.1.1.1.4.2 2008/01/09 01:39:00 matt Exp $

.include <bsd.own.mk>

PROG=iscsifs
SRCS=iscsifs.c virtdir.c initiator.c
LDADD+= -lrefuse -liscsi
CPPFLAGS+= -g -I${.CURDIR}/../virtdir -I${NETBSDSRCDIR}/dist/iscsi/include
MAN=iscsifs.8
WARNS=4

.PATH: ${.CURDIR}/../virtdir ${NETBSDSRCDIR}/dist/iscsi/src

.include <bsd.prog.mk>
