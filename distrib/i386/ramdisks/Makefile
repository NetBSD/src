#	$NetBSD: Makefile,v 1.2 2006/11/19 11:17:51 dsl Exp $

SUBDIR=
SUBDIR+=	ramdisk-big

# These are only needed if you have a system with < 16MB (maybe 32MB)
# of memory.  In which case you probably want to build a custom one anyway.
# SUBDIR+=	ramdisk-rescuetiny
# SUBDIR+=	ramdisk-small
# SUBDIR+=	ramdisk-tiny

TARGETS+=	release

.include <bsd.subdir.mk>
