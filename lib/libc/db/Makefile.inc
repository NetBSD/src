#	from: @(#)Makefile.inc	8.1 (Berkeley) 6/2/93
#	$Id: Makefile.inc,v 1.3 1993/08/01 05:41:09 mycroft Exp $

.include "${.CURDIR}/db/btree/Makefile.inc"
.include "${.CURDIR}/db/db/Makefile.inc"
.include "${.CURDIR}/db/hash/Makefile.inc"
.include "${.CURDIR}/db/man/Makefile.inc"
.include "${.CURDIR}/db/mpool/Makefile.inc"
.include "${.CURDIR}/db/recno/Makefile.inc"
