# $NetBSD: Makefile,v 1.5 2001/01/10 21:30:05 phil Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/kadm5

WARNS?=  1
MKLINT= no

all: depend

depend: kadm5_err.c kadm5_err.h

COMPILEET!=	cd ${.CURDIR}/../libcom_err/compile_et && ${PRINTOBJDIR}

kadm5_err.c kadm5_err.h: kadm5_err.et
	${COMPILEET}/compile_et ${DIST}/heimdal/lib/kadm5/kadm5_err.et

clean cleandir:
	rm -f kadm5_err.c kadm5_err.h

cleanhtml:

INCSDIR= /usr/include/krb5

INCS=	kadm5_err.h

.include <bsd.own.mk>
.include <bsd.inc.mk>
.include <bsd.dep.mk>
.include <bsd.depall.mk>
.include <bsd.obj.mk>
