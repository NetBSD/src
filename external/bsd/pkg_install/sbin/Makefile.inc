# $NetBSD: Makefile.inc,v 1.3.28.1 2017/04/26 02:52:50 pgoyette Exp $

.include <bsd.own.mk>

.include "${.PARSEDIR}/../Makefile.inc"

LIBINSTALL != cd ${.PARSEDIR}/../lib && ${PRINTOBJDIR}

BINDIR?=	/usr/sbin
CPPFLAGS+=	-DBINDIR='"${BINDIR}"'

DPADD+=	${LIBINSTALL}/libinstall.a
LDADD+=	-L${LIBINSTALL} -linstall -ltermcap

DPADD+=		${LIBNETPGPVERIFY} ${LIBFETCH} ${LIBSSL} ${LIBCRYPTO}
LDADD+=		-lnetpgpverify -lfetch -lssl -lcrypto

DPADD+=		${LIBARCHIVE}
LDADD+=		-larchive

DPADD+=		${LIBZ} ${LIBBZ2}
LDADD+=		-lz -lbz2

DPADD+=		${LIBLZMA} ${LIBPTHREAD}
LDADD+=		-llzma -lpthread
