#	$NetBSD: Makefile,v 1.40 2026/04/18 22:06:15 rillig Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
.include <bsd.own.mk>

WARNS?=4
USE_FORT?= yes	# network server

LINTFLAGS+=	-X 117	# bit shift on signed integers
LINTFLAGS+=	-X 132	# loss of accuracy
LINTFLAGS+=	-X 135	# pointer cast increases alignment
LINTFLAGS+=	-X 247	# pointer cast between unrelated types
LINTFLAGS+=	-X 298	# loss of accuracy in function call

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
