#! /bin/sh
#
#	$NetBSD: makewhatis.sh,v 1.18 1999/03/30 03:17:26 cjs Exp $
#
# written by matthew green <mrg@eterna.com.au>, based on the
# original by J.T. Conklin <jtc@netbsd.org> and Thorsten
# Frueauf <frueauf@ira.uka.de>.
#
# Public domain.
#

LIST=/tmp/makewhatislist$$
TMP=/tmp/whatis$$
trap "rm -f $LIST $TMP; exit 1" 1 2 15

MANDIR=${1-/usr/share/man}
if test ! -d "$MANDIR"; then 
	echo "makewhatis: $MANDIR: not a directory"
	exit 1
fi

if test -f $MANDIR/makewhatis.sed; then
	MKWHATIS=$MANDIR/makewhatis.sed
elif test -f $DESTDIR/usr/share/man/makewhatis.sed; then
	MKWHATIS=$DESTDIR/usr/share/man/makewhatis.sed
elif test -f /usr/share/man/makewhatis.sed; then
	MKWHATIS=/usr/share/man/makewhatis.sed
else
	echo Cannot find makewhatis.sed; exit 1
fi

find $MANDIR \( -type f -o -type l \) -name '*.[0-9]*' -ls | \
    sort -n | awk '{if (u[$1]) next; u[$1]++ ; print $11}' > $LIST
 
egrep '\.[1-9]$' $LIST | xargs /usr/libexec/getNAME | \
	sed -e 's/ [a-zA-Z0-9]* \\-/ -/' > $TMP

egrep '\.[1-9].(gz|Z)$' $LIST | while read file
do
	gzip -fdc $file | nroff -man | \
	sed -n -f $MKWHATIS;
done >> $TMP

egrep '\.0$' $LIST | while read file
do
	sed -n -f $MKWHATIS $file;
done >> $TMP

egrep '\.[0].(gz|Z)$' $LIST | while read file
do
	gzip -fdc $file | sed -n -f $MKWHATIS;
done >> $TMP

sort -u -o $TMP $TMP

make -f - DESTDIR="" install <<_Install_Whatis_Db_
FILES=$TMP
FILESDIR=$MANDIR
FILESNAME=whatis.db
NOOBJ=noobj

.include <bsd.prog.mk>
_Install_Whatis_Db_

rm -f $LIST $TMP
exit 0
