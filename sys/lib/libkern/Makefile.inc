#	$NetBSD: Makefile.inc,v 1.10 1995/06/24 20:31:18 christos Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

KERNDIR=	$S/lib/libkern

KERNDST=	lib/kern
KERNREL=	../..
KERNLIB=	${KERNDST}/libkern.a
KERNLIB_PROF=	${KERNDST}/libkern_p.a

${KERNLIB}:		.NOTMAIN __always_make_kernlib
	@echo making sure the kern library is up to date...
	@(cd ${KERNDST} && ${MAKE} -f ${KERNREL}/${KERNDIR}/Makefile \
	  KERNCC="${CC}" \
	  KERNCFLAGS="${CFLAGS}" \
	  KERNREL="${KERNREL}" \
	  KERNDIR="${KERNDIR}" libkern.a)

${KERNLIB_PROF}:	.NOTMAIN __always_make_kernlib
	@echo making sure the profiled kern library is up to date...
	@(cd ${KERNDST} && ${MAKE} -f ${KERNREL}/${KERNDIR}/Makefile \
	  KERNCC="${CC}" \
	  KERNCFLAGS="${CFLAGS}" \
	  KERNREL="${KERNREL}" \
	  KERNDIR="${KERNDIR}" libkern_p.a)

clean:: 		.NOTMAIN __always_make_kernlib
	@echo cleaning the kern library objects
	@(cd ${KERNDST} && ${MAKE} -f ${KERNREL}/${KERNDIR}/Makefile \
	  KERNCC="${CC}" \
	  KERNCFLAGS="${CFLAGS}" \
	  KERNREL="${KERNREL}" \
	  KERNDIR="${KERNDIR}" clean)

__always_make_kernlib:	.NOTMAIN
	@([ -d ${KERNDST} ] || mkdir -p ${KERNDST})
