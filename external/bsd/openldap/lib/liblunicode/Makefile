#	$NetBSD: Makefile,v 1.1 2021/12/28 17:04:18 christos Exp $

USE_FORT?=	yes

LIBISPRIVATE=	yes

.include "../../openldap.mk"

LIB=		lunicode

LIBLUNICODE=${LDAP_DISTDIR}/libraries/liblunicode
.PATH:		${LIBLUNICODE} ${LIBLUNICODE}/ucdata ${LIBLUNICODE}/ure
CPPFLAGS+=	-I${LIBLUNICODE} -I${LIBLUNICODE}/ucdata -I.

SRCS+= ucdata.c ure.c urestubs.c ucstr.c


.if ${USETOOLS} != "yes"
UCGENDATOBJ!=     cd ${.CURDIR}/ucgendat && ${PRINTOBJDIR}
TOOL_UCGENDAT=   ${UCGENDATDIR}/ucgendat

.PHONY:         ucgendat
includes:       ucgendat
SUBDIR=         ucgendat

ucgendat:
	@cd ${.CURDIR}/ucgendat && ${MAKE}
.endif

uctable.h: ${TOOL_UCGENDAT} ${LIBLUNICODE}/UnicodeData.txt ${LIBLUNICODE}/CompositionExclusions.txt
	${_MKTARGET_CREATE}
	${TOOL_UCGENDAT} ${LIBLUNICODE}/UnicodeData.txt \
	    -x ${LIBLUNICODE}/CompositionExclusions.txt

ucdata.c: uctable.h
.endif


.include <bsd.lib.mk>
.if ${USETOOLS} != "yes"
.include <bsd.subdir.mk>
.endif
