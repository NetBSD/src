# $NetBSD: Makefile.inc,v 1.2.46.1 2018/12/26 14:01:14 pgoyette Exp $

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
