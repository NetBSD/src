# $NetBSD: Makefile,v 1.4 2023/06/20 17:23:04 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH: ${HEIMDIST}/lib/kadm5

PROG= iprop-log
MAN=  iprop-log.8

HEIMSRCS= iprop-log.c iprop-commands.in

CPPFLAGS+= -I${DESTDIR}/usr/include/kadm5 -I${HEIMDIST}/lib/krb5

PROGDPLIBS += ${DPLIBKADM5SRV} ${DPLIBHDB} ${DPLIBSL}
LDADD+= -ledit -lterminfo
DPADD+= ${LIBEDIT} ${LIBTERMINFO}

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
