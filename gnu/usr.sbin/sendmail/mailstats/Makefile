#	$NetBSD: Makefile,v 1.4 2003/03/24 15:04:07 atatat Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/mailstats

LIBSMUTILDIR!=	cd ${.CURDIR}/../libsmutil && ${PRINTOBJDIR}
LIBSMDIR!=	cd ${.CURDIR}/../libsm && ${PRINTOBJDIR}

LDADD+=		-L${LIBSMUTILDIR} -lsmutil -L${LIBSMDIR} -lsm
DPADD+=		${LIBSMUTILDIR}/libsmutil.a ${LIBSMDIR}/libsm.a

PROG=		mailstats
MAN=		mailstats.8

CPPFLAGS+=	-DNOT_SENDMAIL

.include <bsd.prog.mk>
