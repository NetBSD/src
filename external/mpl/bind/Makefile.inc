#	$NetBSD: Makefile.inc,v 1.16 2025/01/26 16:24:30 christos Exp $

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
LIBURCUDIR=	${NETBSDSRCDIR}/external/lgpl2/userspace-rcu
LIBURCUMEMBOBJDIR!=   cd ${LIBURCUDIR}/lib/liburcu-memb && ${PRINTOBJDIR}
LIBURCUCDSOBJDIR!=   cd ${LIBURCUDIR}/lib/liburcu-cds && ${PRINTOBJDIR}
LIBURCUCOMMONOBJDIR!=   cd ${LIBURCUDIR}/lib/liburcu-common && ${PRINTOBJDIR}
CPPFLAGS+=	-I${LIBUVDIR}/dist/include
CPPFLAGS+=	-I${LIBURCUDIR}/dist/include -I${LIBURCUDIR}/include
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
	-I${IDIST}/lib/dns/include \
	-I${IDIST}/lib/ns/include \
	-I${IDIST}/lib/isc/include \
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
LDADD+=		-lisccfg_g -ldns_g -lns_g
LDADD+=		-lisccc_g -lisc_g
LDADD+=		-L${LIBURCUMEMBOBJDIR} -lurcu-memb_g
LDADD+=		-L${LIBURCUCDSOBJDIR} -lurcu-cds_g
LDADD+=		-L${LIBURCUCOMMONOBJDIR} -lurcu-common_g
LDADD+=		-L${LIBUVOBJDIR} -luv_g
LDADD+=		-lexecinfo_g -lelf_g -lkvm_g -lz_g -lm_g
.else
LDADD+=		-lisccfg -ldns -lns
DPADD+=		${LIBISCCFG} ${LIBDNS} ${LIBNS}
LDADD+=		-lisccc -lisc 
LDADD+=		-L${LIBURCUMEMBOBJDIR} -lurcu-memb
LDADD+=		-L${LIBURCUCDSOBJDIR} -lurcu-cds
LDADD+=		-L${LIBURCUCOMMONOBJDIR} -lurcu-common
LDADD+=		-L${LIBUVOBJDIR} -luv
LDADD+=		-lexecinfo -lelf -lkvm -lz -lm
DPADD+=		${LIBISCCC} ${LIBISC}
DPADD+=		${LIBUVOBJDIR}/libuv.a
DPADD+=		${LIBEXECINFO} ${LIBKVM} ${LIBZ} ${LIBM}
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
CPPFLAGS+=-DWITH_KERBEROS
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
