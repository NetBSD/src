#	$NetBSD: Makefile,v 1.2 2021/12/28 17:05:23 christos Exp $

USE_FORT?=	yes

LIBISPRIVATE=	yes

.include "../../openldap.mk"

LIB=		lunicode

LIBLUNICODE=${LDAP_DISTDIR}/libraries/liblunicode
.PATH:		${LIBLUNICODE} ${LIBLUNICODE}/ucdata ${LIBLUNICODE}/ure
CPPFLAGS+=	-I${LIBLUNICODE} -I${LIBLUNICODE}/ucdata -I.

SRCS+= ucdata.c ure.c urestubs.c ucstr.c

.include <bsd.lib.mk>
