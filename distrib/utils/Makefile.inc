# $NetBSD: Makefile.inc,v 1.5 1997/12/12 23:26:29 gwr Exp $
# utils one might want in a crunched binary

LDSTATIC=	-static

# Want to avoid floating point in the RAMDISK
# so the FPU emulator is not required.
.if (${MACHINE_ARCH} == "m68k")
COPTS+= -msoft-float
.endif

install:	.NOTMAIN
	@echo "install not expected"

