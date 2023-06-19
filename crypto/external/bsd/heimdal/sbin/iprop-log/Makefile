# $NetBSD: Makefile,v 1.3 2023/06/19 23:11:45 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH: ${HEIMDIST}/lib/kadm5 ${HEIMDIST}/lib/hcrypto

PROG= iprop-log
MAN=  iprop-log.8

CPPFLAGS+=-I${HEIMDIST}/lib

HEIMSRCS= iprop-log.c iprop-commands.in ui.c

CPPFLAGS+= -I${DESTDIR}/usr/include/kadm5 -I${HEIMDIST}/lib/krb5

LDADD+= -lkadm5srv -lhdb -lsl
LDADD+= -ledit -lterminfo
DPADD+= ${LIBKADM5SRV} ${LIBHDB} ${LIBSL}
DPADD+= ${LIBEDIT} ${LIBTERMINFO}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
