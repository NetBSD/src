#	$NetBSD: Makefile.inc,v 1.9 2026/10/04 03:28:42 mrg Exp $
#	@(#)Makefile.inc	8.2 (Berkeley) 2/21/94
#
CPPFLAGS+=-D__DBINTERFACE_PRIVATE

.include "${.PARSEDIR}/btree/Makefile.inc"
.include "${.PARSEDIR}/db/Makefile.inc"
.include "${.PARSEDIR}/hash/Makefile.inc"
.include "${.PARSEDIR}/man/Makefile.inc"
.include "${.PARSEDIR}/mpool/Makefile.inc"
.include "${.PARSEDIR}/recno/Makefile.inc"
