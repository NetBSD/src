#	$NetBSD: Makefile.inc,v 1.1.4.2 2008/01/21 09:46:58 yamt Exp $

S!=		cd ${.CURDIR}/../..;pwd
CPPFLAGS+=	-I${NETBSDSRCDIR}/common/include
USE_FORT=	no
WARNS?=		1

.include <bsd.own.mk>
