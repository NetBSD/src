#	$NetBSD: Makefile,v 1.4 2000/05/03 10:51:00 itojun Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/smrsh

BINDIR=		/usr/libexec
PROG=		smrsh

LIBSMUTILDIR!=	cd ${.CURDIR}/../libsmutil; make print-objdir
LIBSMDBDIR!=	cd ${.CURDIR}/../libsmdb; make print-objdir

CPPFLAGS+=	-DNOT_SENDMAIL
CPPFLAGS+=	-DCMDDIR=\"/usr/libexec/sm.bin\"
LDADD+=		-L${LIBSMUTILDIR} -lsmutil
DPADD+=		${LIBSMUTILDIR}/libsmutil.a

MAN=		smrsh.8

.include <bsd.prog.mk>
