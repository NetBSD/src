#	$NetBSD: Makefile,v 1.6 2002/09/19 03:09:56 lukem Exp $

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/makemap

PROG=		makemap

LIBSMUTILDIR!=	cd ${.CURDIR}/../libsmutil && ${PRINTOBJDIR}
LIBSMDBDIR!=	cd ${.CURDIR}/../libsmdb && ${PRINTOBJDIR}

CPPFLAGS+=	-DNOT_SENDMAIL
LDADD+=		-L${LIBSMDBDIR} -lsmdb -L${LIBSMUTILDIR} -lsmutil
DPADD+=		${LIBSMDBDIR}/libsmdb.a ${LIBSMUTILDIR}/libsmutil.a

MAN=		makemap.8

.include <bsd.prog.mk>
