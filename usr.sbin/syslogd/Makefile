#	$NetBSD: Makefile,v 1.39 2025/10/19 01:00:00 riastradh Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
.include <bsd.own.mk>

WARNS?=4
USE_FORT?= yes	# network server

LINTFLAGS+=-X 132,247,135,259,117,298

PROG=	syslogd
SRCS=	syslogd.c utmpentry.c tls.c sign.c
MAN=	syslogd.8 syslog.conf.5
DPADD+=${LIBUTIL} ${LIBEVENT}
LDADD+=-lutil -levent
#make symlink to old socket location for transitional period
SYMLINKS=	/var/run/log /dev/log
.PATH.c: ${NETBSDSRCDIR}/usr.bin/who
CPPFLAGS+=-I${NETBSDSRCDIR}/usr.bin/who -DSUPPORT_UTMPX -DSUPPORT_UTMP -Wredundant-decls

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

CPPFLAGS+=-DLIBWRAP
.if ${HAVE_OPENSSL} < 11
CPPFLAGS+=-DOPENSSL_API_COMPAT=0x10100000L
.endif
LDADD+=	-lwrap -lblocklist 
DPADD+=	${LIBWRAP} ${LIBBLOCKLIST} 

LDADD+=	-lssl -lcrypto
DPADD+= ${LIBSSL} ${LIBCRYPTO}

# Overflow that appears impossible
COPTS.syslogd.c+=        ${CC_WNO_FORMAT_TRUNCATION} ${CC_WNO_STRINGOP_TRUNCATION}
COPTS.tls.c+= -Wno-error=deprecated-declarations
COPTS.sign.c+= -Wno-error=deprecated-declarations
COPTS.sign.c+= ${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 14:? -Wno-error=use-after-free :}

.include <bsd.prog.mk>
