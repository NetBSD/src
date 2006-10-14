# $Id: Makefile,v 1.1 2006/10/14 04:46:30 yamt Exp $

CPROGS=		lirs
HPROGS=		lfu lru nbsd opt rand
HPROGS+=	genpat5
HPROGS+=	renumber
PROGS+=		linux

C_COMPILE=	cc -O2 -Wall -Werror

all:	_mkall


# clock-pro

CPRO_CFLAGS.x=	-DNOCLRINITREF
CPRO_CFLAGS.t=	-DCOLDREF2TEST
CPRO_CFLAGS.useonce_t=	-DCOLDREF2TEST -DUSEONCE
CPRO_CFLAGS.useonce=	-DUSEONCE
CPRO_CFLAGS.nohash=	-DNOHASH
CPRO_CFLAGS.nomove=	-DCOLDNOMOVE
CPRO_CFLAGS.nomove_t=	-DCOLDNOMOVE -DCOLDREF2TEST
CPRO_CFLAGS.nomove_useonce_t=	-DCOLDNOMOVE -DCOLDREF2TEST -DUSEONCE
CPRO_CFLAGS.iwamoto=	-DIWAMOTO
CPRO_CFLAGS.iwamoto_t=	-DIWAMOTO -DCOLDREF2TEST
CPRO_CFLAGS.idealirr=	-DIDEALIRR
CPRO_CFLAGS.listq=	-DLISTQ
CPRO_CFLAGS.listq_t=	-DLISTQ -DCOLDREF2TEST
CPRO_CFLAGS.listq_useonce_t=	-DLISTQ -DCOLDREF2TEST -DUSEONCE
CPRO_CFLAGS.listq_useonce=	-DLISTQ -DUSEONCE
CPRO_CFLAGS.default=

CPRO_CFLAGS.listq_adaptive=	-DADAPTIVE -DLISTQ
CPRO_CFLAGS.listq_useonce_adaptive=	-DADAPTIVE -DLISTQ -DUSEONCE
CPRO_CFLAGS.useonce_adaptive=	-DADAPTIVE -DUSEONCE
CPRO_CFLAGS.adaptive=	-DADAPTIVE

PDSIM_CPRO_VARIANTS+=	default
PDSIM_CPRO_VARIANTS+=	listq
PDSIM_CPRO_VARIANTS+=	useonce
PDSIM_CPRO_VARIANTS+=	listq_useonce

PDSIM_CPRO_VARIANTS+=	listq_adaptive
PDSIM_CPRO_VARIANTS+=	useonce_adaptive
PDSIM_CPRO_VARIANTS+=	listq_useonce_adaptive
PDSIM_CPRO_VARIANTS+=	adaptive

PDSIM_PROGS+=	${PDSIM_CPRO_VARIANTS:S/^/pdsim.cpro_/}
PDSIM_PROGS.DBG+=	${PDSIM_CPRO_VARIANTS:S/^/pdsim.dbg.cpro_/}
PDPOL=		uvm_pdpolicy_clockpro.c
SYSSRC=		${NETBSDSRC}/sys
.PATH:		${SYSSRC}/uvm

.for _V in ${PDSIM_CPRO_VARIANTS}
_P=pdsim.cpro_${_V}
${_P}:	pdsim.c ${PDPOL}
	${C_COMPILE} -I. -I${SYSSRC} -DPDSIM ${CPRO_CFLAGS.${_V}} -o ${.TARGET} ${.ALLSRC}
pdsim.dbg.cpro_${_V}:	pdsim.c ${PDPOL}
	${C_COMPILE} -I. -I${SYSSRC} -DPDSIM -DDEBUG ${CPRO_CFLAGS.${_V}} -o ${.TARGET} ${.ALLSRC}
.endfor


# clock

CLOCK_CFLAGS.defult=
CLOCK_CFLAGS.inact90=	-DCLOCK_INACTIVEPCT=90

PDSIM_CLOCK_VARIANTS+=	default
PDSIM_CLOCK_VARIANTS+=	inact90

PDSIM_PROGS+=	${PDSIM_CLOCK_VARIANTS:S/^/pdsim.clock_/}
PDSIM_PROGS.DBG+=	${PDSIM_CLOCK_VARIANTS:S/^/pdsim.dbg.clock_/}
PDPOL=		uvm_pdpolicy_clock.c
SYSSRC=		/sys
.PATH:		${SYSSRC}/uvm
.for _V in ${PDSIM_CLOCK_VARIANTS}
_P=pdsim.clock_${_V}
${_P}:	pdsim.c ${PDPOL}
	${C_COMPILE} -I. -I${SYSSRC} -DPDSIM ${CLOCK_CFLAGS.${_V}} -o ${.TARGET} ${.ALLSRC}
pdsim.dbg.clock_${_V}:	pdsim.c ${PDPOL}
	${C_COMPILE} -I. -I${SYSSRC} -DPDSIM -DDEBUG ${CLOCK_CFLAGS.${_V}} -o ${.TARGET} ${.ALLSRC}
.endfor


PROGS+=		${PDSIM_PROGS}
PROGS+=		${PDSIM_PROGS.DBG}

ALLPROGS=	${PROGS} ${CPROGS} ${HPROGS} ${TESTPROGS}
.SUFFIXES:	.hs

_mkall: ${ALLPROGS}

clean:
	rm -rf ${ALLPROGS}
	rm -rf ${HPROGS:S/$/.hi/g}
	rm -rf ${HPROGS:S/$/.o/g}

.hs:
	rm -rf ${.IMPSRC:S/.hs/.o/g}
	ghc -cpp -O -o ${.TARGET} --make ${.TARGET}

linux:	nbsd.hs
	rm -rf ${.ALLSRC:S/.hs/.o/g}
	ghc -cpp -O -DLINUX -o ${.TARGET} --make nbsd
