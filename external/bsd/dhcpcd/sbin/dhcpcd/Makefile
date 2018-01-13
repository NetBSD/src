# $NetBSD: Makefile,v 1.40.4.1 2018/01/13 21:35:30 snj Exp $
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

# Need to generate this for dependencies
DPSRCS+=	dhcpcd-embedded.h dhcpcd-embedded.c

CPPFLAGS+=	-DHAVE_CONFIG_H -D_OPENBSD_SOURCE

USE_INET?=	yes

.if defined(SMALLPROG)
CPPFLAGS+=	-DSMALL
DHCPCD_DEFS=	dhcpcd-definitions-small.conf
.else
DHCPCD_DEFS=	dhcpcd-definitions.conf
CPPFLAGS+=	-DAUTH
SRCS+=		auth.c
.endif

.if (${USE_INET} != "no")
CPPFLAGS+=	-DARP -DINET
SRCS+=		arp.c bpf.c dhcp.c ipv4.c
.if !defined(SMALLPROG)
CPPFLAGS+=	-DIPV4LL -DARPING
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

CPPFLAGS+=	-I${SRCDIR}/include -I${DIST}/src -I${.OBJDIR}

DPADD+=		${LIBUTIL}
LDADD+=		-lutil

.PATH: ${DIST}/src ${DIST}/src/crypt ${LIBC_NET}

CONFIGFILES=			dhcpcd.conf
FILESDIR_dhcpcd.conf=		/etc
FILESMODE_dhcpcd.conf=		644	# -rw-r--r--

MAN=			dhcpcd.conf.5 dhcpcd.8 
CLEANFILES=		dhcpcd.conf.5 dhcpcd.8 

dhcpcd-embedded.h:	genembedh ${DHCPCD_DEFS} dhcpcd-embedded.h.in
	${HOST_SH} ${.ALLSRC} $^ > $@

dhcpcd-embedded.c:	genembedc ${DHCPCD_DEFS} dhcpcd-embedded.c.in
	${HOST_SH} ${.ALLSRC} $^ > $@

CLEANFILES+=		dhcpcd-embedded.c dhcpcd-embedded.h

.for f in dhcpcd.conf.5 dhcpcd.8
${f}:	${f}.in
	${TOOL_SED} ${SED_SYS} ${DIST}/src/${f}.in > $@
.endfor

# XXXGCC5
.if defined(HAVE_GCC) && ${HAVE_GCC} == 53 && ${ACTIVE_CC} == "gcc"
COPTS.if-bsd.c+=  -Wno-error=sign-conversion
.endif

.include <bsd.prog.mk>
