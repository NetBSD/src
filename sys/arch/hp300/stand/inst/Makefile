#	$NetBSD: Makefile,v 1.7.2.1 2004/08/03 10:34:47 skrll Exp $

PROGAOUT=	inst
PROGSOURCE=	inst.c
NEWVERSWHAT=	"Miniroot Installer"

CPPFLAGS+=	-DSUPPORT_ETHERNET -DSUPPORT_TAPE -DSUPPORT_DISK

.include "../Makefile.buildboot"

install:
	${INSTALL} -d -m 755 -o ${BINOWN} -g ${BINGRP} ${INSTPRIV} \
	    ${DESTDIR}${BINDIR}/rbootd
	${INSTALL} ${COPY} -m 444 -o ${BINOWN} -g ${BINGRP} ${INSTPRIV} \
	    inst.lif ${DESTDIR}${BINDIR}/inst.lif
	rm -f ${DESTDIR}${BINDIR}/rbootd/SYS_INST
	ln ${DESTDIR}${BINDIR}/inst.lif ${DESTDIR}${BINDIR}/rbootd/SYS_INST

release: check_RELEASEDIR .WAIT inst.lif
	${HOST_INSTALL_FILE} -m ${NONBINMODE} inst.lif \
	    ${RELEASEDIR}/${MACHINE}/installation/misc/SYS_INST
