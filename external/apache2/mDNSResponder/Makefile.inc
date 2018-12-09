# $NetBSD: Makefile.inc,v 1.3 2018/12/09 19:03:42 christos Exp $

.include <bsd.own.mk>

DIST:=	${.PARSEDIR}/dist

# IPv6 support.
.if ${MKINET6} == "no"
CPPFLAGS+=      -DHAVE_IPV6=0
.else
CPPFLAGS+=      -DHAVE_IPV6=1
.endif

CPPFLAGS+=	-DPID_FILE=\"/var/run/mdnsd/mdnsd.pid\" \
		-DMDNS_UDS_SERVERPATH=\"/var/run/mdnsd/mdnsd\" \
		-DMDNS_VERSIONSTR_NODTS
