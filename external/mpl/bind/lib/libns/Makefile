#	$NetBSD: Makefile,v 1.1.2.2 2018/09/06 06:55:18 pgoyette Exp $

LIB=ns
#USE_SHLIBDIR=   yes

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/ns
.include "${DIST}/api"

.PATH.c:	${DIST}
CPPFLAGS+=	-I${BIND_SRCDIR}/include/ns -I${DIST}

CWARNFLAGS.clang+=	-Wno-tautological-constant-out-of-range-compare

#.for f in lookup byaddr request sdb validator
#COPTS.${f}.c+=  -Wno-pointer-sign -fno-strict-aliasing
#.endfor

SRCS=	client.c interfacemgr.c lib.c listenlist.c \
	log.c notify.c query.c server.c sortlist.c stats.c \
	update.c version.c xfrout.c

.include <bsd.lib.mk>
