#	$NetBSD: Makefile.inc,v 1.1.6.2 2008/02/18 21:07:00 mjf Exp $

S!=		cd ${.CURDIR}/../..;pwd
CPPFLAGS+=	-I${NETBSDSRCDIR}/common/include
USE_FORT=	no
WARNS?=		1

.include <bsd.own.mk>
