#	$NetBSD: Makefile.shark.inc,v 1.4.2.1.2.1 2005/02/06 07:45:26 jmc Exp $

MACHINE_ARCH=		arm
CPPFLAGS+=		-D${MACHINE}
CPPFLAGS+=		-I$S/dist/ipf
GENASSYM_EXTRAS+=	${THISARM}/shark/genassym.cf

.if (${OBJECT_FMT} == "ELF")
# Need to convert the kernel from ELF to a.out so that OpenFirmware
# can load it.

LINKFLAGS=	-T ldscript

SYSTEM_LD_HEAD_EXTRA+=; \
	( cat ${ARM}/conf/kern.ldscript.head ; \
	  OBJDUMP=${OBJDUMP} ${HOST_SH} ${ARM}/conf/mkldscript.sh \
		${SYSTEM_OBJ} ; \
	  cat ${ARM}/conf/kern.ldscript.tail ) > ldscript

SYSTEM_LD_TAIL_EXTRA+=; \
	echo "${DBSYM} $@ || true"; \
	${DBSYM} $@ || true; \
	echo \
	 "${OBJCOPY} -O a.out-arm-netbsd -R .ident -R .arm.atpcs -R .comment $@ $@.aout"; \
	${OBJCOPY} -O a.out-arm-netbsd -R .ident -R .arm.atpcs -R .comment $@ $@.aout
.endif
