#	$NetBSD: Makefile,v 1.8 2003/03/24 15:04:08 atatat Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/smrsh

BINDIR=		/usr/libexec
PROG=		smrsh

LIBSMUTILDIR!=	cd ${.CURDIR}/../libsmutil && ${PRINTOBJDIR}
LIBSMDIR!=	cd ${.CURDIR}/../libsm && ${PRINTOBJDIR}

CPPFLAGS+=	-DNOT_SENDMAIL
CPPFLAGS+=	-DCMDDIR=\"/usr/libexec/sm.bin\"
LDADD+=		-L${LIBSMUTILDIR} -lsmutil -L${LIBSMDIR} -lsm
DPADD+=		${LIBSMUTILDIR}/libsmutil.a ${LIBSMDIR}/libsm.a

MAN=		smrsh.8

.include <bsd.prog.mk>
