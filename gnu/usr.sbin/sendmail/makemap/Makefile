#	$NetBSD: Makefile,v 1.3 2000/05/03 10:50:59 itojun Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/makemap

PROG=		makemap

LIBSMUTILDIR!=	cd ${.CURDIR}/../libsmutil; make print-objdir
LIBSMDBDIR!=	cd ${.CURDIR}/../libsmdb; make print-objdir

CPPFLAGS+=	-DNOT_SENDMAIL
LDADD+=		-L${LIBSMDBDIR} -lsmdb -L${LIBSMUTILDIR} -lsmutil
DPADD+=		${LIBSMDBDIR}/libsmdb.a ${LIBSMUTILDIR}/libsmutil.a

MAN=		makemap.8

.include <bsd.prog.mk>
