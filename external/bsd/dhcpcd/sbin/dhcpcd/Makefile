# $NetBSD: Makefile,v 1.3.2.2 2009/05/10 21:29:56 snj Exp $
#

PROG=		dhcpcd
SRCS=		arp.c bind.c common.c control.c configure.c dhcp.c dhcpcd.c \
		duid.c eloop.c if-options.c if-pref.c ipv4ll.c net.c \
		signals.c
SRCS+=		bpf.c if-bsd.c

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/external/bsd/dhcpcd/dist
.PATH: ${DIST}

CPPFLAGS+=	-I${DIST}

SCRIPTS=			dhcpcd-run-hooks
SCRIPTSDIR_dhcpcd-run-hooks=	/libexec

CONFIGFILES=			dhcpcd.conf
FILESDIR_dhcpcd.conf=		/etc

HOOKS=		01-test 10-mtu 20-resolv.conf 29-lookup-hostname \
		30-hostname 50-ntp.conf

FILES=			${HOOKS:C,^,${DIST}/dhcpcd-hooks/,}
FILESDIR=		/libexec/dhcpcd-hooks

MAN=			dhcpcd.conf.5 dhcpcd.8 dhcpcd-run-hooks.8

CLEANFILES=		dhcpcd.conf.5 dhcpcd.8 \
			dhcpcd-run-hooks dhcpcd-run-hooks.8

.for f in dhcpcd-run-hooks dhcpcd.conf.5 dhcpcd.8 dhcpcd-run-hooks.8
${f}:	${f}.in
	${TOOL_SED} -e 's:@SYSCONFDIR@:/etc:g' -e 's:@DBDIR@:/var/db:g' \
	    -e 's:@HOOKDIR@:/libexec/dhcpcd-hooks:g' \
	    -e 's:@SCRIPT@:/libexec/dhcpcd-run-hooks:g' ${DIST}/${f}.in > $@
.endfor

.include <bsd.prog.mk>
