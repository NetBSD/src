#	$NetBSD: Makefile,v 1.4 2001/01/09 08:52:59 enami Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/praliases

PROG=		praliases

LIBSMUTILDIR!=	cd ${.CURDIR}/../libsmutil && ${PRINTOBJDIR}
LIBSMDBDIR!=	cd ${.CURDIR}/../libsmdb && ${PRINTOBJDIR}

CPPFLAGS+=	-DNOT_SENDMAIL
LDADD+=		-L${LIBSMDBDIR} -lsmdb -L${LIBSMUTILDIR} -lsmutil
DPADD+=		${LIBSMDBDIR}/libsmdb.a ${LIBSMUTILDIR}/libsmutil.a

MAN=		praliases.8

.include <bsd.prog.mk>
