#/bin/sh

set -e

cd dist
rm -rf ChangeLog.xsl Makefile example.style.css index.css *.sgml

uuencode external.png < external.png > external.png.uu
rm external.png

for f in [a-z]*; do
	sed 's/[$]Id:/\$Vendor-Id:/' < $f > $f.new && mv $f.new $f
done
