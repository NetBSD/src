#       @(#)Makefile.inc	5.3 (Berkeley) 8/6/92

.PATH: ${.CURDIR}/DB/man

# mpool.3
MAN3+=	btree.0 dbopen.0 hash.0 recno.0
MLINKS+= dbopen.3 db.3
