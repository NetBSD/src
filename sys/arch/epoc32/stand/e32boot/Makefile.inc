#	$NetBSD: Makefile.inc,v 1.2 2013/05/01 22:57:37 matt Exp $

# Require pkgsrc/cross/psion-gcc.
ARM_EPOC_PE_GCC?=	/usr/pkg/bin/arm-epoc-pe-gcc
PETRAN?=		/usr/pkg/bin/petran

EPOC32_SDKDIR?=		/usr/pkg/psion-sdk/e32/

EXTERNAL_TOOLCHAIN?=	/usr/pkg

MACHINE_GNU_PLATFORM=	arm-epoc-pe

check_psion_gcc:
.if !exists(${ARM_EPOC_PE_GCC})
	@echo "Error: File does not exist: ${ARM_EPOC_PE_GCC}"
	@echo "Please build and install pkgsrc/cross/psion-gcc."
	@false
.endif

check_psion_utils:
.if !exists(${PETRAN})
	@echo "Error: File does not exist: ${PETRAN}"
	@echo "Please build and install pkgsrc/cross/psion-utils."
	@false
.endif

check_psion_sdk:
.if !exists(${EPOC32_SDKDIR}/include/e32base.h)
	@echo "Error: File does not exist: ${EPOC32_SDKDIR}/include/e32base.h"
	@echo "Please build and install pkgsrc/cross/psion-sdk-e32."
	@false
.endif

check_epoc32_cross: check_psion_gcc check_psion_utils check_psion_sdk
