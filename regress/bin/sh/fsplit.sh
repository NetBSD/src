#!/bin/sh
# The standard
# http://www.opengroup.org/onlinepubs/009695399/utilities/xcu_chap02.html
# explains (section 2.6) that Field splitting should be performed on the
# result of variable expansions.
# In particular this means that in ${x-word}, 'word' must be expanded as if
# the "${x-" and "}" were absent from the input line.
#
# So: sh -c 'set ${x-a b c}; echo $#' should give 3.
#

rval=0
exec >&2

nl='
'
OIFS="$IFS"

check()
{
	result="$(eval $1)"
	# Remove newlines
	IFS="$nl"
	result="$(echo $result)"
	IFS="$OIFS"
	if [ "$2" != "$result" ]
	then
		echo "command: $1"
		echo "    expected [$2], found [$result]"
		rval=1
	fi
}

unset x

# Since I managed to break this, leave the test in
check 'for f in $x; do echo x${f}y; done' ''

# Check that IFS is applied to text from ${x-...} unless it is inside
# any set of "..."
check 'set ${x-a b c}; echo $#' 3
check 'for i in ${x-a b c}; do echo "z${i}z"; done' 'zaz zbz zcz'
check 'for i in ${x-"a b" c}; do echo "z${i}z"; done' 'za bz zcz'
check 'for i in ${x-"a ${x-b c}" d}; do echo "z${i}z"; done' 'za b cz zdz'
check 'for i in ${x-"a ${x-"b c"}" d}; do echo "z${i}z"; done' 'za b cz zdz'
check 'for i in ${x-a ${x-"b c"} d}; do echo "z${i}z"; done' 'zaz zb cz zdz'
check 'for i in ${x-a ${x-b c} d}; do echo "z${i}z"; done' 'zaz zbz zcz zdz'

# repeat with an alphabetic in IFS
check 'IFS=q; set ${x-aqbqc}; echo $#' 3
check 'IFS=q; for i in ${x-aqbqc}; do echo "z${i}z"; done' 'zaz zbz zcz'
check 'IFS=q; for i in ${x-"aqb"qc}; do echo "z${i}z"; done' 'zaqbz zcz'
check 'IFS=q; for i in ${x-"aq${x-bqc}"qd}; do echo "z${i}z"; done' 'zaqbqcz zdz'
check 'IFS=q; for i in ${x-"aq${x-"bqc"}"qd}; do echo "z${i}z"; done' 'zaqbqcz zdz'
check 'IFS=q; for i in ${x-aq${x-"bqc"}qd}; do echo "z${i}z"; done' 'zaz zbqcz zdz'

# Some quote propogation checks
check 'set "${x-a b c}"; echo $#' 1
check 'set "${x-"a b" c}"; echo $1' 'a b c'
check 'for i in "${x-a b c}"; do echo "z${i}z"; done' 'za b cz'

# Check we get "$@" right
check 'set "a b" c; for i; do echo "z${i}z"; done' 'za bz zcz'
check 'set "a b" c; for i in "$@"; do echo "z${i}z"; done' 'za bz zcz'
check 'set "a b" c; for i in $@; do echo "z${i}z"; done' 'zaz zbz zcz'
check 'set " a b " c; for i in "$@"; do echo "z${i}z"; done' 'z a b z zcz'

# Some IFS tests
check 't=axb; IFS="x"; set $t; IFS=":"; r="$*"; IFS=; echo $# $r' '2 a:b'
check 't="a x b"; IFS="x"; set $t; IFS=":"; r="$*"; IFS=; echo $# $r' '2 a : b'
check 't="a xx b"; IFS="x"; set $t; IFS=":"; r="$*"; IFS=; echo $# $r' '3 a :: b'
check 't="a xx b"; IFS="x "; set $t; IFS=":"; r="$*"; IFS=; echo $# $r' '3 a::b'
check 't="xax"; IFS="x"; set $t; IFS=":"; r="$*"; IFS=; echo $# $r' '3 :a:'
check 'IFS="x"; set axb; IFS=":"; r="$*"; IFS=; echo $# $r' '1 axb'

# Check that we apply IFS to ${#var}
long=12345678123456781234567812345678
long=$long$long$long$long
check 'echo ${#long}; IFS=2; echo ${#long}; set 1 ${#long};echo $#' '128 1 8 3'
check 'IFS=2; set ${x-${#long}}; IFS=" "; echo $* $#' '1 8 2'
check 'IFS=2; set ${x-"${#long}"}; IFS=" "; echo $* $#' '128 1'

exit $rval
