# $NetBSD: Makefile.inc,v 1.19 2018/02/25 00:16:48 mrg Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare

.include <bsd.own.mk>

USE_FORT?= yes	# network client/server
CWARNFLAGS.clang+=	-Wno-tautological-compare \
			-Wno-conversion -Wno-constant-logical-operand \
			-Wno-format-security -Wno-error=unused-const-variable

DIST:=	${.PARSEDIR}/dist
BIND:=	${.PARSEDIR}/../bind/dist
.PATH:	${DIST}/${DHCPSRCDIR}

COBJDIR!=cd ${.PARSEDIR}/lib/common && ${PRINTOBJDIR}
OMOBJDIR!=cd ${.PARSEDIR}/lib/omapip && ${PRINTOBJDIR}

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DDHCPv6
.endif
CPPFLAGS+= -DISC_PLATFORM_USETHREADS -DHAVE_CONFIG_H
CPPFLAGS+= -I${DIST} -I${DIST}/includes -I${DIST}/../include
CPPFLAGS+= -I${BIND} -I${BIND}/includes -I${BIND}/../include
.for dir in isc dns irs
CPPFLAGS+= -I${BIND}/lib/${dir}/include
CPPFLAGS+= -I${BIND}/lib/${dir}/unix/include
CPPFLAGS+= -I${BIND}/lib/${dir}/pthreads/include
.endfor
CPPFLAGS+= -DLOCALSTATEDIR='"/var"'
LDADD+= ${COBJDIR}/libdhcp.a
LDADD+= ${OMOBJDIR}/libomapi.a
.if defined(PROG) && ${PROG} == "dhclient"
LDADD+=-Wl,-Bstatic
.endif
LDADD+= -lirs -lisccfg -ldns -lisc
LDADD+=-lpthread
.if (${MKKERBEROS} != "no")
LDADD+= -lgssapi -lheimntlm ${LIBKRB5_LDADD} ${LIBKRB5_STATIC_LDADD}
DPADD+= ${LIBKRB5_DPADD} ${LIBHEIMNTLM} ${LIBGSSAPI} ${LIBKRB5_STATIC_DPADD}
.endif
.if defined(PROG) && ${PROG} == "dhclient"
LDADD+=-Wl,-Bdynamic
.endif
LDADD+= -lcrypto -lipsec -lcrypt
DPADD+= ${LIBCRYPTO} ${LIBIPSEC} ${LIBCRYPT}
.if defined(PROG) && ${PROG} == "dhclient"
LDADD+=-Wl,-Bdynamic
.endif
DPADD+= ${COBJDIR}/libdhcp.a
DPADD+= ${OMOBJDIR}/libomapi.a
DPADD+=	${LIBDNS} ${LIBISC}
DPADD+= ${LIBPTHREAD}
