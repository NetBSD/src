#	$NetBSD: Makefile,v 1.10 2008/10/25 22:27:39 apb Exp $

.include <bsd.own.mk>

LIB=	dps
DIST=	${X11SRCDIR.xc}/lib/${LIB}
.PATH:	${DIST}

SRCS=	dpsabbrev.c dpsclient.c dpsdict.c dpsexcept.c dpsprintf.c \
	XDPS.c dpsXclient.c dpsXcmu.c dpsXpriv.c dpsXtdisp.c dpssysnames.c \
	csconndi.c csfindNX.c cslibext.c cslibint.c csopendi.c csstartNX.c \
	${DPSOPSCFILES} ${PSOPSCFILES}

INCS=	dpsops.h psops.h
INCSDIR=${X11INCDIR}/DPS

CPPFLAGS+=	-DXDPS -DCSDPS -DTCPCONN -DUNIXCONN

DPSOPSHEADERFILES= \
	dpsclrops.h dpsctrlops.h dpsctxtops.h dpsdataops.h dpsfontops.h \
	dpsgsttops.h dpsioops.h dpsmathops.h dpsmtrxops.h dpsmiscops.h \
	dpsopstack.h dpspntops.h dpspathops.h dpssysops.h dpswinops.h \
	dpsXops.h dpsl2ops.h

DPSOPSCFILES= ${DPSOPSHEADERFILES:.h=.c}

PSOPSHEADERFILES= \
	psclrops.h psctrlops.h psctxtops.h psdataops.h psfontops.h \
	psgsttops.h psioops.h psmathops.h psmtrxops.h psmiscops.h \
	pspntops.h pspathops.h pssysops.h pswinops.h psopstack.h \
	psXops.h psl2ops.h

PSOPSCFILES= ${PSOPSHEADERFILES:.h=.c}

DPSRCS+=	${DPSOPSHEADERFILES} ${DPSOPSCFILES}
CLEANFILES+=	${DPSOPSHEADERFILES} ${DPSOPSCFILES}
DPSRCS+=	${PSOPSHEADERFILES} ${PSOPSCFILES}
CLEANFILES+=	${PSOPSHEADERFILES} ${PSOPSCFILES}
CPPFLAGS+=	-I.

psops.h: ${PSOPSHEADERFILES} psname.txt header.txt psifdef.txt psendif.txt
	${_MKTARGET_CREATE}
	rm -rf ${.TARGET} ${.TARGET}.tmp
	( \
		(cd ${DIST} && cat psname.txt header.txt psifdef.txt ); \
		cat ${PSOPSHEADERFILES} \
		    | ${TOOL_SED} -e '/^$$/D' -e '/#/D' -e '/^\//D' \
			-e '/^   gener/D' -e '/^.\//D' \
		    | sort \
		    | ${TOOL_AWK} '/;/ {print; printf("\n");}' ; \
		(cd ${DIST} && cat psendif.txt ); \
	) > ${.TARGET}.tmp \
	&& mv ${.TARGET}.tmp ${.TARGET}
CLEANFILES+=	psops.h psops.h.tmp

dpsops.h: ${DPSOPSHEADERFILES} dpsname.txt header.txt dpsifdef.txt dpsendif.txt
	${_MKTARGET_CREATE}
	rm -rf ${.TARGET} ${.TARGET}.tmp
	( \
		(cd ${DIST} && cat dpsname.txt header.txt dpsifdef.txt ); \
		cat ${DPSOPSHEADERFILES} \
		    | ${TOOL_SED} -e '/^$$/D' -e '/#/D' -e '/^\//D' \
			-e '/^   gener/D' -e '/^.\//D' \
		    | sort \
		    | ${TOOL_AWK} '/;/ {print; printf("\n");}' ; \
		(cd ${DIST} && cat dpsendif.txt ); \
	) > ${.TARGET}.tmp \
	&& mv ${.TARGET}.tmp ${.TARGET}
CLEANFILES+=	dpsops.h dpsops.h.tmp

.include "${NETBSDSRCDIR}/x11/tools/pswrap/Makefile.pswrap"

LIBDPLIBS=\
	Xt	${.CURDIR}/../Xt \
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
