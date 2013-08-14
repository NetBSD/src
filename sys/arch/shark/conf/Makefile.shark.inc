#	$NetBSD: Makefile.shark.inc,v 1.17 2013/08/14 14:45:12 martin Exp $

CPPFLAGS+=		-D${MACHINE}
CFLAGS+=		-fno-unwind-tables
GENASSYM_EXTRAS+=	${THISARM}/shark/genassym.cf

# Need to convert the kernel from ELF to a.out so that OpenFirmware
# can load it.

LINKFLAGS=	-T ldscript

SYSTEM_LD_HEAD_EXTRA+=; \
	( cat ${ARM}/conf/kern.ldscript.head ; \
	  OBJDUMP=${OBJDUMP} ${HOST_SH} $S/conf/mkldscript.sh \
		${SYSTEM_OBJ} ; \
	  cat ${ARM}/conf/kern.ldscript.tail ) > ldscript

SYSTEM_LD_TAIL_EXTRA+=; \
	echo "${OBJCOPY} ${OBJCOPY_ELF2AOUT_FLAGS} $@ $@.aout"; \
	${OBJCOPY} ${OBJCOPY_ELF2AOUT_FLAGS} $@ $@.aout

KERNIMAGES=	netbsd netbsd.aout
