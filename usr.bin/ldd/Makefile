#	$NetBSD: Makefile,v 1.7 1998/12/15 22:07:11 pk Exp $
#
# ldd(1) is toolchain dependent
#

.include <bsd.own.mk>			# get OBJECT_FMT definition

.if (${OBJECT_FMT} == "ELF")
SUBDIR+= ldd_elf
.else
SUBDIR+= ldd_aout
.endif

.include <bsd.subdir.mk>
