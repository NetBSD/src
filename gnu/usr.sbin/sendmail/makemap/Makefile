#	$NetBSD: Makefile,v 1.3.2.2 2001/03/09 18:03:03 he Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/makemap

PROG=		makemap

LIBSMUTILDIR!=	cd ${.CURDIR}/../libsmutil; ${MAKE} print-objdir
LIBSMDBDIR!=	cd ${.CURDIR}/../libsmdb; ${MAKE} print-objdir

CPPFLAGS+=	-DNOT_SENDMAIL
LDADD+=		-L${LIBSMDBDIR} -lsmdb -L${LIBSMUTILDIR} -lsmutil
DPADD+=		${LIBSMDBDIR}/libsmdb.a ${LIBSMUTILDIR}/libsmutil.a

MAN=		makemap.8

.include <bsd.prog.mk>
