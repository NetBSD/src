#! /bin/sh
#
# written by matthew green <mrg@eterna.com.au>, based on the
# original by J.T. Conklin <jtc@netbsd.org> and Thorsten
# Frueauf <frueauf@ira.uka.de>.
#
# Public domain.
#

trap "rm -f /tmp/makewhatislist$$ /tmp/whatis$$; exit 1" 1 2 15

MANDIR=${1-/usr/share/man}
LIST=/tmp/makewhatislist$$
TMP=/tmp/whatis$$
if test ! -d "$MANDIR"; then 
	echo "makewhatis: $MANDIR: not a directory"
	exit 1
fi

find $MANDIR \( -type f -or -type l \) -name '*.[0-9]*' -print > $LIST

egrep '\.0$' $LIST | while read file
do
	sed -n -f /usr/share/man/makewhatis.sed $file;
done > $TMP

egrep '\.[1-9]$|\.[1-9]?$' $LIST | xargs /usr/libexec/getNAME | \
	sed -e 's/ [a-zA-Z0-9]* \\-/ -/' >> $TMP


egrep '\.[0].(gz|Z)$' $LIST | while read file
do
	gzip -fdc $file | sed -n -f /usr/share/man/makewhatis.sed;
done >> $TMP

sort -u -o /tmp/whatis$$ /tmp/whatis$$

install -o bin -g bin -m 444 /tmp/whatis$$ "$MANDIR/whatis.db"
exit 0
