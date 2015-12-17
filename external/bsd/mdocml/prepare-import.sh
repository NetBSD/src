#!/bin/sh

set -e

rm -rf CVS ChangeLog.xsl style.css index.css *.sgml regress

cleantags .
for f in [a-z]*; do
	sed -e 's/[$]Mdocdate: \([^$]*\) \([0-9][0-9][0-9][0-9]\) [$]/\1, \2/' < $f > $f.new && mv $f.new $f
done
