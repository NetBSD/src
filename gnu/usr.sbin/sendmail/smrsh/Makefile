#	$NetBSD: Makefile,v 1.6 2001/01/09 08:52:59 enami Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../dist
.PATH: ${DIST}/sendmail/smrsh

BINDIR=		/usr/libexec
PROG=		smrsh

LIBSMUTILDIR!=	cd ${.CURDIR}/../libsmutil && ${PRINTOBJDIR}
LIBSMDBDIR!=	cd ${.CURDIR}/../libsmdb && ${PRINTOBJDIR}

CPPFLAGS+=	-DNOT_SENDMAIL
CPPFLAGS+=	-DCMDDIR=\"/usr/libexec/sm.bin\"
LDADD+=		-L${LIBSMUTILDIR} -lsmutil
DPADD+=		${LIBSMUTILDIR}/libsmutil.a

MAN=		smrsh.8

.include <bsd.prog.mk>
