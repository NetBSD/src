#	$NetBSD: Makefile.inc,v 1.1.2.2 2008/01/19 12:15:29 bouyer Exp $

S!=		cd ${.CURDIR}/../..;pwd
CPPFLAGS+=	-I${NETBSDSRCDIR}/common/include
USE_FORT=	no
WARNS?=		1

.include <bsd.own.mk>
