# $NetBSD: Makefile,v 1.43.4.2 2020/04/13 07:46:00 martin Exp $
#

WARNS?=		6
USE_FORT?=	yes	# network client (local server)

.include <bsd.init.mk>
.include "../../Makefile.inc"

PROG=		dhcpcd
SRCS=		common.c control.c dhcpcd.c duid.c eloop.c logerr.c
SRCS+=		if.c if-options.c route.c sa.c script.c
SRCS+=		dhcp-common.c dhcpcd-embedded.c
SRCS+=		if-bsd.c

CPPFLAGS+=	-DPRIVSEP -DPRIVSEP_USER=\"_dhcpcd\"
SRCS+=		privsep.c privsep-root.c privsep-inet.c
SRCS+=		privsep-bsd.c

CPPFLAGS+=	-DHAVE_CONFIG_H -D_OPENBSD_SOURCE

USE_INET?=	yes

.if defined(SMALLPROG)
CPPFLAGS+=	-DSMALL
.else
CPPFLAGS+=	-DAUTH
SRCS+=		auth.c
.endif

.if (${USE_INET} != "no")
CPPFLAGS+=	-DINET
SRCS+=		bpf.c dhcp.c ipv4.c
SRCS+=		privsep-bpf.c
.if !defined(SMALLPROG)
CPPFLAGS+=	-DARP
SRCS+=		arp.c
CPPFLAGS+=	-DARPING
CPPFLAGS+=	-DIPV4LL
SRCS+=		ipv4ll.c
.endif
.endif

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
SRCS+=		ipv6.c ipv6nd.c
# -DSMALL already strips out prefix delegation
# A network could be IPv6 only and require DHCPv6
CPPFLAGS+=	-DDHCP6
SRCS+=		dhcp6.c
.endif

CPPFLAGS+=	-I${SRCDIR}/include -I${DIST}/src

DPADD+=		${LIBUTIL}
LDADD+=		-lutil

.PATH: ${DIST}/src ${DIST}/src/crypt ${LIBC_NET}

CONFIGFILES=			dhcpcd.conf
FILESDIR_dhcpcd.conf=		/etc
FILESMODE_dhcpcd.conf=		644	# -rw-r--r--

MAN=			dhcpcd.conf.5 dhcpcd.8 
CLEANFILES=		dhcpcd.conf.5 dhcpcd.8 

COPTS.logerr.c+=    ${${ACTIVE_CC} == "clang" && ${MACHINE_ARCH} == "powerpc":? -O0 :}


.for f in dhcpcd.conf.5 dhcpcd.8
${f}:	${f}.in
	${TOOL_SED} ${SED_SYS} ${DIST}/src/${f}.in > $@
.endfor

.include <bsd.prog.mk>
