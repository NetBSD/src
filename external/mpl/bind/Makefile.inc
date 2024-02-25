#	$NetBSD: Makefile.inc,v 1.12.2.1 2024/02/25 15:42:51 martin Exp $

.if !defined(BIND9_MAKEFILE_INC)
BIND9_MAKEFILE_INC=yes

#NAMED_DEBUG=1

USE_FORT?= yes	# network client/server

WARNS?=	1

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-parentheses \
			-Wno-atomic-alignment

.include <bsd.own.mk>

LIBUVDIR=	${NETBSDSRCDIR}/external/mit/libuv
LIBUVOBJDIR!=   cd ${LIBUVDIR}/lib && ${PRINTOBJDIR}
CPPFLAGS+=	-I${LIBUVDIR}/dist/include
CFLAGS+=	-std=gnu18
LINTFLAGS+=	-Ac11

BIND_SRCDIR:=	${.PARSEDIR}
IDIST=		${BIND_SRCDIR}/dist
BIND_HTMLDIR=	/usr/share/doc/reference/ref8/bind9

SYSCONFDIR=/etc
LOCALSTATEDIR=/var

LINTFLAGS+=-Z-include -Z${BIND_SRCDIR}/include/config.h
CPPFLAGS+=-include ${BIND_SRCDIR}/include/config.h
CPPFLAGS+=-I${BIND_SRCDIR}/include \
	-I${IDIST} \
	-I${IDIST}/lib/bind9/include \
	-I${IDIST}/lib/dns/include \
	-I${IDIST}/lib/ns/include \
	-I${IDIST}/lib/irs/include \
	-I${IDIST}/lib/isc/include -I${IDIST}/lib/isc/unix/include \
	-I${IDIST}/lib/isccfg/include \
	-I${IDIST}/lib/isccc/include \
	-DSESSION_KEYFILE=\"${LOCALSTATEDIR}/run/named/session.key\" \
	-DNAMED_CONFFILE=\"${SYSCONFDIR}/named.conf\"

.if defined(HAVE_GCC)
COPTS+=	-Wno-pointer-sign
.endif

.if defined(NAMED_DEBUG)
DBG=-g3 -gstabs
.endif

.if !defined(LIB) || empty(LIB)
# NOTE: the order of these libraries is important...
.if defined(NAMED_DEBUG)
LDADD+=		-lbind9_g -lisccfg_g -ldns_g -lns_g -lirs_g
LDADD+=		-lisccc_g -lisc_g
LDADD+=		-L${LIBUVOBJDIR} -luv_g
LDADD+=		-lexecinfo_g -lkvm_g -lz_g
.else
LDADD+=		-lbind9 -lisccfg -ldns -lns -lirs
DPADD+=		${LIBBIND9} ${LIBISCCFG} ${LIBDNS} ${LIBNS} ${LIBIRS}
LDADD+=		-lisccc -lisc 
LDADD+=		-L${LIBUVOBJDIR} -luv
LDADD+=		-lexecinfo -lkvm -lz
DPADD+=		${LIBISCCC} ${LIBISC}
DPADD+=		${LIBUVOBJDIR}/libuv.a
DPADD+=		${LIBEXECINFO} ${LIBKVM} ${LIBZ}
.endif
.endif

LDADD+=-lblocklist
DPADD+=${LIBBLOCKLIST}

.if !defined (LIB) || empty(LIB)
LDADD+= -lpthread
DPADD+= ${LIBPTHREAD}
.else
LIBDPLIBS+=      pthread  ${NETBSDSRCDIR}/lib/libpthread
.endif

.if ${MKKERBEROS} != "no"
.if !defined (LIB) || empty(LIB)
LDADD+= -lgssapi -lheimntlm ${LIBKRB5_LDADD}
DPADD+= ${LIBGSSAPI} ${LIBHEIMNTLM} ${LIBKRB5_DPADD}
.else
.for L in gssapi krb5 hx509 heimntlm heimbase com_err roken asn1 wind
LIBDPLIBS+=	$L ${NETBSDSRCDIR}/crypto/external/bsd/heimdal/lib/lib$L
.endfor
.endif
.endif

.if !defined (LIB) || empty(LIB)
LDADD+= -lssl -lcrypto -lcrypt
DPADD+= ${LIBSSL} ${LIBCRYPTO} ${LIBCRYPT}
.else
LIBDPLIBS+=	crypto ${NETBSDSRCDIR}/crypto/external/bsd/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto
.endif
.endif

CPPFLAGS+=-I${IDIST}/lib/isc/pthreads/include

.if exists(${.PARSEDIR}/../Makefile.inc)
.include "${.PARSEDIR}/../Makefile.inc"
.endif

COPTS+=	-fcommon
