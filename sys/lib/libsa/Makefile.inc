#	$NetBSD: Makefile.inc,v 1.5 1996/09/30 16:01:18 ws Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree
SADIR=	$S/lib/libsa

SADST=	lib/sa
SAREL?=	../../
SALIB?=	${SADST}/libsa.a

${SALIB}:		.NOTMAIN __always_make_salib
	@echo making sure the stand-alone library is up to date...
.if !defined(NO_NET)
	@(cd ${SADST} && ${MAKE} -f ${SAREL}${SADIR}/Makefile \
	  KERNCC="${CC}" \
	  KERNCFLAGS="${CFLAGS}" \
	  SAREL="${SAREL}" \
	  SADIR="${SADIR}" \
	  EXTRACFLAGS="${EXTRACFLAGS}" libsa.a)
.else
	@(cd ${SADST} && ${MAKE} -f ${SAREL}${SADIR}/Makefile \
	  KERNCC="${CC}" \
	  KERNCFLAGS="${CFLAGS}" \
	  SAREL="${SAREL}" \
	  SADIR="${SADIR}" \
	  NO_NET="" \
	  EXTRACFLAGS="${EXTRACFLAGS}" libsa.a)
.endif

clean:: 		.NOTMAIN __always_make_salib
	@echo cleaning the stand-alone library objects
	@(cd ${SADST} && ${MAKE} -f ${SAREL}${SADIR}/Makefile \
	  KERNCC="${CC}" \
	  KERNCFLAGS="${CFLAGS}" \
	  SAREL="${SAREL}" \
	  SADIR="${SADIR}" clean)

__always_make_salib:	.NOTMAIN
	@([ -d ${SADST} ] || mkdir -p ${SADST})

