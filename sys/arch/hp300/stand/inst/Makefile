#	$NetBSD: Makefile,v 1.11 2024/04/29 07:13:42 tsutsui Exp $

PROGAOUT=	inst
PROGSOURCE=	inst.c
NEWVERSWHAT=	"Miniroot Installer"

CPPFLAGS+=	-DSUPPORT_ETHERNET -DSUPPORT_TAPE -DSUPPORT_DISK

LINKS=	${BINDIR}/${PROG} ${BINDIR}/rbootd/SYS_INST

.include "../Makefile.buildboot"

beforeinstall:
	${INSTALL} -d -m 755 -o ${BINOWN} -g ${BINGRP} ${INSTPRIV} \
	    ${DESTDIR}${BINDIR}/rbootd

release: check_RELEASEDIR .WAIT ${PROG}
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${PROG} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc/SYS_INST
