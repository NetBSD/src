#	$NetBSD: Makefile,v 1.2.2.1 2000/06/22 17:00:01 minoura Exp $

PROGAOUT=	inst
PROGSOURCE=	inst.c
NEWVERSWHAT=	"Miniroot Installer"

CPPFLAGS+=	-DSUPPORT_ETHERNET -DSUPPORT_TAPE -DSUPPORT_DISK

install:
	${INSTALL} -d -m 755 -o ${BINOWN} -g ${BINGRP} \
	    ${DESTDIR}${BINDIR}/rbootd
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 inst.lif \
	    ${DESTDIR}${BINDIR}/inst.lif
	rm -f ${DESTDIR}${BINDIR}/rbootd/SYS_INST
	ln ${DESTDIR}${BINDIR}/inst.lif ${DESTDIR}${BINDIR}/rbootd/SYS_INST

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release: inst.lif
	mkdir -p $(RELEASEDIR)/installation/misc
	rm -f $(RELEASEDIR)/installation/misc/SYS_INST
	${INSTALL} -c -m 444 inst.lif $(RELEASEDIR)/installation/misc/SYS_INST
.endif # RELEASEDIR check

.include "../Makefile.buildboot"
