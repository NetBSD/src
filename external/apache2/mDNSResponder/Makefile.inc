# $NetBSD: Makefile.inc,v 1.2.48.1 2019/06/10 21:42:45 christos Exp $

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
