#	$NetBSD: Makefile,v 1.3 2000/07/07 04:26:58 cgd Exp $

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
