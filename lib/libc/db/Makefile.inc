#	$NetBSD: Makefile.inc,v 1.5.10.1 1997/11/09 00:59:54 lukem Exp $
#	@(#)Makefile.inc	8.2 (Berkeley) 2/21/94
#
CPPFLAGS+=-D__DBINTERFACE_PRIVATE

.include "${.CURDIR}/db/btree/Makefile.inc"
.include "${.CURDIR}/db/db/Makefile.inc"
.include "${.CURDIR}/db/hash/Makefile.inc"
.include "${.CURDIR}/db/man/Makefile.inc"
.include "${.CURDIR}/db/mpool/Makefile.inc"
.include "${.CURDIR}/db/recno/Makefile.inc"
