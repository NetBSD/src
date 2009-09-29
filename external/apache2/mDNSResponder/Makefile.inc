# $NetBSD: Makefile.inc,v 1.1 2009/09/29 23:56:27 tsarna Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/external/apache2/mDNSResponder/dist

# IPv6 support.
.if ${MKINET6} == "no"
CPPFLAGS+=      -DHAVE_IPV6=0
.else
CPPFLAGS+=      -DHAVE_IPV6=1
.endif

CPPFLAGS+=	-DPID_FILE=\"/var/run/mdnsd/mdnsd.pid\" \
		-DMDNS_UDS_SERVERPATH=\"/var/run/mdnsd/mdnsd\"
