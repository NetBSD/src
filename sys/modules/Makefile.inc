#	$NetBSD: Makefile.inc,v 1.8 2019/02/17 04:05:41 rin Exp $

S!=		cd ${.PARSEDIR}/..;pwd
CPPFLAGS+=	-I${NETBSDSRCDIR}/common/include
USE_FORT=	no
WARNS?=		5

# inexpensive kernel consistency checks
# XXX to be commented out on release branch
CPPFLAGS+=	-DDIAGNOSTIC

.if !empty(IOCONF)
_BSD_IOCONF_MK_USER_=1
.include <bsd.ioconf.mk>
.endif

.include <bsd.own.mk>
