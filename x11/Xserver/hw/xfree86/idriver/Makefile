# $NetBSD: Makefile,v 1.1.2.1 2004/11/15 09:12:41 rtr Exp $

.include "../input/Makefile"

LIB=	idriver

.for DN in ${SUBDIR}
.include "../input/${DN}/Makefile.${DN}"
.for FN in ${SRCS.${DN}}
CPPFLAGS.${FN}+=	${CPPFLAGS.${DN}}
.endfor
SRCS+=	${SRCS.${DN}}
.endfor

SRCS+=		drvConf.c
DPSRCS+=	drvConf.c
CLEANFILES+=	drvConf.c

drvConf.c:
	@echo '#include "xf86.h"' > ${.TARGET}
	@echo '#include "xf86Xinput.h"' >> ${.TARGET}
	@echo >> ${.TARGET}
	@echo 'extern InputDriverRec' >> ${.TARGET}
	@echo '	xf86KEYBOARD,' >> ${.TARGET}
.for SD in ${SUBDIR:Njs_x:Nkbd:[1..-2]:tu}
	@echo '	${SD},' >> ${.TARGET}
.endfor
	@echo '	${SUBDIR:Njs_x:Nkbd:[-1]:tu};' >> ${.TARGET}
	@echo >> ${.TARGET}
	@echo 'InputDriverPtr xf86InputDriverList[] = {' >> ${.TARGET}
	@echo '	&xf86KEYBOARD,' >> ${.TARGET}
.for SD in ${SUBDIR:Njs_x:Nkbd:tu}
	@echo '	&${SD},' >> ${.TARGET}
.endfor
	@echo '};' >> ${.TARGET}
	@echo 'int xf86NumInputDrivers = sizeof(xf86InputDriverList) / sizeof(xf86InputDriverList[0]);' >> ${.TARGET}

CPPFLAGS.drvConf.c=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
			-I${X11SRCDIR.xc}/programs/Xserver/include \
			-I${X11SRCDIR.xc}/include \
			-I${XSERVERINCDIR}
.include <bsd.x11.mk>
.include <bsd.lib.mk>
