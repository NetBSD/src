#	$NetBSD: Makefile,v 1.5 2001/01/09 08:52:59 enami Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/makemap

PROG=		makemap

LIBSMUTILDIR!=	cd ${.CURDIR}/../libsmutil && ${PRINTOBJDIR}
LIBSMDBDIR!=	cd ${.CURDIR}/../libsmdb && ${PRINTOBJDIR}

CPPFLAGS+=	-DNOT_SENDMAIL
LDADD+=		-L${LIBSMDBDIR} -lsmdb -L${LIBSMUTILDIR} -lsmutil
DPADD+=		${LIBSMDBDIR}/libsmdb.a ${LIBSMUTILDIR}/libsmutil.a

MAN=		makemap.8

.include <bsd.prog.mk>
