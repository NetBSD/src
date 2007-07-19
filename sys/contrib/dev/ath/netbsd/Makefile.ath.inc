#	$NetBSD: Makefile.ath.inc,v 1.13.2.1 2007/07/19 14:38:23 liamjfoy Exp $

# Rules for building a HAL on various platforms.  We put them here
# to centralize them all in one place.

##
## This selects a "reasonable default" HAL object.  Override it if you
## like by setting ATH_MACHINE_HAL in your architecture specific.  Otherwise
## the "default" will be used, which might not be what you want.  E.g.
## there is an optimized HAL for xscale, or you might need N32 mode code
## for 64-bit MIPS hardware, or the mips1 HAL for really ancient MIPS
## hardware (R3000).
##

#
# First we look for some specific MACHINEs that we know about
#
.if ${MACHINE} == "macppc"
ATH_HAL=powerpc-be-eabi

#
# Then use use MACHINE_ARCH if we still don't know
#
.elif ${MACHINE_ARCH} == "alpha"
ATH_HAL=alpha-elf

.elif ${MACHINE_ARCH} == "arm"
ATH_HAL=armv4-le-elf

.elif ${MACHINE_ARCH} == "armeb"
ATH_HAL=armv4-be-elf

.elif ${MACHINE_ARCH} == "mipseb"
ATH_HAL=mips-be-elf

.elif ${MACHINE_ARCH} == "mipsel"
ATH_HAL=mips-le-elf

.elif ${MACHINE_ARCH} == "i386"
ATH_HAL=i386-elf

.elif ${MACHINE_ARCH} == "powerpc"
ATH_HAL=powerpc-be-elf

.elif ${MACHINE_ARCH} == "sparc"
ATH_HAL=sparc-be-elf

.elif ${MACHINE_ARCH} == "sparc64"
ATH_HAL=sparc64-be-elf

.elif ${MACHINE_ARCH} == "x86_64"
ATH_HAL=x86_64-elf

.endif

.if !empty(ATH_MACHINE_HAL)
ATH_HAL=${ATH_MACHINE_HAL}
.endif

.if !empty(OBJS:M\/athhal.o)
OBJS:=${OBJS:C/\/athhal.o/athhal.o/}

ATH_UUDEC?=	@${_MKSHMSG} "uudecode ${.CURDIR:T}/${.TARGET}"; \
		${_MKSHECHO}\
		${UUDECODE} -p $> \> ${.TARGET}; \
		rm -f ${.TARGET}; \
		${UUDECODE} -p $> > ${.TARGET}

athhal.o: $S/contrib/dev/ath/public/${ATH_HAL}.hal.o.uu
	${ATH_UUDEC}

athhal_options.h: $S/contrib/dev/ath/public/${ATH_HAL}.opt_ah.h
	${_MKSHECHO} cp $> ${.TARGET}
	rm -f ${.TARGET}
	cp $> ${.TARGET}

depend if_ath_cardbus.o if_ath_pci.o athrate-sample.o ath_netbsd.o ath.o ah_osdep.o: athhal_options.h

EXTRA_INCLUDES+= -I$S/contrib/dev/ath/netbsd
.endif
