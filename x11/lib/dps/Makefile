#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:08 lukem Exp $

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

CLEANFILES+=	${DPSOPSHEADERFILES} ${DPSOPSCFILES}
CLEANFILES+=	${PSOPSHEADERFILES} ${PSOPSCFILES}

psops.h: ${PSOPSHEADERFILES} psname.txt header.txt psifdef.txt psendif.txt
	rm -rf ${.TARGET} ${.TARGET}.tmp
	( \
		(cd ${DIST} && cat psname.txt header.txt psifdef.txt ); \
		cat ${PSOPSHEADERFILES} \
		    | sed -e '/^$$/D' -e '/#/D' -e '/^\//D' \
			-e '/^   gener/D' -e '/^.\//D' \
		    | sort \
		    | awk '/;/ {print; printf("\n");}' ; \
		(cd ${DIST} && cat psendif.txt ); \
	) > ${.TARGET}.tmp \
	&& mv ${.TARGET}.tmp ${.TARGET}
CLEANFILES+=	psops.h psops.h.tmp

dpsops.h: ${DPSOPSHEADERFILES} dpsname.txt header.txt dpsifdef.txt dpsendif.txt
	rm -rf ${.TARGET} ${.TARGET}.tmp
	( \
		(cd ${DIST} && cat dpsname.txt header.txt dpsifdef.txt ); \
		cat ${DPSOPSHEADERFILES} \
		    | sed -e '/^$$/D' -e '/#/D' -e '/^\//D' \
			-e '/^   gener/D' -e '/^.\//D' \
		    | sort \
		    | awk '/;/ {print; printf("\n");}' ; \
		(cd ${DIST} && cat dpsendif.txt ); \
	) > ${.TARGET}.tmp \
	&& mv ${.TARGET}.tmp ${.TARGET}
CLEANFILES+=	dpsops.h dpsops.h.tmp

.include "${NETBSDSRCDIR}/x11/tools/pswrap/Makefile.pswrap"

#LM#	LDADD+=	-lXt -lSM -lICE -lXext -lX11
#LM#	DPADD+=	${LIBXT} ${LIBSM} ${LIBICE} ${LIBXEXT} ${LIBX11}

.include <bsd.lib.mk>
.include <bsd.x11.mk>
