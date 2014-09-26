# $NetBSD: Makefile.inc,v 1.12 2014/09/26 22:18:48 christos Exp $

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
DSTOBJDIR!=cd ${.PARSEDIR}/lib/dst && ${PRINTOBJDIR}

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
LDADD+= ${OMOBJDIR}/libomapi.a ${DSTOBJDIR}/libdst.a
.if defined(PROG) && ${PROG} == "dhclient"
LDADD+=-Wl,-Bstatic
.endif
LDADD+= -lirs -lisccfg -ldns -lisc
LDADD+=-lpthread
.if (${MKCRYPTO} != "no")
.if (${MKKERBEROS} != "no")
LDADD+= -lgssapi -lkrb5 -lhx509 -lheimntlm -lheimbase \
	-lcom_err  -lroken -lasn1 -lwind
DPADD+= ${LIBGSSAPI} ${LIBKRB5} ${LIBHX509} ${LIBHEIMNTLM} ${LIBHEIMBASE} \
	${LIBCOM_ERR} ${LIBROKEN} ${LIBASN1} ${LIBWIND}
.endif
.if defined(PROG) && ${PROG} == "dhclient"
LDADD+=-Wl,-Bdynamic
.endif
LDADD+= -lcrypto -lipsec -lcrypt
DPADD+= ${LIBCRYPTO} ${LIBIPSEC} ${LIBCRYPT}
.endif
DPADD+= ${COBJDIR}/libdhcp.a
DPADD+= ${OMOBJDIR}/libomapi.a ${DSTOBJDIR}/libdst.a
DPADD+=	${LIBDNS} ${LIBISC}
DPADD+= ${LIBPTHREAD}
