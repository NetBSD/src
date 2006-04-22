#	$NetBSD: Makefile.inc,v 1.3.6.1 2006/04/22 11:37:31 simonb Exp $

#	$OpenBSD: Makefile.inc,v 1.4 2001/03/29 03:58:17 mickey Exp $
#
#	NOTE:	$S must correspond to the top of the 'sys' tree
#		$HPPA must correspond to the top of hppa tree

SPMATHDIR=	${HPPA}/spmath

SPMATHDST?=	${.OBJDIR}/lib/spmath

SPMATH=		${SPMATHDST}/spmath.o
SPMATH_PROF=	${SPMATHDST}/spmath.po

SPMATHMAKE= \
	cd ${SPMATHDIR} && MAKEOBJDIRPREFIX= && unset MAKEOBJDIRPREFIX && \
	  MAKEOBJDIR=${SPMATHDST} ${MAKE} \
	    CC='${CC}' CFLAGS='${CFLAGS}' \
	    SPMATHCPPFLAGS='${CPPFLAGS:S@^-I.@-I. -I../../.@g}'\
	    AS='${AS}' AFLAGS='${AFLAGS}' \
	    LD='${LD}' STRIP='${STRIP}' \
	    CPP='${CPP}' STRIP='${STRIP}' AR='${AR}' \
	    NM='${NM}' LORDER='${LORDER}' \
	    XMACHINE='${MACHINE}' XMACHINE_ARCH='${MACHINE_ARCH}'

${SPMATH}:	.NOTMAIN .MAKE __always_make_spmath
	@echo making sure the spmath library is up to date...
	@${SPMATHMAKE} spmath.o

${SPMATH_PROF}:	.NOTMAIN .MAKE __always_make_spmath
	@echo making sure the profiled spmath library is up to date...
	@${SPMATHMAKE} spmath.po

__CLEANSPMATH:	.NOTMAIN .MAKE __always_make_spmath
	@echo cleaning the spmath library objects
	@${SPMATHMAKE} clean

clean:	__CLEANSPMATH

__DEPENDSPMATH:	.NOTMAIN .MAKE __always_make_spmath assym.h
	@echo depending the spmath library objects
	@${SPMATHMAKE} depend

depend:	__DEPENDSPMATH

__always_make_spmath:	.NOTMAIN
	-mkdir -p ${SPMATHDST}

