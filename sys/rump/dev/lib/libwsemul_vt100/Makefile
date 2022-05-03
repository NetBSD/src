#	$NetBSD: Makefile,v 1.1 2022/05/03 14:11:59 uwe Exp $
#

# This library provides wscons vt100 emulation (aka TERM=wsvt25).
# This is internal module, not a device, so there's no RUMP_COMPONENT.
# The public interface provided by this module is wsemul_vt100_ops and
# it needs the user to provide (in rump namespace) wsdisplay_emulinput
# and wsdisplay_emulbell, provided in the real kernel by wsdisplay.

.PATH:	${.CURDIR}/../../../../dev/wscons

LIB=	rumpdev_wsemul_vt100
COMMENT=Workstation console VT100 emulation

SRCS+= wsemul_vt100.c
SRCS+= wsemul_vt100_chars.c
SRCS+= wsemul_vt100_keys.c
SRCS+= wsemul_vt100_subr.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
