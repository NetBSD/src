# $NetBSD: Makefile,v 1.21.2.1 2014/08/10 07:06:59 tls Exp $
#

PROG=		dhcpcd
SRCS=		common.c control.c dhcpcd.c duid.c eloop.c
SRCS+=		if.c if-options.c script.c
SRCS+=		dhcp-common.c dhcpcd-embedded.c
SRCS+=		if-bsd.c

WARNS?=		5
USE_FORT?=	yes	# network client (local server)

.include <bsd.own.mk>

SRCS+=		auth.c hmac_md5.c

CPPFLAGS+=	-DINET
SRCS+=		arp.c dhcp.c ipv4.c ipv4ll.c

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
SRCS+=		ipv6.c ipv6nd.c dhcp6.c
.endif

DIST=		${NETBSDSRCDIR}/external/bsd/dhcpcd/dist
CPPFLAGS+=	-I${DIST}

.PATH: ${DIST} ${DIST}/crypt ${LIBC_NET}

SCRIPTS=			dhcpcd-run-hooks
SCRIPTSDIR_dhcpcd-run-hooks=	/libexec

CONFIGFILES=			dhcpcd.conf
FILESDIR_dhcpcd.conf=		/etc

HOOKS=			01-test 02-dump 10-mtu 10-wpa_supplicant 15-timezone
HOOKS+=			20-resolv.conf 29-lookup-hostname 30-hostname
HOOKS+=			50-ntp.conf

FILES=			${HOOKS:C,^,${DIST}/dhcpcd-hooks/,}
FILESDIR=		/libexec/dhcpcd-hooks

MAN=			dhcpcd.conf.5 dhcpcd.8 dhcpcd-run-hooks.8

CLEANFILES=		dhcpcd.conf.5 dhcpcd.8 \
			dhcpcd-run-hooks dhcpcd-run-hooks.8

.for f in dhcpcd-run-hooks dhcpcd.conf.5 dhcpcd.8 dhcpcd-run-hooks.8
${f}:	${f}.in
	${TOOL_SED} -e 's:@SYSCONFDIR@:/etc:g' -e 's:@DBDIR@:/var/db:g' \
	    -e 's:@LIBDIR@:/lib:g' \
	    -e 's:@RUNDIR@:/var/run:g' \
	    -e 's:@HOOKDIR@:/libexec/dhcpcd-hooks:g' \
	    -e 's:@SCRIPT@:/libexec/dhcpcd-run-hooks:g' \
	    -e 's:@SERVICEEXISTS@::g' \
	    -e 's:@SERVICECMD@::g' \
	    -e 's:@SERVICESTATUS@::g' \
	    ${DIST}/${f}.in > $@
.endfor

.include <bsd.prog.mk>
