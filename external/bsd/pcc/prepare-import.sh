#!/bin/sh -x
#
#	prepare pcc distribution for import
#
# pcc can be built as part of the toolchain by setting
#
#	HAVE_PCC=yes
#
# and as a native binary for a release build by setting
#
#	MKPCC=yes
#	MKPCCCMDS=yes
#

#
# TODO
# - some files only have NetBSD tags to start with, they end up with none

set -e

if [ ! -d work -o ! -d work/pcc -o ! -d work/pcc-libs ]; then
	echo "checkout or unpack pcc and pcc-libs to work/ first, eg"
	echo ""
	echo "    cvs -d :pserver:anonymous@pcc.ludd.ltu.se:/cvsroot -f checkout -P -d work -N pcc pcc-libs"
	echo ""
	exit 1
fi

echo "====> Removing pcc CVS directories..."
find work -type d -name CVS | xargs rm -Rf

echo "====> Fixing file and directory permissions..."
find work -type d -exec chmod u=rwx,go=rx {} \;
find work -type f -exec chmod u=rw,go=r {} \;

echo "====> Fixing RCS tags..."
# fix existing RCS tags, and insert blank NetBSD tags
for f in $(grep -RL '\$(NetBSD|Id).*\$' work); do
    sed -e '/\$NetBSD\$/d'				\
	-e 's,\$\(NetBSD[[:>:]].*\)\$,\1,'		\
	-e 's,\(.*\)\$\(Id[[:>:]].*\)\$\(.*\),\1\2\3	\
\1\$NetBSD\$\3,'					\
	< ${f} > ${f}_tmp
    mv ${f}_tmp ${f}
done

echo "====> Creating pcc \"config.h\" file..."
mkdir work/tmp
cd work/tmp
sh ../pcc/configure
cd ../..
#
# comment out items we provide at build time from Makefile.inc
# modify the PACKAGE_STRING to include the checkout date
# define PREPROCESSOR as pcpp to avoid conflicts with GCC
#
sed -e "s,^\(#define[[:space:]]*VERSSTR[[:>:]].*\)\$,/* \1 */,"					\
    -e "s,^\(#define[[:space:]]*HOST_BIG_ENDIAN[[:>:]].*\)\$,/* \1 */,"				\
    -e "s,^\(#define[[:space:]]*HOST_LITTLE_ENDIAN[[:>:]].*\)\$,/* \1 */,"			\
    -e "s,^\(#define[[:space:]]*TARGET_BIG_ENDIAN[[:>:]].*\)\$,/* \1 */,"			\
    -e "s,^\(#define[[:space:]]*TARGET_LITTLE_ENDIAN[[:>:]].*\)\$,/* \1 */,"			\
    -e "s,^\(#define[[:space:]]*PACKAGE_STRING[[:>:]].*\".*\)\(\".*\)\$,\1 [$(date +%Y%m%d)]\2,"\
    -e "s,^\(.*[[:<:]]PREPROCESSOR[[:>:]].*\)\$,#define PREPROCESSOR \"pcpp\","			\
    < work/tmp/config.h > work/config.h

echo "====> Replacing pcc sources..."
rm -Rf dist/pcc dist/pcc-libs
mv work/pcc work/pcc-libs dist
if cmp -s work/config.h include/config.h; then :; else
    echo "====> Updating include/config.h..."
    mv work/config.h include/config.h
fi

echo "====> Done."
rm -Rf work

echo ""
echo "after testing, use the following command to import from the dist directory,"
echo ""
echo "    cvs import src/external/bsd/pcc/dist ragge pcc-$(date +%y%m%d)"
echo ""
echo "providing a ChangeLog in the commit message."
