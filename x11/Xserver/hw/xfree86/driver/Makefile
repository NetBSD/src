# $NetBSD: Makefile,v 1.1.2.1 2004/11/15 09:12:39 rtr Exp $

.include "../drivers/Makefile"

LIB=	driver

.include "../drivers/cirrus/cirrus_alpine/Makefile.cirrus_alpine"
.include "../drivers/cirrus/cirrus_laguna/Makefile.cirrus_laguna"

.for FN in ${SRCS.cirrus_alpine}
CPPFLAGS.${FN}+=	${CPPFLAGS.cirrus_alpine}
.endfor

.for FN in ${SRCS.cirrus_laguna}
CPPFLAGS.${FN}+=	${CPPFLAGS.cirrus_laguna}
.endfor

SRCS+=	${SRCS.cirrus_alpine} ${SRCS.cirrus_laguna}

.for DN in ${SUBDIR}
.include "../drivers/${DN}/Makefile.${DN}"
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
	@echo >> ${.TARGET}
	@echo 'extern DriverRec' >> ${.TARGET}
.for SD in ${SUBDIR:Natimisc:Nr128:Nradeon:[1..-2]:tu}
	@echo '	${SD},' >> ${.TARGET}
.endfor
	@echo '	${SUBDIR:Natimisc:Nr128:Nradeon:[-1]:tu};' >> ${.TARGET}
	@echo >> ${.TARGET}
	@echo 'DriverPtr xf86DriverList[] = {' >> ${.TARGET}
.for SD in ${SUBDIR:Natimisc:Nr128:Nradeon:tu}
	@echo '	&${SD},' >> ${.TARGET}
.endfor
	@echo '};' >> ${.TARGET}
	@echo 'int xf86NumDrivers = sizeof(xf86DriverList) / sizeof(xf86DriverList[0]);' >> ${.TARGET}

CPPFLAGS.drvConf.c=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
			-I${X11SRCDIR.xc}/programs/Xserver/include \
			-I${X11SRCDIR.xc}/include \
			-I${XSERVERINCDIR}
	
.include <bsd.x11.mk>
.include <bsd.lib.mk>

