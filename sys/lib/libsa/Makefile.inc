#	$NetBSD: Makefile.inc,v 1.7 1997/01/02 22:06:51 christos Exp $
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
	  KERNCC=${CC:Q} \
	  KERNCFLAGS=${CFLAGS:Q} \
	  SAREL=${SAREL:Q} \
	  SADIR=${SADIR:Q} \
	  EXTRACFLAGS=${EXTRACFLAGS:Q} libsa.a)
.else
	@(cd ${SADST} && ${MAKE} -f ${SAREL}${SADIR}/Makefile \
	  KERNCC=${CC:Q} \
	  KERNCFLAGS=${CFLAGS:Q} \
	  SAREL=${SAREL:Q} \
	  SADIR=${SADIR:Q} \
	  NO_NET="" \
	  EXTRACFLAGS=${EXTRACFLAGS:Q} libsa.a)
.endif

clean:: 		.NOTMAIN __always_make_salib
	@echo cleaning the stand-alone library objects
	@(cd ${SADST} && ${MAKE} -f ${SAREL}${SADIR}/Makefile \
	  KERNCC=${CC:Q} \
	  KERNCFLAGS=${CFLAGS:Q} \
	  SAREL=${SAREL:Q} \
	  SADIR=${SADIR:Q} clean)

__always_make_salib:	.NOTMAIN
	@([ -d ${SADST} ] || mkdir -p ${SADST})

