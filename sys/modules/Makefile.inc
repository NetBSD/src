#	$NetBSD: Makefile.inc,v 1.5 2011/09/10 08:27:06 mbalmer Exp $

S!=		cd ${.PARSEDIR}/..;pwd
CPPFLAGS+=	-I${NETBSDSRCDIR}/common/include -DSYSCTL_INCLUDE_DESCR
USE_FORT=	no
WARNS?=		3

.if !empty(IOCONF)
_BSD_IOCONF_MK_USER_=1
.include <bsd.ioconf.mk>
.endif

.include <bsd.own.mk>
