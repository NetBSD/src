#	$NetBSD: Makefile,v 1.37 2025/09/27 13:56:26 hgutch Exp $
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

LDADD+=	-lssl -lcrypto -lpthread
DPADD+= ${LIBSSL} ${LIBCRYPTO} ${LIBPTHREAD}

# Overflow that appears impossible
COPTS.syslogd.c+=        ${CC_WNO_FORMAT_TRUNCATION} ${CC_WNO_STRINGOP_TRUNCATION}
COPTS.tls.c+= -Wno-error=deprecated-declarations
COPTS.sign.c+= -Wno-error=deprecated-declarations
.if defined(HAVE_GCC) && ${ACTIVE_CC} == "gcc"
COPTS.sign.c+= -Wno-error=use-after-free
.endif

.include <bsd.prog.mk>
