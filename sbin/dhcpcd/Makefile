# $NetBSD: Makefile,v 1.2 2008/05/26 20:17:00 joerg Exp $
#

PROG=		dhcpcd
SRCS=		bpf.c client.c common.c configure.c dhcp.c dhcpcd.c \
		if-bsd.c logger.c net.c signals.c

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/dist/dhcpcd
.PATH: ${DIST}

CPPFLAGS+=	-I${DIST}

SCRIPTS=			dhcpcd-run-hooks
SCRIPTSDIR_dhcpcd-run-hooks=	/libexec

CONFIGFILES=			dhcpcd.conf
FILESDIR_dhcpcd.conf=		/etc

HOOKS=		01-test 10-resolv.conf 14-lookup-hostname \
		15-hostname 50-ntp.conf

FILES=			${HOOKS:C,^,${DIST}/hook.d/,}
FILESDIR=		/libexec/dhcpcd-hooks

MAN=			dhcpcd.conf.5 dhcpcd.8 dhcpcd-run-hooks.8

CLEANFILES=		dhcpcd.conf.5 dhcpcd.8 \
			dhcpcd-run-hooks dhcpcd-run-hooks.8

.for f in dhcpcd-run-hooks dhcpcd.conf.5 dhcpcd.8 dhcpcd-run-hooks.8
${f}:	${f}.in
	sed -e 's:@SYSCONFDIR@:/etc:g' -e 's:@DBDIR@:/var/db:g' \
	    -e 's:@HOOKDIR@:/libexec/dhcpcd-hooks:g' \
	    -e 's:@SCRIPT@:/libexec/dhcpcd.sh:g' ${DIST}/${f}.in > $@
.endfor

.include <bsd.prog.mk>
