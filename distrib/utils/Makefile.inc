# $NetBSD: Makefile.inc,v 1.4.2.1 1997/12/13 00:10:14 gwr Exp $
# utils one might want in a crunched binary

LDSTATIC=	-static

# Want to avoid floating point in the RAMDISK
# so the FPU emulator is not required.
.if (${MACHINE_ARCH} == "m68k")
COPTS+= -msoft-float
.endif

install:	.NOTMAIN
	@echo "install not expected"

