# $NetBSD: Makefile,v 1.3 2010/09/01 17:25:57 agc Exp $

PROG=hkpd
SRCS=hkpd.c main.c
CPPFLAGS+=-g # -DHAVE_CONFIG_H=1
LDFLAGS+=-g
LDADD+= -lnetpgp
MAN=hkpd.8
WARNS=0 # anything over 0 will fail at the link stage with IDEA errors

.include <bsd.prog.mk>

t: ${PROG}
	./${PROG} -D &
	sleep 1
	ftp -o- 'http://localhost:11371/pks/lookup?op=index&search=agc&options=json'
	ftp -o- 'http://localhost:11371/pks/lookup?op=get&search=agc&options=json'
	ftp -o- 'http://localhost:11371/pks/lookup?op=get&search=agc&options=mr'
	pkill hkpd
