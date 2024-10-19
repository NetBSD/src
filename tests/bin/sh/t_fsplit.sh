# $NetBSD: t_fsplit.sh,v 1.10 2024/10/19 11:59:51 kre Exp $
#
# Copyright (c) 2007-2016 The NetBSD Foundation, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

# The standard
# http://www.opengroup.org/onlinepubs/009695399/utilities/xcu_chap02.html
# explains (section 2.6) that Field splitting should be performed on the
# result of variable expansions.
# In particular this means that in ${x-word}, 'word' must be expanded as if
# the "${x-" and "}" were absent from the input line.
#
# So: sh -c 'set ${x-a b c}; echo $#' should give 3.
# and: sh -c 'set -- ${x-}' echo $#' should give 0
#

# the implementation of "sh" to test
: ${TEST_SH:="/bin/sh"}

nl='
'

check()
{
	if [ "${TEST}" -eq 0 ]
	then
		FAILURES=
	fi

	TEST=$((${TEST} + 1))

	case "$#" in
	(2)	;;
	(*)	atf_fail "Internal test error, $# args to check, test ${TEST}";;
	esac

	result=$( ${TEST_SH} -c "unset x a b d c e f g h; $1" )
	STATUS="$?"

	# Remove newlines
	oifs="$IFS"
	IFS="$nl"
	result="$(echo $result)"
	IFS="$oifs"

	#  # trim the test text in case we use it in a message below
	#  case "$1" in
	#  ????????????????*)
	#	  set -- "$(expr "$1" : '\(............\).*')..." "$2" ;;
	#  esac

	if [ "$2" != "${result}" ]
	then
		FAILURES="${FAILURES}${FAILURES:+ }${TEST}"
		printf >&2 'Sub-test %d failed:\n      %s\n' \
			"${TEST}" "$1"
		printf >&2 ' Expected: [%s]\n' "$2"
		printf >&2 ' Received: [%s]\n' "${result}"

		if [ "${STATUS}" != 0 ]
		then
			printf >&2 ' Sub-test exit status: %d\n' "${STATUS}"
		fi
	elif [ "${STATUS}" != 0 ]
	then
		FAILURES="${FAILURES}${FAILURES:+ }${TEST}"
		printf >&2 'Sub-test %d failed:\n\t%s\n' \
			"${TEST}" "$1"
		printf >&2 ' Sub-test exit status: %d (with correct output)\n' \
			"${STATUS}"
	fi

	return 0
}

