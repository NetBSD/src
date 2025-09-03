# $NetBSD: Makefile,v 1.4 2025/07/19 11:26:23 christos Exp $

.include <bsd.own.mk>
.include "${.CURDIR}/../Makefile.inc"

PROG=hkpd
BINDIR=/usr/sbin
SRCS=hkpd.c main.c

MAN=hkpd.1

.PATH: ${.CURDIR}/../../dist/src/hkpd

.include <bsd.prog.mk>

t: ${PROG}
	./${PROG} -D &
	sleep 1
	ftp -o- 'http://localhost:11371/pks/lookup?op=index&search=agc&options=json'
	ftp -o- 'http://localhost:11371/pks/lookup?op=get&search=agc&options=json'
	ftp -o- 'http://localhost:11371/pks/lookup?op=get&search=agc&options=mr'
	pkill hkpd
