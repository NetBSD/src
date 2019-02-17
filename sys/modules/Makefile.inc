#	$NetBSD: Makefile.inc,v 1.7 2019/02/17 03:57:31 rin Exp $

S!=		cd ${.PARSEDIR}/..;pwd
CPPFLAGS+=	-I${NETBSDSRCDIR}/common/include
USE_FORT=	no
WARNS?=		3

# inexpensive kernel consistency checks
# XXX to be commented out on release branch
CPPFLAGS+=	-DDIAGNOSTIC

.if !empty(IOCONF)
_BSD_IOCONF_MK_USER_=1
.include <bsd.ioconf.mk>
.endif

.include <bsd.own.mk>