check_results()
{
	NAME=$1

	set -- ${FAILURES}

	if [ $# -eq 0 ]
	then
		return 0
	fi

	unset IFS
	printf >&2 'Subtest %s: %d sub-tests (of %d) [%s] failed.\n' \
		"${NAME}" "$#" "${TEST}" "$*"

	atf_fail "$# of ${TEST} sub-tests (${FAILURES}), see stderr"
	return 0
}

atf_test_case for
for_head()
{
	atf_set "descr" "Checks field splitting in for loops"
}
for_body()
{
	unset x

	TEST=0
	# Since I managed to break this, leave the test in
	check 'for f in $x; do echo x${f}y; done' ''

	check_results for
}

atf_test_case default_val
default_val_head()
{
	atf_set "descr" "Checks field splitting in variable default values"
}
default_val_body()
{
	TEST=0
	# Check that IFS is applied to text from ${x-...} unless it is inside
	# any set of "..."
	check 'set -- ${x-a b c};   echo $#'   3

	check 'set -- ${x-"a b" c}; echo $#'   2
	check 'set -- ${x-a "b c"}; echo $#'   2
	check 'set -- ${x-"a b c"}; echo $#'   1

	check "set -- \${x-'a b' c}; echo \$#" 2
	check "set -- \${x-a 'b c'}; echo \$#" 2
	check "set -- \${x-'a b c'}; echo \$#" 1

	check 'set -- ${x-a\ b c};  echo $#'   2
	check 'set -- ${x-a b\ c};  echo $#'   2
	check 'set -- ${x-a\ b\ c}; echo $#'   1

	check 'set -- ${x};        echo $#' 0
	check 'set -- ${x-};       echo $#' 0
	check 'set -- ${x-""};     echo $#' 1
	check 'set -- ""${x};      echo $#' 1
	check 'set -- ""${x-};     echo $#' 1
	check 'set -- ""${x-""};   echo $#' 1
	check 'set -- ${x}"";      echo $#' 1
	check 'set -- ${x-}"";     echo $#' 1
	check 'set -- ${x-""}"";   echo $#' 1
	check 'set -- ""${x}"";    echo $#' 1
	check 'set -- ""${x-}"";   echo $#' 1
	check 'set -- ""${x-""}""; echo $#' 1

	check 'for i in ${x-a b c};            do echo "z${i}z"; done' \
		'zaz zbz zcz'
	check 'for i in ${x-"a b" c};          do echo "z${i}z"; done' \
		'za bz zcz'
	check 'for i in ${x-"a ${x-b c}" d};   do echo "z${i}z"; done' \
		'za b cz zdz'
	check 'for i in ${x-a ${x-b c} d};     do echo "z${i}z"; done' \
		'zaz zbz zcz zdz'

	# I am not sure the first of these two is correct, the rules on
	# quoting word in ${var-word} are peculiar, and hard to fathom...
	# It is what the NetBSD shell does, and bash, not the freebsd shell
	# and not ksh93 (as of Mar 1, 2016, and still in June 2017)
	# The likely correct interp of the next one is 'za bz zcz zdz'

	# That and the "should be" below are correct as of POSIX 7 TC2
	# But this is going to change to "unspecified" in POSIX 8
	# (resolution of bug 221)  so instead of being incorrect (as now)
	# the NetBSD shell will simply be implementing is version
	# of unspecified behaviour.  Just beware that shells differ,
	# a shell that fails this test is not incorrect because of it.

	# should be:    uuuu qqqqqq uuu q uuu   (unquoted/quoted) no nesting.
	check 'for i in ${x-"a ${x-"b c"}" d}; do echo "z${i}z"; done' \
		'za b cz zdz'
	check 'for i in ${x-a ${x-"b c"} d};   do echo "z${i}z"; done' \
		'zaz zb cz zdz'

	check_results default_val
}

atf_test_case replacement_val
replacement_val_head()
{
	atf_set "descr" "Checks field splitting in variable replacement values"
}
replacement_val_body()
{
	TEST=0

	# Check that IFS is applied to text from ${x+...} unless it is inside
	# any set of "...", or whole expansion is quoted, or both...

	check 'x=BOGUS; set -- ${x+a b c};   echo $#'   3

	check 'x=BOGUS; set -- ${x+"a b" c}; echo $#'   2
	check 'x=BOGUS; set -- ${x+a "b c"}; echo $#'   2
	check 'x=BOGUS; set -- ${x+"a b c"}; echo $#'   1

	check "x=BOGUS; set -- \${x+'a b' c}; echo \$#" 2
	check "x=BOGUS; set -- \${x+a 'b c'}; echo \$#" 2
	check "x=BOGUS; set -- \${x+'a b c'}; echo \$#" 1

	check 'x=BOGUS; set -- ${x+a\ b c};  echo $#'   2
	check 'x=BOGUS; set -- ${x+a b\ c};  echo $#'   2
	check 'x=BOGUS; set -- ${x+a\ b\ c}; echo $#'   1

	check 'x=BOGUS; set -- ${x+};       echo $#' 0
	check 'x=BOGUS; set -- ${x+""};     echo $#' 1
	check 'x=BOGUS; set -- ""${x+};     echo $#' 1
	check 'x=BOGUS; set -- ""${x+""};   echo $#' 1
	check 'x=BOGUS; set -- ${x+}"";     echo $#' 1
	check 'x=BOGUS; set -- ${x+""}"";   echo $#' 1
	check 'x=BOGUS; set -- ""${x+}"";   echo $#' 1
	check 'x=BOGUS; set -- ""${x+""}""; echo $#' 1

	# verify that the value of $x does not affecty the value of ${x+...}
	check 'x=BOGUS; set -- ${x+};       echo X$1' X
	check 'x=BOGUS; set -- ${x+""};     echo X$1' X
	check 'x=BOGUS; set -- ""${x+};     echo X$1' X
	check 'x=BOGUS; set -- ""${x+""};   echo X$1' X
	check 'x=BOGUS; set -- ${x+}"";     echo X$1' X
	check 'x=BOGUS; set -- ${x+""}"";   echo X$1' X
	check 'x=BOGUS; set -- ""${x+}"";   echo X$1' X
	check 'x=BOGUS; set -- ""${x+""}""; echo X$1' X

	check 'x=BOGUS; set -- ${x+};       echo X${1-:}X' X:X
	check 'x=BOGUS; set -- ${x+""};     echo X${1-:}X' XX
	check 'x=BOGUS; set -- ""${x+};     echo X${1-:}X' XX
	check 'x=BOGUS; set -- ""${x+""};   echo X${1-:}X' XX
	check 'x=BOGUS; set -- ${x+}"";     echo X${1-:}X' XX
	check 'x=BOGUS; set -- ${x+""}"";   echo X${1-:}X' XX
	check 'x=BOGUS; set -- ""${x+}"";   echo X${1-:}X' XX
	check 'x=BOGUS; set -- ""${x+""}""; echo X${1-:}X' XX

	# and validate that the replacement can be used as expected
	check 'x=BOGUS; for i in ${x+a b c};            do echo "z${i}z"; done'\
		'zaz zbz zcz'
	check 'x=BOGUS; for i in ${x+"a b" c};          do echo "z${i}z"; done'\
		'za bz zcz'
	check 'x=BOGUS; for i in ${x+"a ${x+b c}" d};   do echo "z${i}z"; done'\
		'za b cz zdz'

	# see the (extended) comment in the default_val test.  This will be
	# unspecified, hence we are OK (will be) but expect differences.
	# also incorrect:        uuuu qqqqqq uuu q uuu
	check 'x=BOGUS; for i in ${x+"a ${x+"b c"}" d}; do echo "z${i}z"; done'\
		'za b cz zdz'

	check 'x=BOGUS; for i in ${x+a ${x+"b c"} d};   do echo "z${i}z"; done'\
		'zaz zb cz zdz'
	check 'x=BOGUS; for i in ${x+a ${x+b c} d};     do echo "z${i}z"; done'\
		'zaz zbz zcz zdz'

	check_results replacement_val
}

atf_test_case ifs_alpha
ifs_alpha_head()
{
	atf_set "descr" "Checks that field splitting works with alphabetic" \
	                "characters"
}
ifs_alpha_body()
{
	unset x

	TEST=0
	# repeat with an alphabetic in IFS
	check 'IFS=q; set ${x-aqbqc}; echo $#' 3
	check 'IFS=q; for i in ${x-aqbqc};            do echo "z${i}z"; done' \
		'zaz zbz zcz'
	check 'IFS=q; for i in ${x-"aqb"qc};          do echo "z${i}z"; done' \
		'zaqbz zcz'
	check 'IFS=q; for i in ${x-"aq${x-bqc}"qd};   do echo "z${i}z"; done' \
		'zaqbqcz zdz'

	# this is another almost certainly incorrect expectation
	# (but again, see comment in default_val test - becoming unspecified.)
	#                        uu qqqqqq uuu q uu	(quoted/unquoted)
	check 'IFS=q; for i in ${x-"aq${x-"bqc"}"qd}; do echo "z${i}z"; done' \
		'zaqbqcz zdz'

	check 'IFS=q; for i in ${x-aq${x-"bqc"}qd};  do echo "z${i}z"; done' \
		'zaz zbqcz zdz'

	check_results ifs_alpha
}

atf_test_case quote
quote_head()
{
	atf_set "descr" "Checks that field splitting works with multi-word" \
	                "fields"
}
quote_body()
{
	unset x

	TEST=0
	# Some quote propagation checks
	check 'set "${x-a b c}";   echo $#' 1

	# this is another almost certainly incorrect expectation
	# (but again, see comment in default_val test - becoming unspecified.)
	#           qqqq uuu qqq  	(quoted/unquoted)  $1 is a $# is 2
	check 'set "${x-"a b" c}"; echo $1' 'a b c'

	check 'for i in "${x-a b c}"; do echo "z${i}z"; done' 'za b cz'

	check_results quote
}

atf_test_case dollar_at
dollar_at_head()
{
	atf_set "descr" "Checks that field splitting works when expanding" \
	                "\$@"
}
dollar_at_body()
{
	unset x

	TEST=0
	# Check we get "$@" right

	check 'set --;        for i in x"$@"x;  do echo "z${i}z"; done' 'zxxz'
	check 'set a;         for i in x"$@"x;  do echo "z${i}z"; done' 'zxaxz'
	check 'set a b;       for i in x"$@"x;  do echo "z${i}z"; done' \
		'zxaz zbxz'

	check 'set --;        for i;            do echo "z${i}z"; done' ''
	check 'set --;        for i in $@;      do echo "z${i}z"; done' ''
	check 'set --;        for i in "$@";    do echo "z${i}z"; done' ''
	# atf_expect_fail "PR bin/50834"
	check 'set --;        for i in ""$@;    do echo "z${i}z"; done' 'zz'
	# atf_expect_pass
	check 'set --;        for i in $@"";    do echo "z${i}z"; done' 'zz'
	check 'set --;        for i in ""$@"";  do echo "z${i}z"; done' 'zz'
	check 'set --;        for i in """$@";  do echo "z${i}z"; done' 'zz'
	check 'set --;        for i in "$@""";  do echo "z${i}z"; done' 'zz'
	check 'set --;        for i in """$@""";do echo "z${i}z"; done' 'zz'

	check 'set "";        for i;            do echo "z${i}z"; done' 'zz'
	check 'set "";        for i in "$@";    do echo "z${i}z"; done' 'zz'
	check 'set "" "";     for i;            do echo "z${i}z"; done' 'zz zz'
	check 'set "" "";     for i in "$@";    do echo "z${i}z"; done' 'zz zz'
	check 'set "" "";     for i in $@;      do echo "z${i}z"; done' ''

	check 'set "a b" c;   for i;            do echo "z${i}z"; done' \
		'za bz zcz'
	check 'set "a b" c;   for i in "$@";    do echo "z${i}z"; done' \
		'za bz zcz'
	check 'set "a b" c;   for i in $@;      do echo "z${i}z"; done' \
		'zaz zbz zcz'
	check 'set " a b " c; for i in "$@";    do echo "z${i}z"; done' \
		'z a b z zcz'

	check 'set a b c;     for i in "$@$@";  do echo "z${i}z"; done' \
		'zaz zbz zcaz zbz zcz'
	check 'set a b c;     for i in "$@""$@";do echo "z${i}z"; done' \
		'zaz zbz zcaz zbz zcz'

	check_results dollar_at
}

atf_test_case ifs
ifs_head()
{
	atf_set "descr" "Checks that IFS correctly configures field" \
	                "splitting behavior"
}
ifs_body()
{
	unset x

	TEST=0
	# Some IFS tests
	check 't="-- "; IFS=" ";  set $t; IFS=":"; r="$*"; IFS=; echo $# $r' '0'
	check 't=" x";  IFS=" x"; set $t; IFS=":"; r="$*"; IFS=; echo $# $r' '1'
	check 't=" x "; IFS=" x"; set $t; IFS=":"; r="$*"; IFS=; echo $# $r' '1'
	check 't=axb; IFS="x"; set $t; IFS=":"; r="$*"; IFS=; echo $# $r'  \
		'2 a:b'
	check 't="a x b"; IFS="x";  set $t; IFS=":"; r="$*"; IFS=; echo $# $r' \
		'2 a : b'
	check 't="a xx b"; IFS="x"; set $t; IFS=":"; r="$*"; IFS=; echo $# $r' \
		'3 a :: b'
	check 't="a xx b"; IFS="x ";set $t; IFS=":"; r="$*"; IFS=; echo $# $r' \
		'3 a::b'
	# A recent 'clarification' means that a single trailing IFS
	# non-whitespace doesn't generate an empty parameter
	check 't="xax"; IFS="x"; set $t; IFS=":"; r="$*"; IFS=; echo $# $r' \
		'2 :a'
	check 't="xax "; IFS="x "; set $t; IFS=":"; r="$*"; IFS=; echo $# $r' \
		'2 :a'
	# Verify that IFS isn't being applied where it shouldn't be.
	check 'IFS="x"; set axb; IFS=":"; r="$*"; IFS=; echo $# $r' '1 axb'
	check 'IFS=x; set axb; IFS=:; r=$*; IFS=; echo $# $r'       '1 axb'
	check 'IFS=x; set axb; set -- "$*"; IFS=:; r=$*; IFS=; echo $# $r' \
		'1 axb'
	check 'IFS=x; set axb; set --  $*  ; IFS=:; r=$*; IFS=; echo $# $r' \
		'2 a:b'

	check_results ifs
}

atf_test_case var_length
var_length_head()
{
	atf_set "descr" "Checks that field splitting works when expanding" \
	                "a variable's length"
}
var_length_body()
{
	TEST=0

	long=12345678123456781234567812345678
	long=$long$long$long$long
	export long
	unset x

	# first test that the test method works...
	check 'set -u; : ${long}; echo ${#long}' '128'

	# Check that we apply IFS to ${#var}
	check 'echo ${#long}; IFS=2; echo ${#long}; set 1 ${#long};echo $#' \
		'128 1 8 3'
	check 'IFS=2; set ${x-${#long}};   IFS=" "; echo $* $#'     '1 8 2'
	check 'IFS=2; set ${x-"${#long}"}; IFS=" "; echo $* $#'     '128 1'
	check 'IFS=2; set "${x-${#long}}"; IFS=" "; echo $* $#'     '128 1'
	check 'IFS=2; set ${x-${#long}};   :      ; echo $* $#'     '1 8 '
	check 'IFS=2; set ${x-${#long}};   :      ; echo $* "$#"'   '1 8 2'
	check 'IFS=2; set ${x-${#long}};   :      ; echo "$*" "$#"' '128 2'
	check 'IFS=2; set ${x-${#long}};   :      ; echo "$@" "$#"' '1 8 2'

	check_results var_length
}

atf_test_case split_arith
split_arith_head()
{
	atf_set "descr" "Checks that field splitting works when expanding" \
	                "the results from arithmetic"
}
split_arith_body()
{
	TEST=0

	# Check that we apply IFS to $(( expr ))

	# Note: we do not check the actual arithmetic operations here
	# (there is a separate test just for that) so we just enter
	# the "answer" inside $(( )) ... also makes it easier to visualise

	check 'IFS=5; echo $(( 123456789 ))'	'1234 6789'
	check 'IFS=5; echo "$(( 123456789 ))"'	'123456789'
	check 'IFS=37; echo $(( 123456789 ))'	'12 456 89'
	check 'IFS=37; echo "$(( 123456789 ))"'	'123456789'
	check 'IFS=159; echo $(( 123456789 ))'	' 234 678'

	check 'IFS=5; set -- $(( 123456789 )); echo $#: $1 $2 $3 $4' \
		'2: 1234 6789'
	check 'IFS=5; set -- "$(( 123456789 ))"; echo $#: $1 $2 $3 $4' \
		'1: 1234 6789'		# go ahead: explain it!
	check 'IFS=5; set -- "$(( 123456789 ))"; echo "$#: $1 $2 $3 $4"' \
		'1: 123456789   '	# ah!

	check 'IFS=37; set -- $(( 123456789 )); echo $#: $1 $2 $3 $4' \
		' : 12 456 89'		# Tricky!
	check 'IFS=5; set -- $(( 123456789 )); echo $#: $*' \
		'2: 1234 6789'
	check 'IFS=47; set -- $(( 123456789 )); echo $#: $*' \
		'3: 123 56 89'
	check 'IFS=5; set -- $(( 123456789 )); echo "$#: $*"' \
		'2: 123456789'
	check 'IFS=37; set -- $(( 123456789 )); echo "$#: $*"' \
		'3: 123456389'	# [sic]
	check 'IFS=5; set -- $(( 123456789 )); echo $#: $@' \
		'2: 1234 6789'
	check 'IFS=47; set -- $(( 123456789 )); echo $#: $@' \
		'3: 123 56 89'
	check 'IFS=5; set -- $(( 123456789 )); echo "$#: $@"' \
		'2: 1234 6789'
	check 'IFS=37; set -- $(( 123456789 )); echo "$#: $*"' \
		'3: 123456389'	# [sic]

	check 'IFS=1; set -- $(( 1111 )); echo "$#:" $*'	'4:   '
	check 'IFS=" 1"; set -- $(( 1231231231 )); echo "$#: $*"' \
		'4:  23 23 23'
	check 'IFS="1 "; set -- $(( 1231231231 )); echo "$#: $*"' \
		'4: 123123123'

	check 'IFS=5; echo 5$(( 123456789 ))5'		'51234 67895'
	check 'IFS=37; echo 73$(( 123456789 ))37'	'7312 456 8937'
	check 'IFS=159; echo 11$(( 123456789 ))95'	'11 234 678 95'
	check 'IFS="159 "; echo 11$(( 123456789 ))95'	'11 234 678 95'
	check 'IFS="159 "; echo 11$(( 11234567899 ))95'	'11  234 678  95'

	check_results split_arith
}

atf_test_case read_split
read_split_head()
{
	atf_set "descr" "Checks that field splitting works for the read" \
	                "built-in utility"
}
#
# CAUTION: There are literal <tab> chars in the following test.
# It is important that they be retained as is (the ones in the data
# and results - those used for test formatting are immaterial).
#
read_split_body()
{
	DATA="  aaa bbb:ccc ddd+eee	fff:ggg+hhh	  "   # CAUTION: tabs!

	TEST=0

	check "unset IFS; printf '%s\n' '${DATA}' | {
	  read a b c d e f g h || printf 'FAIL:%d' \"\$?\" &&
	  printf '<%s>' "'"$a" "$b" "$c" "$d" "$e" "$f" "$g" "$h"; }' \
	  '<aaa><bbb:ccc><ddd+eee><fff:ggg+hhh><><><><>'

	check "unset IFS; printf '%s\n' '${DATA}' | {
	  read x || printf 'FAIL:%d' \"\$?\" &&
	  printf '<%s>' "'"$x"; }' \
	  '<aaa bbb:ccc ddd+eee	fff:ggg+hhh>'

	check "IFS=; printf '%s\n' '${DATA}' | {
	  read a b c d e f g h || printf 'FAIL:%d' \"\$?\" &&
	  printf '<%s>' "'"$a" "$b" "$c" "$d" "$e" "$f" "$g" "$h"; }' \
	  "<${DATA}><><><><><><><>"

	check "IFS=' 	'; printf '%s\n' '${DATA}' | {
	  read a b c d e f g h || printf 'FAIL:%d' \"\$?\" &&
	  printf '<%s>' "'"$a" "$b" "$c" "$d" "$e" "$f" "$g" "$h"; }' \
	  '<aaa><bbb:ccc><ddd+eee><fff:ggg+hhh><><><><>'

	check "IFS=':'; printf '%s\n' '${DATA}' | {
	  read a b c d e f g h || printf 'FAIL:%d' \"\$?\" &&
	  printf '<%s>' "'"$a" "$b" "$c" "$d" "$e" "$f" "$g" "$h"; }' \
	  '<  aaa bbb><ccc ddd+eee	fff><ggg+hhh	  ><><><><><>'

	check "IFS=': '; printf '%s\n' '${DATA}' | {
	  read a b c d e f g h || printf 'FAIL:%d' \"\$?\" &&
	  printf '<%s>' "'"$a" "$b" "$c" "$d" "$e" "$f" "$g" "$h"; }' \
	  '<aaa><bbb><ccc><ddd+eee	fff><ggg+hhh	><><><>'

	check "IFS=':	'; printf '%s\n' '${DATA}' | {
	  read a b c d e f g h || printf 'FAIL:%d' \"\$?\" &&
	  printf '<%s>' "'"$a" "$b" "$c" "$d" "$e" "$f" "$g" "$h"; }' \
	  '<  aaa bbb><ccc ddd+eee><fff><ggg+hhh><  ><><><>'

	check "IFS='+'; printf '%s\n' '${DATA}' | {
	  read a b c d e f g h || printf 'FAIL:%d' \"\$?\" &&
	  printf '<%s>' "'"$a" "$b" "$c" "$d" "$e" "$f" "$g" "$h"; }' \
	  '<  aaa bbb:ccc ddd><eee	fff:ggg><hhh	  ><><><><><>'

	check "IFS=' +'; printf '%s\n' '${DATA}' | {
	  read a b c d e f g h || printf 'FAIL:%d' \"\$?\" &&
	  printf '<%s>' "'"$a" "$b" "$c" "$d" "$e" "$f" "$g" "$h"; }' \
	  '<aaa><bbb:ccc><ddd><eee	fff:ggg><hhh	><><><>'

	check "IFS='+	'; printf '%s\n' '${DATA}' | {
	  read a b c d e f g h || printf 'FAIL:%d' \"\$?\" &&
	  printf '<%s>' "'"$a" "$b" "$c" "$d" "$e" "$f" "$g" "$h"; }' \
	  '<  aaa bbb:ccc ddd><eee><fff:ggg><hhh><  ><><><>'

	# This tests the bug from PR bin/57849 (which existed about 2 days)
	# It also tests that a var-assign before read does not corrupt the
	# value of the var in the executing shell environment
	check "IFS='+'; printf '%s\n' '${DATA}' | {
	  IFS=: read a b c d e f g h || printf 'FAIL:%d' \"\$?\" &&
	  printf '<%s>' "'"$IFS" "$a" "$b" "$c" "$d" "$e" "$f" "$g" "$h"; }' \
	  '<+><  aaa bbb><ccc ddd+eee	fff><ggg+hhh	  ><><><><><>'

	check "IFS='+'; printf '%s\n' '${DATA}' | {
	  IFS= read a b c d e f g h || printf 'FAIL:%d' \"\$?\" &&
	  printf '<%s>' "'"$IFS" "$a" "$b" "$c" "$d" "$e" "$f" "$g" "$h"; }' \
	  "<+><${DATA}><><><><><><><>"

	# This doesn't really belong here, just tests that EOF works...
	# (and that read sets unused vars to '', doesn't leave them unset)
	check "unset IFS; set -u;
	  read a b c d e f g h </dev/null || printf 'FAIL:%d' \"\$?\" &&
	  printf '<%s>' "'"$a" "$b" "$c" "$d" "$e" "$f" "$g" "$h"'	\
	  "FAIL:1<><><><><><><><>"

	# And a similar one where EOF follows some data (which is read)
	check "unset IFS; set -u; printf 'a b c' | {
	  read a b c d e f g h || printf 'FAIL:%d' \"\$?\" &&
	  printf '<%s>' "'"$a" "$b" "$c" "$d" "$e" "$f" "$g" "$h"; }'	\
	  "FAIL:1<a><b><c><><><><><>"

	check_results read_split
}

atf_init_test_cases()
{
	atf_add_test_case for
	atf_add_test_case default_val
	atf_add_test_case replacement_val
	atf_add_test_case ifs_alpha
	atf_add_test_case quote
	atf_add_test_case dollar_at
	atf_add_test_case ifs
	atf_add_test_case var_length
	atf_add_test_case split_arith
	atf_add_test_case read_split
}
