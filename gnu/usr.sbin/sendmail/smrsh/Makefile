#	$NetBSD: Makefile,v 1.4.2.1 2000/08/13 16:48:58 itojun Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/smrsh

BINDIR=		/usr/libexec
PROG=		smrsh

LIBSMUTILDIR!=	cd ${.CURDIR}/../libsmutil; ${MAKE} print-objdir
LIBSMDBDIR!=	cd ${.CURDIR}/../libsmdb; ${MAKE} print-objdir

CPPFLAGS+=	-DNOT_SENDMAIL
CPPFLAGS+=	-DCMDDIR=\"/usr/libexec/sm.bin\"
LDADD+=		-L${LIBSMUTILDIR} -lsmutil
DPADD+=		${LIBSMUTILDIR}/libsmutil.a

MAN=		smrsh.8

.include <bsd.prog.mk>
