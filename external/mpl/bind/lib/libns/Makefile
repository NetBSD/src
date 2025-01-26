#	$NetBSD: Makefile,v 1.8 2025/01/26 16:25:53 christos Exp $

LIB=ns
#USE_SHLIBDIR=   yes

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/ns

.PATH.c:	${DIST}
CPPFLAGS+=	-I${BIND_SRCDIR}/include/ns -I${DIST}
CPPFLAGS+=	-DNAMED_PLUGINDIR=\"/usr/lib/named\"

CWARNFLAGS.clang+=	-Wno-tautological-constant-out-of-range-compare

#.for f in lookup byaddr request sdb validator
#COPTS.${f}.c+=  -Wno-pointer-sign -fno-strict-aliasing
#.endfor

SRCS=	client.c hooks.c interfacemgr.c listenlist.c \
	log.c notify.c query.c server.c sortlist.c stats.c \
	update.c xfrout.c pfilter.c

.include <bsd.lib.mk>
