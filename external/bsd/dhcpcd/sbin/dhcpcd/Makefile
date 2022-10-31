# $NetBSD: Makefile,v 1.57 2022/10/31 09:27:23 wiz Exp $
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

# Disable privilege separation and sandboxing
# This should be fine for ramdisk based installers
.if !defined(SMALLPROG)
CPPFLAGS+=	-DPRIVSEP -DPRIVSEP_USER=\"_dhcpcd\"
SRCS+=		privsep.c privsep-root.c privsep-inet.c privsep-control.c
SRCS+=		privsep-bsd.c
.endif

CPPFLAGS+=	-DHAVE_CONFIG_H

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
.if !defined(SMALLPROG)
SRCS+=		privsep-bpf.c
.endif

.if (${MKLIBCSANITIZER:Uno} == "yes")
.if (${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} == 8)
COPTS.dhcp.c+=	-Wno-error=sign-conversion
.endif
.endif

.if !defined(SMALLPROG)
CPPFLAGS+=	-DARP
SRCS+=		arp.c
CPPFLAGS+=	-DARPING
CPPFLAGS+=	-DIPV4LL
SRCS+=		ipv4ll.c
.endif
.endif

.if (!defined(SMALLPROG) || defined(SMALLPROG_INET6)) && (${USE_INET6} != "no")
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

COPTS.logerr.c+=    ${${ACTIVE_CC} == "clang" && ${MACHINE_ARCH} == "powerpc":? -O0 :}

.include <bsd.prog.mk>
