#	$NetBSD: Makefile.inc,v 1.7 2011/05/26 12:56:29 joerg Exp $
#	@(#)Makefile.inc	8.2 (Berkeley) 2/21/94
#
CPPFLAGS+=-D__DBINTERFACE_PRIVATE

CWARNFLAGS.clang+=	-Wno-array-bounds

.include "${.CURDIR}/db/btree/Makefile.inc"
.include "${.CURDIR}/db/db/Makefile.inc"
.include "${.CURDIR}/db/hash/Makefile.inc"
.include "${.CURDIR}/db/man/Makefile.inc"
.include "${.CURDIR}/db/mpool/Makefile.inc"
.include "${.CURDIR}/db/recno/Makefile.inc"
