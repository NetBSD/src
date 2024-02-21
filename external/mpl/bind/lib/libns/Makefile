#	$NetBSD: Makefile,v 1.6 2024/02/21 22:52:54 christos Exp $

NOLINT=yes	# XXX
LIB=ns
#USE_SHLIBDIR=   yes

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/ns

.PATH.c:	${DIST}
CPPFLAGS+=	-I${BIND_SRCDIR}/include/ns -I${DIST}
CPPFLAGS+=	-DNAMED_PLUGINDIR=\"/usr/libexec/named\"

CWARNFLAGS.clang+=	-Wno-tautological-constant-out-of-range-compare

#.for f in lookup byaddr request sdb validator
#COPTS.${f}.c+=  -Wno-pointer-sign -fno-strict-aliasing
#.endfor

SRCS=	client.c hooks.c interfacemgr.c listenlist.c \
	log.c notify.c query.c server.c sortlist.c stats.c \
	update.c xfrout.c pfilter.c

.include <bsd.lib.mk>
