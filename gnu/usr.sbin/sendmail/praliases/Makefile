#	$NetBSD: Makefile,v 1.2.2.2 2001/03/09 18:03:55 he Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/praliases

PROG=		praliases

LIBSMUTILDIR!=	cd ${.CURDIR}/../libsmutil; ${MAKE} print-objdir
LIBSMDBDIR!=	cd ${.CURDIR}/../libsmdb; ${MAKE} print-objdir

CPPFLAGS+=	-DNOT_SENDMAIL
LDADD+=		-L${LIBSMDBDIR} -lsmdb -L${LIBSMUTILDIR} -lsmutil
DPADD+=		${LIBSMDBDIR}/libsmdb.a ${LIBSMUTILDIR}/libsmutil.a

MAN=		praliases.8

.include <bsd.prog.mk>
