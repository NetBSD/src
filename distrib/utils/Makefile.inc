# $NetBSD: Makefile.inc,v 1.6 1998/03/02 19:50:53 cgd Exp $
# utils one might want in a crunched binary

LDSTATIC=	-static		# only static compilation makes sense here

# Want to avoid floating point in the RAMDISK
# so the FPU emulator is not required.
.if (${MACHINE_ARCH} == "m68k")
COPTS+= -msoft-float
.endif

install:	.NOTMAIN
	@echo "install not expected"

