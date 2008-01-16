#	$NetBSD: Makefile.inc,v 1.1 2008/01/16 12:34:56 ad Exp $

S!=		cd ${.CURDIR}/../..;pwd
CPPFLAGS+=	-I${NETBSDSRCDIR}/common/include
USE_FORT=	no
WARNS?=		1

.include <bsd.own.mk>
