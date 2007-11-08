# $NetBSD: Makefile,v 1.1.1.1 2007/11/08 23:12:03 agc Exp $

.include <bsd.own.mk>

PROG=iscsifs
SRCS=iscsifs.c virtdir.c initiator.c
LDADD+= -lrefuse -liscsi
CPPFLAGS+= -g -I${.CURDIR}/../virtdir -I${NETBSDSRCDIR}/dist/iscsi/include
MAN=iscsifs.8
WARNS=4

.PATH: ${.CURDIR}/../virtdir ${NETBSDSRCDIR}/dist/iscsi/src

.include <bsd.prog.mk>
