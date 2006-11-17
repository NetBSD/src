#! /bin/sh
# $NetBSD: expand.sh,v 1.3 2006/11/17 22:25:46 dsl Exp $
#

#
# This file tests the functions in expand.c.
#

set -e

# usage: assert_equals <testname> <expected> <got>
assert_equals() {
  if test "$2" != "$3"; then
    echo "FAIL in testcase $1: expected [$2], got [$3]" 1>&2
    false
  fi
}

#
# Somewhere between 2.0.2 and 3.0 the expansion of the $@ variable had
# been broken.
#

got=`echo "" "" | sed 's,$,EOL,'`	# This one should work everywhere.
assert_equals "1" " EOL" "$got"

set -- "" ""				# This code triggered the bug.
got=`echo "$@" | sed 's,$,EOL,'`
assert_equals "2" " EOL" "$got"

set -- -
shift
n_arg() { echo $#; }
assert_equals "3" "0" `n_arg "$@"`

line='#define bindir "/usr/bin" /* comment */'
stripped='#define bindir "/usr/bin" '
assert_equals "3" "$stripped" "${line%%/\**}"
