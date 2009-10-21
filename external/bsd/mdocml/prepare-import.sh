#/bin/sh

set -e

cd dist
rm -rf ChangeLog.xsl Makefile example.style.css index.css *.sgml

uuencode external.png < external.png > external.png.uu

rm external.png

for f in [a-z]*; do
	sed 's/\$Id: prepare-import.sh,v 1.1 2009/10/21 18:04:52 joerg Exp $Vendor-Id:/' < $f > $f.new && mv $f.new $f
done
