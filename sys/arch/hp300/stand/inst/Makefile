#	$NetBSD: Makefile,v 1.6 2001/12/14 07:36:32 gmcgarry Exp $

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

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
release: inst.lif
	mkdir -p $(RELEASEDIR)/installation/misc
	rm -f $(RELEASEDIR)/installation/misc/SYS_INST
	${INSTALL} ${COPY} -m 444 inst.lif \
	    $(RELEASEDIR)/installation/misc/SYS_INST
.endif # RELEASEDIR check

