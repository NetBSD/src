# $NetBSD: Makefile.inc,v 1.1 2008/09/30 19:19:56 joerg Exp $

.include <bsd.own.mk>

.include "${.PARSEDIR}/../Makefile.inc"

LIBINSTALL != cd ${.PARSEDIR}/../lib && ${PRINTOBJDIR}

BINDIR?=	/usr/sbin
CPPFLAGS+=	-DBINDIR='"${BINDIR}"'

DPADD+=	${LIBINSTALL}/libinstall.a
LDADD+=	-L${LIBINSTALL} -linstall -ltermcap

.if defined(LINK_PKGVUL) || defined(LINK_ARCHIVE)
DPADD+=		${LIBFETCH} ${LIBSSL} ${LIBCRYPTO}
LDADD+=		-lfetch -lssl -lcrypto
.endif

.if defined(LINK_ARCHIVE)
DPADD+=		${LIBARCHIVE}
LDADD+=		-larchive
.endif

.if defined(LINK_PKGVUL) || defined(LINK_ARCHIVE)
DPADD+=		${LIBZ} ${LIBBZ2}
LDADD+=		-lz -lbz2
.endif
