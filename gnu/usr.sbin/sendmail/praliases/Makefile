#	$NetBSD: Makefile,v 1.2 2000/05/03 10:51:00 itojun Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/praliases

PROG=		praliases

LIBSMUTILDIR!=	cd ${.CURDIR}/../libsmutil; make print-objdir
LIBSMDBDIR!=	cd ${.CURDIR}/../libsmdb; make print-objdir

CPPFLAGS+=	-DNOT_SENDMAIL
LDADD+=		-L${LIBSMDBDIR} -lsmdb -L${LIBSMUTILDIR} -lsmutil
DPADD+=		${LIBSMDBDIR}/libsmdb.a ${LIBSMUTILDIR}/libsmutil.a

MAN=		praliases.8

.include <bsd.prog.mk>
