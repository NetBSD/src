# System definitions

PICFLAG?=	-fPIC

BINMODE?=	0555
NONBINMODE?=	0444
MANMODE?=	${NONBINMODE}
CONFMODE?=	0644

CC?=		cc
INSTALL?=	install
LINT?=		lint
SED?=		sed
HOST_SH?=	/bin/sh

# This isn't very portable, but I generaly make releases from NetBSD
CKSUM?=		cksum -a SHA256
PGP?=		netpgp

SCRIPT=		${LIBEXECDIR}/dhcpcd-run-hooks
HOOKDIR=	${LIBEXECDIR}/dhcpcd-hooks

SED_RUNDIR=		-e 's:@RUNDIR@:${RUNDIR}:g'
SED_DBDIR=		-e 's:@DBDIR@:${DBDIR}:g'
SED_LIBDIR=		-e 's:@LIBDIR@:${LIBDIR}:g'
SED_DATADIR=		-e 's:@DATADIR@:${DATADIR}:g'
SED_HOOKDIR=		-e 's:@HOOKDIR@:${HOOKDIR}:g'
SED_SERVICEEXISTS=	-e 's:@SERVICEEXISTS@:${SERVICEEXISTS}:g'
SED_SERVICECMD=		-e 's:@SERVICECMD@:${SERVICECMD}:g'
SED_SERVICESTATUS=	-e 's:@SERVICESTATUS@:${SERVICESTATUS}:g'
SED_STATUSARG=		-e 's:@STATUSARG@:${STATUSARG}:g'
SED_SCRIPT=		-e 's:@SCRIPT@:${SCRIPT}:g'
SED_SYS=		-e 's:@SYSCONFDIR@:${SYSCONFDIR}:g'
