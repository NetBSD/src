#	$NetBSD: Makefile,v 1.7 2002/09/19 03:09:56 lukem Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
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
