#	$NetBSD: Makefile.inc,v 1.3 1995/09/14 20:55:01 pk Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree
SADIR=	$S/lib/libsa

SADST=	lib/sa
SAREL?=	../../
SALIB?=	${SADST}/libsa.a

${SALIB}:		.NOTMAIN __always_make_salib
	@echo making sure the stand-alone library is up to date...
	@(cd ${SADST} && ${MAKE} -f ${SAREL}${SADIR}/Makefile \
	  KERNCC="${CC}" \
	  KERNCFLAGS="${CFLAGS}" \
	  SAREL="${SAREL}" \
	  SADIR="${SADIR}" libsa.a)

clean:: 		.NOTMAIN __always_make_salib
	@echo cleaning the stand-alone library objects
	@(cd ${SADST} && ${MAKE} -f ${SAREL}${SADIR}/Makefile \
	  KERNCC="${CC}" \
	  KERNCFLAGS="${CFLAGS}" \
	  SAREL="${SAREL}" \
	  SADIR="${SADIR}" clean)

__always_make_salib:	.NOTMAIN
	@([ -d ${SADST} ] || mkdir -p ${SADST})

