#	$NetBSD: Makefile.inc,v 1.1.14.2 2008/03/23 02:05:04 matt Exp $

S!=		cd ${.CURDIR}/../..;pwd
CPPFLAGS+=	-I${NETBSDSRCDIR}/common/include
USE_FORT=	no
WARNS?=		1

.include <bsd.own.mk>
