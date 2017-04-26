TOP?=	../
include ${TOP}/iconfig.mk

PROG=		dhcpcd-run-hooks
BINDIR=		${LIBEXECDIR}
CLEANFILES=	dhcpcd-run-hooks
MAN8=		dhcpcd-run-hooks.8
CLEANFILES+=	dhcpcd-run-hooks.8

SCRIPTSDIR=	${HOOKDIR}
SCRIPTS=	01-test 02-dump
SCRIPTS+=	20-resolv.conf
SCRIPTS+=	30-hostname
SCRIPTS+=	${HOOKSCRIPTS}

# Some hooks should not be installed by default
FILESDIR=	${DATADIR}/dhcpcd/hooks
FILES=		10-wpa_supplicant
FILES+=		15-timezone
FILES+=		29-lookup-hostname
FILES+=		${EGHOOKSCRIPTS}

.SUFFIXES:	.in

.in: Makefile ${TOP}/config.mk
	${SED} ${SED_RUNDIR} ${SED_DBDIR} ${SED_LIBDIR} ${SED_HOOKDIR} \
		${SED_SYS} ${SED_SCRIPT} ${SED_DATADIR} \
		${SED_SERVICEEXISTS} ${SED_SERVICECMD} ${SED_SERVICESTATUS} \
		${SED_STATUSARG} \
		-e 's:@YPDOMAIN_DIR@:${YPDOMAIN_DIR}:g' \
		-e 's:@YPDOMAIN_SUFFIX@:${YPDOMAIN_SUFFIX}:g' \
		$< > $@

all: ${PROG} ${MAN8} ${SCRIPTS} ${EGHOOKSCRIPTS}

clean:
	rm -f ${CLEANFILES} 50-ypbind

depend:

proginstall: ${PROG} ${HOOKSCRIPTS}
	${INSTALL} -d ${DESTDIR}${BINDIR}
	${INSTALL} -m ${BINMODE} ${PROG} ${DESTDIR}${BINDIR}
	${INSTALL} -d ${DESTDIR}${SCRIPTSDIR}
	${INSTALL} -m ${NONBINMODE} ${SCRIPTS} ${DESTDIR}${SCRIPTSDIR}
	# We need to remove the old MTU change script if we at all can.
	rm -f ${DESTDIR}${SCRIPTSDIR}/10-mtu

eginstall: ${EGHOOKSCRIPTS}
	${INSTALL} -d ${DESTDIR}${FILESDIR}
	${INSTALL} -m ${NONBINMODE} ${FILES} ${DESTDIR}${FILESDIR}

maninstall: ${MAN8}
	${INSTALL} -d ${DESTDIR}${MANDIR}/man8
	${INSTALL} -m ${MANMODE} ${MAN8} ${DESTDIR}${MANDIR}/man8

install: proginstall eginstall maninstall

import: ${HOOKSCRIPTS}
	${INSTALL} -d /tmp/${DISTPREFIX}/dhcpcd-hooks
	${INSTALL} -m ${NONBINMODE} ${SCRIPTS} /tmp/${DISTPREFIX}/dhcpcd-hooks
	${INSTALL} -m ${NONBINMODE} ${FILES} /tmp/${DISTPREFIX}/dhcpcd-hooks

include ${TOP}/Makefile.inc
