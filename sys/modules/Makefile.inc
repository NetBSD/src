#	$NetBSD: Makefile.inc,v 1.2 2010/03/13 22:06:30 christos Exp $

S!=		cd ${.PARSEDIR}/..;pwd
CPPFLAGS+=	-I${NETBSDSRCDIR}/common/include
USE_FORT=	no
WARNS?=		1

.include <bsd.own.mk>
