# $NetBSD: Makefile,v 1.8 2010/11/10 14:12:28 roy Exp $
#

PROG=		dhcpcd
SRCS=		arp.c bind.c common.c control.c configure.c dhcp.c dhcpcd.c \
		duid.c eloop.c if-options.c if-pref.c ipv4ll.c net.c \
		signals.c
SRCS+=		bpf.c if-bsd.c platform-bsd.c

# getline is only available in NetBSD-6, so build the compat shim
SRCS+=		getline.c

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/external/bsd/dhcpcd/dist
.PATH: ${DIST} ${DIST}/compat

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

SERVICEEXISTS=		[ -x /etc/rc.d/$$1 ]
SERVICECMD=		/etc/rc.d/$$1 $$2
SERVICESTATUS=		service_command $$1 status >/dev/null 2>\&1
.for f in dhcpcd-run-hooks dhcpcd.conf.5 dhcpcd.8 dhcpcd-run-hooks.8
${f}:	${f}.in
	${TOOL_SED} -e 's:@SYSCONFDIR@:/etc:g' -e 's:@DBDIR@:/var/db:g' \
	    -e 's:@HOOKDIR@:/libexec/dhcpcd-hooks:g' \
	    -e 's:@SCRIPT@:/libexec/dhcpcd-run-hooks:g' \
	    -e 's:@SERVICEEXISTS@:[ -x /etc/rc.d/$$1 ]:g' \
	    -e 's:@SERVICECMD@:/etc/rc.d/$$1 $$2:g' \
	    -e 's:@SERVICESTATUS@:service_command $$1 status >/dev/null 2>\&1:g' \
	    ${DIST}/${f}.in > $@
.endfor

.include <bsd.prog.mk>
