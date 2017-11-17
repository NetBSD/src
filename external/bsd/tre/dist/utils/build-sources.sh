#! /bin/sh
#
# Creates the source code distribution packages.
#

set -e

rm -rf tmp-build
mkdir tmp-build
cd tmp-build

../configure
make dist

mkdir stage1-tree
cd stage1-tree
tar xzvf ../tre-*.tar.gz
cd ..

mkdir stage2
cd stage2
tar xzvf ../tre-*.tar.gz
cd tre-*
./configure
make dist
cd ../..

mkdir stage2-tree
cd stage2-tree
tar xzvf ../stage2/tre-*/tre-*.tar.gz
cd ..

diff -r -wibu stage1-tree stage2-tree

base=`basename tre-*.tar.gz .tar.gz`
gunzip -c tre-*.tar.gz | bzip2 -9 -c > $base.tar.bz2
bunzip2 -c < $base.tar.bz2 | tar xf -
zip -9 -r $base.zip $base
chmod a+r $base.tar.gz $base.tar.bz2 $base.zip

cd ..
if test ! -d dist; then
  mkdir dist
fi
mv tmp-build/$base.tar.gz tmp-build/$base.tar.bz2 tmp-build/$base.zip dist
ls -l dist
