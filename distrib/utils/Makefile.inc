# $NetBSD: Makefile.inc,v 1.7 1999/04/28 09:18:13 drochner Exp $
# utils one might want in a crunched binary

LDSTATIC=	-static		# only static compilation makes sense here

# Want to avoid floating point in the RAMDISK
# so the FPU emulator is not required.
.if (${MACHINE_ARCH} == "m68k")
COPTS+= -msoft-float
.endif

install: .NOTMAIN .MADE
