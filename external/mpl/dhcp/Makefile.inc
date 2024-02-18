# $NetBSD: Makefile.inc,v 1.12 2024/02/18 20:57:30 christos Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare

.include <bsd.own.mk>

USE_FORT?= yes	# network client/server
CWARNFLAGS.clang+=	-Wno-tautological-compare \
			-Wno-conversion -Wno-constant-logical-operand \
			-Wno-format-security -Wno-error=unused-const-variable

DIST:=	${.PARSEDIR}/dist
BIND:=	${.PARSEDIR}/bind
BINDDIST=	${BIND}/dist
.PATH:	${DIST}/${DHCPSRCDIR}

COBJDIR!=cd ${.PARSEDIR}/lib/common && ${PRINTOBJDIR}
OMOBJDIR!=cd ${.PARSEDIR}/lib/omapip && ${PRINTOBJDIR}

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DDHCPv6
.endif
CPPFLAGS+= -DISC_PLATFORM_USETHREADS -DHAVE_CONFIG_H
CPPFLAGS+= -I${DIST} -I${DIST}/includes -I${DIST}/../include
CPPFLAGS+= -I${BINDDIST} -I${BINDDIST}/includes -I${BINDDIST}/../include
.for dir in isc dns irs
CPPFLAGS+= -I${BINDDIST}/lib/${dir}/include
CPPFLAGS+= -I${BINDDIST}/lib/${dir}/unix/include
CPPFLAGS+= -I${BINDDIST}/lib/${dir}/pthreads/include
.endfor
CPPFLAGS+= -DLOCALSTATEDIR='"/var"'
LDADD+= ${COBJDIR}/libdhcp.a
LDADD+= ${OMOBJDIR}/libomapi.a
.if defined(PROG) && ${PROG} == "dhclient" && ${MKSANITIZER:Uno} != "yes"
# We link the libraries that are in /usr/lib and not in /lib statically
# because dhclient is in /sbin and is needed before /usr is mounted in
# some configurations with a split / and /usr. Another way to fix this,
# would be to install the libraries in /lib...
LDADD+=-Wl,-Bstatic
.endif
PROGDPLIBS+= irs ${BIND}/lib/libirs
PROGDPLIBS+= isccfg ${BIND}/lib/libisccfg
PROGDPLIBS+= dns ${BIND}/lib/libdns
PROGDPLIBS+= isc ${BIND}/lib/libisc
PROGDPLIBS+= uv ${NETBSDSRCDIR}/external/mit/libuv/lib
LDADD+= -lexecinfo -lkvm -lpthread -lz
DPADD+= ${LIBEXECINFO} ${LIBKVM} ${LIBPTHREAD} ${LIBZ}
.if (${MKKERBEROS} != "no")
LDADD+= -lgssapi -lheimntlm ${LIBKRB5_LDADD} ${LIBKRB5_STATIC_LDADD}
DPADD+= ${LIBKRB5_DPADD} ${LIBHEIMNTLM} ${LIBGSSAPI} ${LIBKRB5_STATIC_DPADD}
.endif
.if defined(PROG) && ${PROG} == "dhclient" && ${MKSANITIZER:Uno} != "yes"
LDADD+=-Wl,-Bdynamic
.endif
LDADD+= -lssl -lcrypto -lipsec -lcrypt
DPADD+= ${LIBSSL} ${LIBCRYPTO} ${LIBIPSEC} ${LIBCRYPT}
.if defined(PROG) && ${PROG} == "dhclient" && ${MKSANITIZER:Uno} != "yes"
LDADD+=-Wl,-Bdynamic
.endif
DPADD+= ${COBJDIR}/libdhcp.a
DPADD+= ${OMOBJDIR}/libomapi.a

COPTS+=	-fcommon
