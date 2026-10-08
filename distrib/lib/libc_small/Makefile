#	$NetBSD: Makefile,v 1.1 2026/10/08 17:50:26 mrg Exp $

# Simple libc that elides most asserts etc, useful for size
# restricted media like installers.

NOPIC=
NOPROFILE=

.include "../../../lib/libc/Makefile"

CPPFLAGS+=	-DNDEBUG 
CPPFLAGS+=	-U_DIAGNOSTIC
