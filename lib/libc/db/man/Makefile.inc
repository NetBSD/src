#	from: @(#)Makefile.inc	8.1 (Berkeley) 6/4/93
#	$Id: Makefile.inc,v 1.2 1993/08/01 05:41:17 mycroft Exp $

.PATH: ${.CURDIR}/db/man

# mpool.3
MAN3+=	btree.0 dbopen.0 hash.0 recno.0
MLINKS+= dbopen.3 db.3
