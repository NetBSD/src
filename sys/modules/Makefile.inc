#	$NetBSD: Makefile.inc,v 1.8.4.1 2019/11/21 18:17:59 martin Exp $

S!=		cd ${.PARSEDIR}/..;pwd
CPPFLAGS+=	-I${NETBSDSRCDIR}/common/include
USE_FORT=	no
WARNS?=		5

# inexpensive kernel consistency checks
# XXX to be commented out on release branch
# CPPFLAGS+=	-DDIAGNOSTIC

.if !empty(IOCONF)
_BSD_IOCONF_MK_USER_=1
.include <bsd.ioconf.mk>
.endif

.include <bsd.own.mk>
