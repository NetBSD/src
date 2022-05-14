#	$NetBSD: Makefile,v 1.8 2022/05/14 11:43:25 rillig Exp $

LIB=dns
#USE_SHLIBDIR=   yes

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/dns
.include "${DIST}/mapapi"

LIBDPLIBS+=	isc	${.CURDIR}/../libisc

.PATH.c:	${DIST}/unix ${DIST}/sec/dst ${DIST}
CPPFLAGS+=	-I${BIND_SRCDIR}/include/dns -I${DIST}
CPPFLAGS+=	-I${IDIST}/lib/isc
CPPFLAGS+=	-DMAPAPI=\"${MAPAPI}\" -DMAJOR=\"${MAJORVER}.${MINORVER}\"

CWARNFLAGS.clang+=	-Wno-tautological-constant-out-of-range-compare

.for f in lookup byaddr request sdb validator
COPTS.${f}.c+=  -Wno-pointer-sign -fno-strict-aliasing
.endfor

DNSSEC_SRCS=	dst_api.c dst_parse.c dst_result.c \
	gssapictx.c hmac_link.c key.c openssl_link.c \
	openssldh_link.c opensslrsa_link.c \
	opensslecdsa_link.c openssleddsa_link.c
.if ${MKKERBEROS} != "no"
DNSSEC_SRCS+=	gssapi_link.c

CWARNFLAGS.clang+=	-Wno-error=tautological-constant-compare
.endif

# util.h, ISC_REQUIRE
LINTFLAGS+=	-X 129	# expression has null effect
# opensslecdsa_link.c, DST_RET does not conform to macro conventions.
LINTFLAGS+=	-X 193	# statement not reached
# Most casts are to isc__magic_t and thus look intentional.
LINTFLAGS+=	-X 247	# pointer cast from '%s' to '%s' may be troublesome
# 'uint32_tobuffer(token.value.as_ulong' looks obviously wrong but is used in
# a lot of places.
LINTFLAGS+=	-X 132	# conversion from '%s' to '%s' may lose accuracy
LINTFLAGS+=	-X 298	# conversion from '%s' to '%s' may lose accuracy, arg #%d

SRCS=	acl.c adb.c badcache.c byaddr.c cache.c callbacks.c catz.c \
	clientinfo.c compress.c client.c dyndb.c tsec.c ecdb.c ecs.c \
	db.c dbiterator.c dbtable.c diff.c dispatch.c dlz.c dns64.c dnsrps.c \
	dnssec.c \
	ds.c fixedname.c forward.c ipkeylist.c iptable.c journal.c \
	kasp.c keydata.c keymgr.c keytable.c lib.c log.c \
	lookup.c master.c masterdump.c message.c name.c ncache.c nsec.c \
	nsec3.c nta.c order.c peer.c portlist.c rbt.c rbtdb.c \
	rcode.c rdata.c rdatalist.c rdataset.c rdatasetiter.c rdataslab.c \
	request.c resolver.c result.c rootns.c rpz.c rriterator.c rrl.c sdb.c \
	soa.c ssu.c ssu_external.c stats.c tcpmsg.c time.c timer.c tkey.c \
	tsig.c ttl.c private.c validator.c version.c view.c xfrin.c zone.c \
	zonekey.c zoneverify.c zt.c sdlz.c update.c ${DNSSEC_SRCS}

.include <bsd.lib.mk>
