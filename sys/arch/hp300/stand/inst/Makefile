#	$NetBSD: Makefile,v 1.10.26.2 2024/06/22 10:57:11 martin Exp $

PROGELF=	inst
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
