#!/bin/sh
#
# $NetBSD: greptests.sh,v 1.2 2003/09/14 15:10:49 cjep Exp $
#
#
# Regression tests for grep. Some of these tests are based on those
# from OpenBSD src/regress/usr.bin/grep/Makefile
#
# OpenBSD: Makefile,v 1.4 2003/07/17 19:14:43 pvalchev Exp 

p="$1";
[ -z "$1" ] && p=".";

# Temporary measure so that multiple greps can be tested
#
grep=grep
egrep=egrep
fgrep=fgrep
zgrep=zgrep

# Path to words directory
#
words=/usr/share/dict/words;


# Test 1: Simple grep from words.
#
echo "1. Simple grep"
${grep} aa ${words} | diff $p/test1.gnu.out -
echo ""

# Test 2: Binary files.
#
echo "2. Binary files"
${grep} NetBSD /bin/sh | diff $p/test2.gnu.out -
echo ""

# Test 3: Simple recursive grep
#
echo "3. Recursive grep"
tmpdir=`mktemp -d /tmp/greptest.XXXXXX` || exit 1;
mkdir -p ${tmpdir}/recurse/a/f ${tmpdir}/recurse/d
cat <<EOF > ${tmpdir}/recurse/a/f/favourite-fish
cod
haddock
plaice
EOF

cat <<EOF > ${tmpdir}/recurse/d/fish
cod
dover sole
haddock
halibut
pilchard
EOF

(cd ${tmpdir} && ${grep} -r haddock recurse) | diff $p/test3.gnu.out -
rm -rf ${tmpdir}
echo ""

# Test 4: Symbolic link recursion
#
echo "4. Symbolic link recursion"
tmpdir=`mktemp -d /tmp/greptest.XXXXXX` || exit 1;
mkdir -p ${tmpdir}/test/c/d
(cd ${tmpdir}/test/c/d && ln -s ../d .)
echo "Test string" > ${tmpdir}/test/c/match
(cd ${tmpdir} && ${grep} -r string test) 2>&1 | diff $p/test4.gnu.out -
rm -rf ${tmpdir}
echo ""

# Test 5: Test word-regexps
#
echo "5. Test word-regexps"
${grep} -w separated $p/input | diff $p/test5.gnu.out -
echo ""

# Test 6: Beginning and ends
#
echo "6. Beginnings and ends"
${grep} ^Front $p/input | diff $p/test6a.gnu.out -
${grep} ending$ $p/input | diff $p/test6b.gnu.out -
echo ""

# Test 7: Match on any case
#
echo "7. Ignore case"
${grep} -i Upper $p/input | diff $p/test7.gnu.out -
echo ""

# Test 8: Test -v
#
echo "8. -v"
${grep} -v fish $p/negativeinput | diff $p/test8.gnu.out -
echo ""

# Test 9: Test whole-line matching with -x
#
echo "9. Whole-line matching"
${grep} -x matchme $p/input | diff $p/test9.gnu.out -
echo ""

# Test 10: Check for negative matches
#
echo "10. Test for non-matches"
${grep} "not a hope in hell" $p/input | diff /dev/null -
echo ""

# Test 11: Context
#
echo "11. Context tests"
${grep} -C2 bamboo $p/contextinput | diff $p/test11a.gnu.out -
${grep} -A3 tilt $p/contextinput | diff $p/test11b.gnu.out -
${grep} -B4 Whig $p/contextinput | diff $p/test11c.gnu.out -
(cd $p && ${grep} -C1 pig contextinput contextinput1) \
	| diff $p/test11d.gnu.out -
echo ""

# Test 12: Test reading expressions from a file
#
echo "12. Test reading expressions from a file"
${grep} -f $p/test1.gnu.out ${words} | diff $p/test1.gnu.out -
echo ""

# Test 13: Extended grep
#
echo "13. Extended grep of special characters"
${egrep} '\?|\*$$' $p/input | diff $p/test13.gnu.out -
echo ""

# Test 14: zgrep
#
echo "14. zgrep"
tmpdir=`mktemp -d /tmp/greptest.XXXXXX` || exit 1;
cp $p/input ${tmpdir}/input
gzip ${tmpdir}/input
${zgrep} line ${tmpdir}/input.gz | diff $p/test14.gnu.out -
rm -rf ${tmpdir}
echo ""

# Test 15: Don't print out infomation about non-existent files
#
echo "15. Ignore non-existent files (-s)"
${grep} -s foobar $p/notexistent | diff /dev/null -
echo ""

# Test 16: Context output with -z
#
echo "16. Context output with -z"
tmpdir=`mktemp -d /tmp/greptest.XXXXXX` || exit 1;
printf "haddock\000cod\000plaice\000" > ${tmpdir}/test1
printf "mackeral\000cod\000crab\000" > ${tmpdir}/test2
(cd ${tmpdir} && ${grep} -z -A1 cod test1 test2) | diff $p/test16a.gnu.out -
(cd ${tmpdir} && ${grep} -z -B1 cod test1 test2) | diff $p/test16b.gnu.out -
(cd ${tmpdir} && ${grep} -z -C1 cod test1 test2) | diff $p/test16c.gnu.out -
rm -rf ${tmpdir}
echo ""

