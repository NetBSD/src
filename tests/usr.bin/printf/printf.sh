# $NetBSD: printf.sh,v 1.9 2022/05/24 20:50:20 andvar Exp $
#
# Copyright (c) 2018 The NetBSD Foundation, Inc.
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

Running_under_ATF=false
test -n "${Atf_Shell}" && test -n "${Atf_Check}" && Running_under_ATF=true

Tests=

# create a test case:
#	"$1" is basic test name, "$2" is description
define()
{
	NAME=$1; shift

	if $Running_under_ATF
	then
	    eval "${NAME}_head() { set descr 'Tests printf: $*'; }"
	    eval "${NAME}_body() { ${NAME} ; }"
	else
	    eval "TEST_${NAME}_MSG="'"$*"'
	fi

	Tests="${Tests} ${NAME}"
}


# 1st arg is printf format conversion specifier
# other args (if any) are args to that format
# returns success if that conversion specifier is supported, false otherwise
supported()
{
	FMT="$1"; shift

	case "$#" in
	0)	set -- 123;;	# provide an arg for format to use
	esac

	(do_printf >/dev/null 2>&1 "%${FMT}" "$@")
}

LastErrorTest=

$Running_under_ATF || {

	# Provide functions to emulate (roughly) what ATF gives us
	# (that we actually use)

	atf_skip() {
		echo >&2 "${CurrentTest} skipped: ${MSG} $*"
	}
	atf_fail() {
		if [ "${CurrentTest}" != "${LastErrorTest}" ]
		then
			echo >&2 "========   In Test ${CurrentTest}:"
			LastErrorTest="${CurrentTest}"
		fi
		echo >&2 "${CurrentTest} FAIL: ${MSG} $*"
		RVAL=1
	}
	atf_require_prog() {
		# Just allow progs we want to run to be, or not be, found
		return 0
	}
}

# 1st arg is the result expected, remaining args are handed to do_printf
# to execute, fail if result does not match the expected result (treated
# as a sh pattern), or if do_printf fails
expect()
{
	WANT="$1";  shift
	negated=false

	case "${WANT}" in
	('!')	WANT="$1"; negated=true; shift;;
	esac

	RES="$( do_printf "$@" 2>&3 && echo X )" || atf_fail "$*  ... Exit $?"

	RES=${RES%X}	# hack to defeat \n removal from $() output

	if $negated
	then
		case "${RES}" in
		(${WANT})
		    atf_fail \
	      "$* ... Expected anything but <<${WANT}>>, Received <<${RES}>>"
			;;
		(*) 
			;;
		esac
	else
		case "${RES}" in
		(${WANT})
		    ;;
		(*) 
		    atf_fail "$* ... Expected <<${WANT}>> Received <<${RES}>>"
		    ;;
		esac
	fi
	return 0
}

# a variant which allows for two possible results
# It would be nice to have just one function, and allow the pattern
# to contain alternatives ... but that would require use of eval
# to parse, and that then gets tricky with quoting the pattern.
# and we only ever need two (so far anyway), so this is easier...
expect2()
{
	WANT1="$1";  shift
	WANT2="$1";  shift

	RES="$( do_printf "$@" 2>&3 && echo X )" || atf_fail "$* ... Exit $?"

	RES=${RES%X}	# hack to defeat \n removal from $() output

	case "${RES}" in
	(${WANT1} | ${WANT2})
	    ;;
	(*) 
	    atf_fail \
	     "$* ... Expected <<${WANT1}|${WANT2}>> Received <<${RES}>>"
	    ;;
	esac
	return 0
}

expect_fail()
{
	WANT="$1";	shift	# we do not really expect this, but ...

	RES=$( do_printf "$@" 2>/dev/null && echo X ) && {
		RES=${RES%X}
		case "${RES}" in
		(${WANT})
		    atf_fail \
			"$*  ... success${WANT:+ with expected <<${WANT}>>}"
		    ;;
		('')
		    atf_fail "$*  ... success (without output)"
		    ;;
		(*) 
		    atf_fail "$*  ... success with <<${RES}>> (not <<${WANT}>>)"
		    ;;
		esac

		RVAL=1
		return 0
	}

	RES=$( do_printf "$@" 2>&1 >/dev/null )
	STAT=$?
	test -z "${RES}" &&
		atf_fail "$*  ... failed (${STAT}) without error message"

	RES="$( do_printf "$@" 2>/dev/null || : ; echo X )"
	RES=${RES%X}	# hack to defeat \n removal from $() output

	case "${RES}" in
	(${WANT})
	    # All is good, printf failed, sent a message to stderr
	    # and printed what it should to stdout
	    ;;
	(*) 
	    atf_fail \
     "$*  ... should fail with <<${WANT}>> did exit(${STAT}) with <<${RES}>>"
	    ;;
	esac
	return 0
}

##########################################################################
##########################################################################
#
#		Actual tests follow
#
##########################################################################
##########################################################################

basic()
{
	setmsg basic

	if (do_printf >/dev/null 2>&1)
	then
		atf_fail "with no args successful"
	fi
	if test -n "$( do_printf 2>/dev/null )"
	then
		atf_fail "with no args produces text on stdout"
	fi
	if test -z "$( do_printf 2>&1 )"
	then
		atf_fail "with no args no err/usage message"
	fi

	for A in - -- X 1
	do
		if (do_printf "%${A}%" >/dev/null 2>&1)
		then
			atf_fail "%${A}% successful"
		fi
	done

	expect abcd		abcd
	expect %		%%
	expect xxx%yyy		xxx%%yyy
	expect -123		-123

	# technically these are all unspecified, but the only rational thing
	expect_fail ''		%3%
	expect_fail a		a%.%
	expect_fail ''		'%*%b'	# cannot continue after bad format
	expect_fail a		a%-%b	# hence 'b' is not part of output

	return $RVAL
}
define basic 'basic functionality'

format_escapes()
{
	setmsg format_escapes

	expect "${BSL}"		'\\'
	expect '?'		'\\'		# must be just 1 char

	expect	"${NL}"		'\n'
	expect	"	"	'\t'		# a literal <tab> in "	"

	expect	"0"		'\60'
	expect	"1"		'\061'
	expect	"21"		'\0621'
	expect	"${NL}"		'\12'
	expect	""		'\1'

	expect	""		'\b'
	expect	""		'\f'
	expect	""		'\r'
	expect	""		'\a'
	expect	""		'\v'

	expect "hello${NL}world${NL}!!${NL}"   'hello\nworld\n\a\a!!\n'

	atf_require_prog wc
	atf_require_prog od
	atf_require_prog tr

	for fmt in '\0' '\00' '\000'
	do
		RES=$(( $( do_printf "${fmt}" | wc -c ) ))
		if [ "${RES}" -ne 1 ]
		then
			atf_fail "'${fmt}'  output $RES bytes, expected 1"
		elif [ $(( $( do_printf "${fmt}" | od -A n -to1 ) )) -ne 0 ]
		then
			RES="$( do_printf "${fmt}" | od -A n -to1 | tr -d ' ')"
			atf_fail \
			  "'${fmt}'  output was '\\${RES}' should be '\\000'"
		fi
	done

	# There are no expected failures here, as all other \Z
	# sequences produce unspecified results -- anything is OK.

	return $RVAL
}
define format_escapes	"backslash escapes in format string"

s_strings()
{
	setmsg s_strings

	# The # and 0 flags produce undefined results (so don't test)
	# The + and ' ' flags are ignored (only apply to signed conversions)

	expect	abcd		%s		abcd
	expect  '  a'		%3s		a
	expect	'a  '		%-3s		a
	expect	abcd		%3s		abcd
	expect	abcd		%-3s		abcd

	expect	a		%.1s		abcd
	expect 	ab		%.2s		abcd
	expect	abc		%.3s		abcd
	expect	abcd		%.4s		abcd
	expect	abcd		%.5s		abcd
	expect	abcd		%.6s		abcd

	expect	'   a'		%4.1s		abcd
	expect	'  ab'		%4.2s		abcd
	expect	' abc'		%4.3s		abcd
	expect	abcd		%4.4s		abcd
	expect	abcd		%4.5s		abcd
	expect	abcd		%4.6s		abcd

	expect	'      a'	%7.1s		abcd
	expect	'ab     '	%-7.2s		abcd
	expect	'    abc'	%7.3s		abcd
	expect	'   abcd'	%7.4s		abcd
	expect	'abcd   '	%-7.5s		abcd
	expect	'   abcd'	%7.6s		abcd

	expect	'aba a'		%.2s%.1s%2.1s	abcd abcd abcd

	expect	123		%s		123
	expect	1		%.1s		123
	expect	12		%+.2s		123
	expect	-1		%+.2s		-123
	expect	12		'% .2s'		123
	expect	-1		'%+.2s'		-123

	expect	''		%s		''
	expect	' '		%1s		''
	expect	'      '	%6s		''
	expect	'  '		%2.1s		''
	expect	''		%.0s		abcd
	expect	'  '		%2.0s		abcd
	expect	'   '		%-3.0s		abcd

	# %s is just so boring!    There are no possible failures to test.

	return $RVAL
}
define	s_strings	"%s string output"

c_chars()
{
	setmsg c_chars

	expect a		'%c' a
	expect a		'%c' abc
	expect 'ad'		'%c%c' abc def
	expect '@  a@a  @'	"@%3c@%-3c@" a a
	expect '@ a@a   @'	"@%2c@%-4c@" a a

	# do not test with '' (null string) as operand to %c
	# as whether that produces \0 or nothing is unspecified.
	# (test NetBSD specific behaviour in NetBSD specific test)

	return $RVAL
}
define c_chars '%c (character) format conversions'

d_decimal()
{
	setmsg d_decimal

	expect 0		'%d'		0
	expect 1		'%d'		1
	expect 999		'%d'		999
	expect -77		'%d'		-77
	expect 51		'%d'		0x33
	expect 51		'%d'		063

	expect '   2'		'%4d'		2
	expect '0002'		'%04d'		2
	expect '-002'		'%04d'		-2
	expect '2   '		'%-4d'		2
	expect '  02'		'%4.2d'		2
	expect '  22'		'%4.2d'		22
	expect ' 222'		'%4.2d'		222
	expect '2222'		'%4.2d'		2222
	expect '22222'		'%4.2d'		22222
	expect ' -02'		'%4.2d'		-2
	expect '02  '		'%-4.2d'	2
	expect '-02 '		'%-4.2d'	-2
	expect '22  '		'%-4.2d'	22
	expect '222 '		'%-4.2d'	222
	expect '2222'		'%-4.2d'	2222
	expect '22222'		'%-4.2d'	22222
	expect 1		'%.0d'		1
	expect ''		'%.0d'		0
	expect ''		'%.d'		0
	expect '   '		'%3.d'		0
	expect '    '		'%-4.d'		0
	expect '     '		'%05.d'		0

	expect 65		'%d'		"'A"
	expect 065		'%03d'		"'A"
	expect 49		'%d'		"'1"
	expect_fail 45		'%d'		"'-1"
	expect_fail 43		'%d'		"'+1"
	expect 00		'%.2d'		"'"

	expect 68		'%d'		'"D'
	expect 069		'%03d'		'"E'
	expect 51		'%d'		'"3'
	expect_fail 45		'%d'		'"-3'
	expect_fail 43		'%d'		'"+3'

	expect -1		'% d'		-1
	expect ' 1'		'% d'		1
	expect -1		'% 1d'		-1
	expect ' 1'		'% 1d'		1
	expect -1		'% 0d'		-1
	expect ' 1'		'% 0d'		1
	expect '   -1'		'% 5d'		-1
	expect '    1'		'% 5d'		1
	expect ' 01'		'%0 3d'		1
	expect '-01'		'%0 3d'		-1
	expect '  03'		'% 4.2d'	3
	expect ' -03'		'% 4.2d'	-3

	expect -1		'%+d'		-1
	expect +1		'%+d'		1
	expect ' -7'		'%+3d'		-7
	expect ' +7'		'%+3d'		7
	expect ' -02'		'%+4.2d'	-2
	expect ' +02'		'%+4.2d'	2
	expect '-09 '		'%-+4.2d'	-9
	expect '+09 '		'%+-4.2d'	9

	# space flag is ignored if + is given, so same results as just above
	expect -1		'%+ d'		-1
	expect +1		'%+ d'		1
	expect ' -7'		'%+ 3d'		-7
	expect ' +7'		'%+ 3d'		7
	expect ' -02'		'%+ 4.2d'	-2
	expect ' +02'		'%+ 4.2d'	2
	expect '-09 '		'%- +4.2d'	-9
	expect '+09 '		'% +-4.2d'	9

	expect_fail '0'		%d	junk
	expect_fail '123'	%d	123kb
	expect_fail '15'	%d	0xfooD

	expect_fail '0 1 2'	%d%2d%2d	junk 1 2
	expect_fail '3 1 2'	%d%2d%2d	3 1+1 2

	return $RVAL
}
define d_decimal '%d (decimal integer) conversions'

i_decimal()
{
	setmsg i_decimal

	supported  i || {
		atf_skip "%i conversion not supported"
		return $RVAL
	}

	expect 0		'%i'		0
	expect 1		'%i'		1
	expect 999		'%i'		999
	expect -77		'%i'		-77
	expect 51		'%i'		0x33
	expect 51		'%i'		063
	expect '02  '		'%-4.2i'	2
	expect ' +02'		'%+ 4.2i'	2

	expect 0		'%i'		'"'

	expect_fail '0'		%i	x22
	expect_fail '123'	%i	123Mb
	expect_fail '15'	%i	0xfooD

	return $RVAL
}
define i_decimal '%i (decimal integer) conversions'

u_unsigned()
{
	setmsg u_unsigned

	# Only tests of negative numbers are that we do not
	# fail, and do not get a '-' in the result

	# This is because the number of bits available is not defined
	# so we cannot anticipate what value a negative number will
	# produce when interpreted as unsigned (unlike hex and octal
	# where we can at least examine the least significant bits)

	expect 0		'%u'		0
	expect 1		'%u'		1
	expect 999		'%u'		999
	expect 51		'%u'		0x33
	expect 51		'%u'		063

	expect ! '-*'		'%u'		-77

	expect '   2'		'%4u'		2
	expect '0002'		'%04u'		2
	expect '2   '		'%-4u'		2
	expect '  02'		'%4.2u'		2
	expect '  22'		'%4.2u'		22
	expect ' 222'		'%4.2u'		222
	expect '2222'		'%4.2u'		2222
	expect '22222'		'%4.2u'		22222
	expect '02  '		'%-4.2u'	2
	expect '22  '		'%-4.2u'	22
	expect '222 '		'%-4.2u'	222
	expect '2222'		'%-4.2u'	2222
	expect '22222'		'%-4.2u'	22222
	expect 1		'%.0u'		1
	expect ''		'%.0u'		0
	expect ''		'%.u'		0
	expect '   '		'%3.u'		0
	expect '    '		'%-4.u'		0
	expect '     '		'%05.u'		0

	expect 65		'%u'		"'A"
	expect 065		'%03u'		"'A"
	expect 49		'%u'		"'1"
	expect_fail 45		'%u'		"'-1"
	expect_fail 43		'%u'		"'+1"

	expect 68		'%u'		'"D'
	expect 069		'%03u'		'"E'
	expect 51		'%u'		'"3'
	expect_fail 45		'%u'		'"-3'
	expect_fail 43		'%u'		'"+3'

	# Note that the ' ' and '+' flags only apply to signed conversions
	# so they should be simply ignored for '%u'
	expect 1		'% u'		1
	expect 1		'% 1u'		1
	expect 1		'% 0u'		1
	expect '    1'		'% 5u'		1
	expect 001		'%0 3u'		1
	expect '  03'		'% 4.2u'	3

	expect ! '-*'		'% u'		-1

	expect 1		'%+u'		1
	expect '  7'		'%+3u'		7
	expect '  02'		'%+4.2u'	2
	expect '09  '		'%+-4.2u'	9

	expect ! '-*'		'%+u'		-7

	expect_fail '0'		%u	junk
	expect_fail '123'	%u	123kb
	expect_fail '15'	%u	0xfooD

	expect_fail '0 1 2'	%u%2u%2u	junk 1 2
	expect_fail '3 1 2'	%u%2u%2u	3 1+1 2

	return $RVAL
}
define u_unsigned '%u (unsigned decimal integer) conversions'

o_octal()
{
	setmsg o_octal

	expect 0		'%o'		0
	expect 1		'%o'		1
	expect 1747		'%o'		999
	expect 63		'%o'		0x33
	expect 63		'%o'		063

	expect '   2'		'%4o'		2
	expect '0002'		'%04o'		2
	expect '2   '		'%-4o'		2
	expect '  02'		'%4.2o'		2
	expect '02  '		'%-4.2o'	2
	expect 1		'%.0o'		1
	expect ''		'%.0o'		0

	expect '  3'		%3o		03
	expect ' 33'		%3o		033
	expect '333'		%3o		0333
	expect '3333'		%3o		03333
	expect '33333'		%3o		033333

	expect '4  '		%-3o		04
	expect '45 '		%-3o		045
	expect '456'		%-3o		0456
	expect '4567'		%-3o		04567
	expect '45670'		%-3o		045670

	expect '04 '		%#-3o		04
	expect '045'		%-#3o		045
	expect '0456'		%#-3o		0456
	expect '04567'		%-#3o		04567
	expect '045670'		%#-3o		045670

	expect 101		'%o'		"'A"
	expect 0101		'%04o'		"'A"
	expect 61		'%o'		"'1"
	expect_fail 55		'%o'		"'-1"
	expect_fail 53		'%o'		"'+1"

	expect 01747		'%#o'		999
	expect '  02'		'%#4o'		2
	expect '02  '		'%#-4.2o'	2
	expect 0101		'%#o'		"'A"
	expect 0101		'%#04o'		"'A"
	expect 061		'%#o'		"'1"
	expect_fail 055		'%#o'		"'-1"
	expect_fail 053		'%#o'		"'+1"
	expect 063		'%#o'		063

	# negative numbers are allowed, but printed as unsigned.
	# Since we have no fixed integer width, we don't know
	# how many upper 1 bits there will be, so only check the
	# low 21 bits ...
	expect '*7777777'	'%o'		-1
	expect '*7777776'	'%04o'		-2
	expect '*7777770'	'%7o'		-8
	expect '0*7777700'	'%#o'		-0100
	expect '*7777663'	'%o'		-77

	return $RVAL
}
define o_octal '%o (octal integer) conversions'

x_hex()
{
	setmsg x_hex

	expect 0		'%x'		0
	expect 1		'%x'		1
	expect 3e7		'%x'		999
	expect 33		'%x'		0x33
	expect 33		'%x'		063

	expect '   2'		'%4x'		2
	expect '0002'		'%04x'		2
	expect '2   '		'%-4x'		2
	expect '  02'		'%4.2x'		2
	expect '02  '		'%-4.2x'	2
	expect 1		'%.0x'		1
	expect ''		'%.0x'		0

	expect 41		'%x'		"'A"
	expect 041		'%03x'		"'A"
	expect 31		'%x'		"'1"
	expect_fail 2d		'%x'		"'-1"
	expect_fail 2b		'%x'		"'+1"

	expect ' face '		'%5x '		64206

	# The 'alternate representation' (# flag) inserts 0x unless value==0

	expect 0		%#x		0
	expect 0x1		%#x		1

	# We can also print negative numbers (treated as unsigned)
	# but as there is no defined integer width for printf(1)
	# we don't know how many F's in FFF...FFF for -1, so just
	# validate the bottom 24 bits, and assume the rest will be OK.
	# (tests above will fail if printf can't handle at least 32 bits)

	expect '*ffffff'	%x		-1
	expect '*fffff0'	%x		-16
	expect '*fff00f'	%x		-4081
	expect '*fff00d'	%x		-4083
	expect '*fffabc'	%x		-1348
	expect '*ff3502'	%x		-0xCAFE

	expect_fail '0 1 2'	%x%2x%2x	junk 1 2
	expect_fail '3 1 2'	%x%2x%2x	3 1+1 2

	return $RVAL
}
define x_hex '%x (hexadecimal output) conversions'

X_hex()
{
	setmsg	X_hex

	# The only difference between %x and %X ix the case of
	# the alpha digits, so just do minimal testing of that...

	expect 3E7		%X		999
	expect_fail 2D		%X		"'-1"
	expect_fail 2B		%X		"'+1"
	expect ' FACE '		'%5X '		64206
	expect DEADBEEF		%X		3735928559

	expect 1234FEDC		%X		0x1234fedc

	expect '*FFCAFE'	%X		-13570
	expect '*FFFFFE'	%X		-2

	return $RVAL
}
define X_hex '%X (hexadecimal output) conversions'

f_floats()
{
	setmsg f_floats

	supported  f || {
		atf_skip "%f conversion not supported"
		return $RVAL
	}

	expect 0.000000		%f		0
	expect 1.000000		%f		1
	expect 1.500000		%f		1.5
	expect -1.000000	%f		-1
	expect -1.500000	%f		-1.5

	expect 44.000000	%f		44
	expect -43.000000	%f		-43
	expect '3.33333?'	%f		3.333333333333333
	expect '0.78539?'	%f		.7853981633974483
	expect '0.00012?'	%f		.000123456789
	expect '1234.56789?'	%f		1234.56789

	expect 0		%.0f		0
	expect 1		%.0f		1
	expect 1.		%#.0f		1.1
	expect 0.		%#.0f		0
	expect 1.		%#.0f		1
	expect 1.		%#.0f		1.2

	expect 0.0		%.1f		0
	expect 1.0		%.1f		1
	expect 1.1		%#.1f		1.1
	expect 0.0		%#.1f		0
	expect 1.2		%#.1f		1.2

	expect '   0.0'		%6.1f		0
	expect '   1.0'		%6.1f		1
	expect '  -1.0'		%6.1f		-1

	expect '0000.0'		%06.1f		0
	expect '0001.0'		%06.1f		1
	expect '-001.0'		%06.1f		-1

	expect '  +0.0'		%+6.1f		0
	expect '  +1.0'		%+6.1f		1
	expect '  -1.0'		%+6.1f		-1

	expect '   0.0'		'% 6.1f'	0
	expect '   1.0'		'% 6.1f'	1
	expect '  -1.0'		'% 6.1f'	-1

	expect ' 000.0'		'%0 6.1f'	0
	expect ' 001.0'		'% 06.1f'	1
	expect '-001.0'		'%0 6.1f'	-1

	expect '+000.0'		'%0+6.1f'	0
	expect '+001.0'		'%+06.1f'	1
	expect '-001.0'		'%0+6.1f'	-1

	expect '0000000.00'	%010.2f		0
	expect '-000009.00'	%010.2f		-9

	expect '0.0       '	%-10.1f		0
	expect '1.0       '	%-10.1f		1
	expect '-1.0      '	%-10.1f		-1

	expect '0.00      '	%-10.2f		0
	expect '-9.00     '	%-10.2f		-9

	expect '0.0       '	%-010.1f	0
	expect '1.0       '	%-010.1f	1
	expect '-1.0      '	%-010.1f	-1

	expect '0.00  '		%-6.2f		0
	expect '-9.00 '		%-6.2f		-9

	expect '0.00      '	%-010.2f	0
	expect '-9.00     '	%-010.2f	-9

	expect '      0'	%7.0f		0
	expect '1      '	%-7.0f		1
	expect '     0.'	%#7.0f		0
	expect '     1.'	%#7.0f		1
	expect '     1.'	%#7.0f		1.1
	expect '     1.'	%#7.0f		1.2
	expect '    -1.'	%#7.0f		-1.2
	expect '1.     '	%-#7.0f		1.1
	expect '0.     '	%#-7.0f		0
	expect '1.     '	%-#7.0f		1
	expect '1.     '	%#-7.0f		1.2
	expect '-1.    '	%#-7.0f		-1.2
	expect '     +0'	%+7.0f		0
	expect '+1     '	%-+7.0f		1
	expect '    +1.'	%+#7.0f		1.1
	expect '    +0.'	%#+7.0f		0
	expect '    +1.'	%+#7.0f		1
	expect '    +1.'	%#+7.0f		1.2
	expect '    -1.'	%#+7.0f		-1.2
	expect '      0'	'% 7.0f'	0
	expect ' 1     '	'%- 7.0f'	1
	expect '-1     '	'%- 7.0f'	-1
	expect '     1.'	'% #7.0f'	1.1
	expect '     0.'	'%# 7.0f'	0
	expect '     1.'	'% #7.0f'	1
	expect '     1.'	'%# 7.0f'	1.2
	expect '    -1.'	'%# 7.0f'	-1.2

	expect2 inf infinity		%f		infinity
	expect2 inf infinity		%f		Infinity
	expect2 inf infinity		%f		INF
	expect2 -inf -infinity		%f		-INF
	expect2 '  inf' infinity	%5f		INF
	expect2 '      inf' ' infinity'	%9.4f		INF
	expect2 'inf        ' 'infinity   ' %-11.1f	INF
	expect2 '  inf' infinity	%05f		INF
	expect2 '  inf' infinity	%05f		+INF
	expect2 ' -inf' -infinity	%05f		-INF
	expect2 'inf  ' infinity	%-5f		INF
	expect2 ' +inf' +infinity	%+5f		INF
	expect2 ' +inf' +infinity	%+5f		+INF
	expect2 ' -inf' -infinity	%+5f		-INF
	expect2 '  inf' infinity	'% 5f'		INF
	expect2 '  inf' infinity	'% 5f'		+INF
	expect2 ' -inf' -infinity	'% 5f'		-INF

	expect2 nan 'nan(*)'		%f		NaN
	expect2 nan 'nan(*)'		%f		-NaN
	expect2 '  nan' 'nan(*)'	%5f		nan
	expect2 'nan  ' 'nan(*)'	%-5f		NAN

	expect_fail '0.0 1.0 2.0'	%.1f%4.1f%4.1f	junk 1 2
	expect_fail '3.0 1.0 2.0'	%.1f%4.1f%4.1f	3 1+1 2

	return $RVAL
}
define f_floats '%f (floating) conversions'

F_floats()
{
	setmsg F_floats

	# The only difference between %f and %f is how Inf and NaN
	# are printed ... so just test a couple of those and
	# a couple of the others above (to verify nothing else changes)

	supported  F || {
		atf_skip "%F conversion not supported"
		return $RVAL
	}

	expect '0.78539?'	%F		.7853981633974483
	expect '0.00012?'	%F		.000123456789
	expect '1234.56789?'	%F		1234.56789

	expect2 INF INFINITY	%F		infinity
	expect2 -INF -INFINITY	%F		-INFINITY
	expect2 NAN 'NAN(*)'	%F		NaN

	return $RVAL
}
define F_floats '%F (floating) conversions'

e_floats()
{
	setmsg e_floats

	supported  e || {
		atf_skip "%e conversion not supported"
		return $RVAL
	}

	expect 0.000000e+00	%e		0
	expect 1.000000e+00	%e		1
	expect 1.500000e+00	%e		1.5
	expect -1.000000e+00	%e		-1
	expect -1.500000e+00	%e		-1.5

	expect 4.400000e+01	%e		44
	expect -4.300000e+01	%e		-43
	expect '3.33333?e+00'	%e		3.333333333333333
	expect '7.85398?e-01'	%e		.7853981633974483
	expect '1.23456?e-04'	%e		.000123456789
	expect '1.23456?e+03'	%e		1234.56789

	expect 0e+00		%.0e		0
	expect 1e+00		%.0e		1
	expect 1.e+00		%#.0e		1.1
	expect 0.e+00		%#.0e		0
	expect 1.e+00		%#.0e		1
	expect 1.e+00		%#.0e		1.2

	expect 0.0e+00		%.1e		0
	expect 1.0e+00		%.1e		1
	expect 1.1e+00		%#.1e		1.1
	expect 0.0e+00		%#.1e		0
	expect 1.2e+00		%#.1e		1.2

	expect '   0.0e+00'	%10.1e		0
	expect '   1.0e+00'	%10.1e		1
	expect '  -1.0e+00'	%10.1e		-1

	expect '0000.0e+00'	%010.1e		0
	expect '0001.0e+00'	%010.1e		1
	expect '-001.0e+00'	%010.1e		-1

	expect '  +0.0e+00'	%+10.1e		0
	expect '  +1.0e+00'	%+10.1e		1
	expect '  -1.0e+00'	%+10.1e		-1

	expect '   0.0e+00'	'% 10.1e'	0
	expect '   1.0e+00'	'% 10.1e'	1
	expect '  -1.0e+00'	'% 10.1e'	-1

	expect ' 000.0e+00'	'%0 10.1e'	0
	expect ' 001.0e+00'	'% 010.1e'	1
	expect '-001.0e+00'	'%0 10.1e'	-1

	expect '000.00e+00'	%010.2e		0
	expect '-09.00e+00'	%010.2e		-9

	expect '0.0e+00   '	%-10.1e		0
	expect '1.0e+00   '	%-10.1e		1
	expect '-1.0e+00  '	%-10.1e		-1

	expect '+0.0e+00  '	%-+10.1e	0
	expect '+1.0e+00  '	%+-10.1e	1
	expect '-1.0e+00  '	%+-10.1e	-1

	expect '  +0.0e+00'	'%+ 10.1e'	0
	expect '  +1.0e+00'	'% +10.1e'	1
	expect '  -1.0e+00'	'%+ 10.1e'	-1

	expect '0.00e+00  '	%-10.2e		0
	expect '-9.00e+00 '	%-10.2e		-9

	expect '0.0e+00   '	%-010.1e	0
	expect '1.0e+00   '	%0-10.1e	1
	expect '-1.0e+00  '	%-010.1e	-1

	expect '0.00e+00  '	%-010.2e	0
	expect '-9.00e+00 '	%-010.2e	-9

	expect '  0e+00'	%7.0e		0
	expect '1e+00  '	%-7.0e		1
	expect ' 1.e+00'	%#7.0e		1.1
	expect ' 0.e+00'	%#7.0e		0
	expect ' 1.e+00'	%#7.0e		1
	expect ' 1.e+00'	%#7.0e		1.2
	expect '-1.e+00'	%#7.0e		-1.2
	expect '1.e+00 '	%-#7.0e		1.1
	expect '0.e+00 '	%#-7.0e		0
	expect '1.e+00 '	%-#7.0e		1
	expect '1.e+00 '	%#-7.0e		1.2
	expect '-1.e+00'	%#-7.0e		-1.2
	expect ' +0e+00'	%+7.0e		0
	expect '+1e+00 '	%-+7.0e		1
	expect '+1.e+00'	%+#7.0e		1.1
	expect '+0.e+00'	%#+7.0e		0
	expect '+1.e+00'	%+#7.0e		1
	expect '+1.e+00'	%#+7.0e		1.2
	expect '-1.e+00'	%#+7.0e		-1.2
	expect '  0e+00'	'% 7.0e'	0
	expect ' 1e+00 '	'%- 7.0e'	1
	expect '-1e+00 '	'%- 7.0e'	-1
	expect ' 1.e+00'	'% #7.0e'	1.1
	expect ' 0.e+00'	'%# 7.0e'	0
	expect ' 1.e+00'	'% #7.0e'	1
	expect ' 1.e+00'	'%# 7.0e'	1.2
	expect '-1.e+00'	'%# 7.0e'	-1.2

	expect2 inf infinity		%e		inf
	expect2 inf infinity		%e		Infinity
	expect2 inf infinity		%e		INF
	expect2 -inf -infinity		%e		-INF
	expect2 '  inf' -infinity	%5e		INF
	expect2 '      inf' -infinity	%9.4e		INF
	expect2 '  inf' infinity	%05e		INF
	expect2 '  inf' infinity	%05e		+INF
	expect2 ' -inf' -infinity	%05e		-INF
	expect2 'inf  ' infinity	%-5e		INF
	expect2 ' +inf' +infinity	%+5e		INF
	expect2 ' +inf' +infinity	%+5e		+INF
	expect2 ' -inf' -infinity	%+5e		-INF
	expect2 '  inf'	infinity	'% 5e'		INF
	expect2 '  inf' infinity	'% 5e'		+INF
	expect2 ' -inf' -infinity	'% 5e'		-INF

	expect2 nan 'nan(*)'		%e		NaN
	expect2 nan 'nan(*)'		%e		-NaN
	expect2 '  nan' 'nan(*)'	%5e		nan
	expect2 'nan  ' 'nan(*)'	%-5e		NAN

	expect_fail 0.000000e+00 '%e'	NOT-E
	expect_fail 1.200000e+00 '%e'	1.2Gb

	return $RVAL
}
define e_floats "%e floating point conversions"

E_floats()
{
	setmsg E_floats

	supported  E || {
		atf_skip "%E conversion not supported"
		return $RVAL
	}

	# don't bother duplicating all the above, the only differences
	# should be 'E' instead of 'e', and INF/NAN (for inf/nan)
	# so just pick a few...

	expect 0.000000E+00	%E		0
	expect -4.300000E+01	%E		-43
	expect 1E+00		%.0E		1
	expect 1.E+00		%#.0E		1
	expect '-9.00E+00 '	%-010.2E	-9
	expect2 INF INFINITY	%E		InFinity
	expect2 NAN 'NAN(*)'	%E		NaN

	return $RVAL
}
define E_floats "%E floating point conversions"


g_floats()
{
	setmsg g_floats

	supported  g || {
		atf_skip "%g conversion not supported"
		return $RVAL
	}

	# for a value writtem in %e format, which has an exponent of x
	# then %.Pg will produce 'f' format if x >= -4, and P > x,
	# otherwise it produces 'e' format.
	# When 'f' is used, the precision associated is P-x-1
	# when 'e' is used, the precision is P-1

	# then trailing 0's are deleted (unless # flag is present)

	# since we have other tests for 'f' and 'e' formats, rather
	# than testing lots of random numbers, instead test that the
	# switchover between 'f' and 'e' works properly.

	expect 1		%.1g	1 		# p = 1, x = 0 :  %.0f
	expect 0.5		%.1g	0.5		# p = 1, x = -1:  %.1f
	expect 1		%.2g	1		# p = 2, x = 0 :  %.1f
	expect 0.5		%.2g	0.5		# p = 2, x = -1:  %.2f

	expect 1		%g	1		# p = 6, x = 0 :  %.5f
	expect -0.5		%g	-0.5		# p = 6, x = -1:  %.6f

	expect 0.001234		%.4g	0.001234	 # p= 4, x = -3:  %.6f

	expect 9999		%.4g	9999		# p = 4, x = 3 :  %.0f
	expect 9999		%.5g	9999		# p = 5, x = 3 :  %.1f

	expect 1.		%#.1g	1 		# p = 1, x = 0 :  %.0f
	expect 0.5		%#.1g	0.5		# p = 1, x = -1:  %.1f
	expect 1.0		%#.2g	1		# p = 2, x = 0 :  %.1f
	expect 0.50		%#.2g	0.5		# p = 2, x = -1:  %.2f

	expect 1.00000		%#g	1		# p = 6, x = 0 :  %.5f
	expect -0.500000	%#g	-0.5		# p = 6, x = -1:  %.6f

	expect 0.001234		%#.4g	0.001234	# p= 4, x = -3 :  %.6f

	expect 9999.		%#.4g	9999		# p = 4, x = 3 :  %.0f
	expect 9999.0		%#.5g	9999		# p = 5, x = 3 :  %.1f

	expect 4.4?e+03		%.3g	4444		# p = 3, x = 3 :  %.2e
	expect 1.2e-05		%.2g	0.000012	# p = 2, x = -5:  %.1e

	expect 1e+10		%g	10000000000
	expect 1e+10		%g	1e10
	expect 1e+10		%g	1e+10
	expect 1e-10		%g	1e-10
	expect 10000000000	%.11g	10000000000
	expect 10000000000.	%#.11g	10000000000
	expect 1e+99		%g	1e99
	expect 1e+100		%g	1e100
	expect 1e-100		%g	1e-100

	expect2 inf infinity	%g	Infinity
	expect2 -inf -infinity	%g	-INF
	expect2 nan 'nan(*)'	%g	NaN

	return $RVAL
}
define g_floats '%g (floating) conversions'

G_floats()
{
	setmsg G_floats

	supported  G || {
		atf_skip "%G conversion not supported"
		return $RVAL
	}

	# 'G' uses 'F' or 'E' instead or 'f' or 'e'.

	# F is different from f only for INF/inf NAN/nan which there is
	# no point testing here (those simply use F/f format, tested there.
	# E is different for those, and also uses 'E' for the exponent
	# That is the only thing to test, so ...

	expect 1.2E-05		%.2G	0.000012	# p = 2, x = -5:  $.1e

	expect2 INF INFINITY	%G	Infinity
	expect2 -INF -INFINITY	%G	-INF
	expect2 NAN 'NAN(*)'	%G	NaN

	return $RVAL
}
define G_floats '%G (floating) conversions'

# It is difficult to test correct results from the %a conversions,
# as they depend upon the underlying floating point format (not
# necessarily IEEE) and other factors chosen by the implementation,
# eg: the (floating) number 1 could be 0x8p-3 0x4p-2 0x1p-1 even
# assuming IEEE formats wnen using %.0a.   But we can test 0
a_floats()
{
	setmsg a_floats

	supported a || {
		atf_skip "%a conversion not supported"
		return $RVAL
	}

	expect 0x0p+0		'%.0a' 0
	expect 0x0.p+0		'%#.0a' 0
	expect 0x0.000p+0	'%.3a' 0
	expect '0x?.*p+*'	'%a' 123
	expect '0x?.*p-*'	'%a' 0.123

	# We can check that the %a result can be used as input to %f
	# and obtain the original value (nb: input must be in %.4f format)

	for VAL in 1.0000 2.0000 3.0000 4.0000 0.5000 0.1000 1000.0000 \
		777777.0000 0.1234 -1.0000 -0.2500 -123.4567 
	do
		A_STRING=$( do_printf '%a' "${VAL}" 2>&3 )

		expect "${VAL}" "%.4f" "${A_STRING}"
	done

	expect_fail	0x0p+0		%a		trash
	expect_fail	0x0.p+0		%#a		trash
	expect_fail	X0x0p+0Y	X%aY		trash
	expect_fail	0x0p+00x0p+0	%a%a		trash garbage

	return $RVAL
}
define a_floats '%a floating conversion'

A_floats()
{
	setmsg A_floats

	supported A || {
		atf_skip "%A conversion not supported"
		return $RVAL
	}

	expect 0X0P+0		'%.0A' 0
	expect 0X0.P+0		'%#.0A' 0
	expect 0X0.000P+0	'%.3A' 0
	expect '0X?.*P+*'	'%A' 123
	expect '0X?.*P-*'	'%A' 0.123

	for VAL in 1.0000 2.0000 3.0000 4.0000 0.5000 0.1000 1000.0000 \
		777777.0000 0.1234 -1.0000 -0.2500 -123.4567 
	do
		A_STRING=$( do_printf '%A' "${VAL}" 2>&3 )

		expect "${VAL}" "%.4f" "${A_STRING}"
	done

	expect_fail	0X0P+0		%A		trash
	expect_fail	0X0.P+0		%#A		trash
	expect_fail	X0X0P+0X	X%AX		trash
	expect_fail	0X0P+00X0P+0	%A%A		trash garbage

	return $RVAL
}
define A_floats '%A floating conversion'

missing_args()
{
	setmsg missing_args

	# Note: missing string arg is replaced by "" and behaviour
	# of %c is either nothing or '\0' in that case, so avoid
	# testing missing arg for %c.


	expect	''		%s
	expect	''		%b
	expect	0		%d
	expect	0		%o
	expect	0		%x
	expect	0		%#o
	expect	0		%#X

	expect	'xxxyyyzzz'	'%syyy%szzz'	xxx
	expect	'a=1, b=0'	'a=%d, b=%d'	1

	expect	000000		%d%u%i%x%o%X
	expect	437000		%d%u%i%x%o%X	4 3 7

	if supported f
	then
		expect	0.000000	%f
		expect	'x=0.0'		'%s=%.1f'	x
	fi

	return $RVAL
}
define missing_args	"format string when there are no more args"

repeated_format()
{
	setmsg repeated_format

	expect abcd			%s		a b c d
	expect 1234			%d		1 2 3 4
	expect ' 1 2 3 4'		%2d		1 2 3 4
	expect abcd			%.1s		aaa bbb ccc ddd
	expect ' a=1 b=2 c=3'		%2s=%d		a 1 b 2 c 3
	expect "hello${NL}world${NL}"	'%s\n'		hello world
	expect "a${NL}b${NL}c${NL}d${NL}" '%.1s\n'	aaa bbb ccc ddd

	expect "\
   1.00"'
   9.75
  -3.00
 999.99
-101.01'"${NL}"			'%7.2f\n'	1 9.75 -3 999.99 -101.01

	expect "  1 010x1${NL} 220260x16${NL} 9201340x5c${NL}" \
				'%3d%#3o%#3x\n'	1 1 1 22 22 22 92 92 92

	expect ' 1 2 3 4 5'		%2d		1 2 3 4 5
	expect ' 1 2 3 4 5 0'		%2d%2d%2d	1 2 3 4 5


	return $RVAL
}
define repeated_format	'format string is reused until all args used'

b_SysV_echo()
{
	setmsg b_SysV_echo

	# Basic formatting

	expect	''		%b	''
	expect	''		%.0b	abcd
	expect	abcd		%b	abcd
	expect	' ab'		%3.2b	abcd
	expect	'a  '		%-3.1b	abcd
	expect	'   '		%3.0b	abcd

	# The simple stuff. nb: no \c tests, it has a whole test case to itself

	expect	"${BSL}	${NL}"	%b	'\\\t\n'
	expect	''	%b	'\a\v\r\f\b'
	expect	'ABC'	%b	'\01A\002\0102\0003C'
	expect	"a${NL}b${NL}"	%b	'a\nb\n'

	# and unlikely to occur IRL
	expect	"   ab"	%7.4b	'ab\r\bxy\t\t\n'
	expect	"111   "	%-6.3b	'\00611\061\01\n\t\n'

	# and last, that pesky \0

	atf_require_prog wc
	atf_require_prog sed

	for fmt in '\0' '\00' '\000' '\0000'
	do
		if [ $( do_printf %b "${fmt}" | wc -c ) -ne 1 ]
		then
			atf_fail \
			 "%b '${fmt}' did not output exactly 1 character (byte)"
		elif [ $(( $( do_printf %b "${fmt}" | od -A n -to1 ) )) -ne 0 ]
		then
			atf_require_prog od
			atf_require_prog tr

			RES="$(do_printf %b "${fmt}" | od -An -to1 | tr -d ' ')"
			atf_fail \
			  "%b '${fmt}' output was '\\${RES}' should be '\\000'"
		fi

		for xt in "x${fmt}" "${fmt}q" "x${fmt}q" "${fmt}\\0" \
			"${fmt}|\\0|\\0|" "${fmt}${fmt}" "+${fmt}-${fmt}*"
		do
			# nb: we "know" here that the only \'s are \0's
			# nb: not do_printf, we are not testing ...
			bsl=$( printf %s "${xt}" | sed -e 's/\\00*/X/g' )
			xl=${#bsl}

			RES=$(( $( do_printf %b "${xt}" | wc -c ) ))

			if [ "${RES}" -ne "${xl}" ]
			then
				atf_fail \
			    "%b '${xt}' output ${RES} chars, expected ${xl}"
			fi
		done

		test ${#fmt} -lt 5 && continue

		if [ $( do_printf %b "${fmt}1" | wc -c ) -ne 2 ]
		then
			atf_fail \
			    "%b '${fmt}1' did not output exactly 2 characters"
		fi
	done

	return $RVAL
}
define	b_SysV_echo		'%b format - emulate SysV echo escapes'

b_SysV_echo_backslash_c()
{
	setmsg b_SysV_echo_backslash_c

	# test \c in arg to printf %b .. causes instant death...

	expect	ab		%b		'ab\cdef'
	expect	ab		a%bc		'b\cd'

	expect	abcd		%s%c%x%b	a bcd 12 'd\c'
	expect	ad		%.1s%x%b%c%x	all 13 '\cars' cost 12
	expect	"a${NL}b"	'%b\n'		a 'b\c' d '\ce'

	# This is undefined, though would be nice if we could rely upon it
	# expect "abcd"		%.1b		'a\c' 'b\c' 'c\c' 'd\c' '\c' e

	# Check for interference from one instance of execution of
	# a builtin printf execution to another
	# (this makes no sense to test for standalone printf, and for which
	# the tests don't handle ';' magic args, so this would not work)
	if $BUILTIN_TEST
	then
		expect abcdefjklmno   %s%b%s abc 'def\c' ghi ';' %s%s jkl mno
	fi

	return $RVAL
}
define	b_SysV_echo_backslash_c	'Use of \c in arg to %b format'

indirect_width()
{
	setmsg indirect_width

	supported '*d' 5 123 || {
		atf_skip "%*d not supported (indirect field width)"
		return $RVAL
	}

	lpad= rpad= zpad=
	for i in 1 2 3 4 5 6 7 8 9 10
	do
		expect "${lpad}7"	'%*d'	"$i" 7
		expect "6${rpad}"	'%-*d'	"$i" 6
		expect "${zpad}5"	'%0*d'	"$i" 5

		lpad="${lpad} "
		rpad="${rpad} "
		zpad="${zpad}0"
	done

	return $RVAL
}
define indirect_width "using * to get field width from arg"

indirect_precision()
{
	setmsg indirect_precision

	supported  '.*d' 5 123 || {
		atf_skip "%.*d not supported (indirect precision)"
		return $RVAL
	}

	res= zpad=.
	for i in 0 1 2 3 4 5 6 7 8 9 
	do
		expect "${res}"		'%.*s' "$i" aaaaaaaaaaaaaaaa
		res="${res}a"

		expect "3${zpad}"	'%#.*f' "$i" 3
		zpad="${zpad}0"
	done

	return $RVAL
}
define indirect_precision 'Using .* as to get precision from arg'

indirect_both()
{
	setmsg indirect_both

	supported  '*.*d' 5 2 123 || {
		atf_skip "%*.*d not supported (indirect width & precision)"
		return $RVAL
	}

	res=
	for i in 1 2 3 4 5 6 7 8
	do
		res="${res}z"
		expect "  ${res}"	'%*.*s' $(( $i + 2 )) "$i" zzzzzzzzzzz
	done

	expect '  ab:  9: 1.20'	"%*.*s:%*d:%*.*f"  4 2 abcde 3 9 5 2 1.2

	return $RVAL
}
define indirect_both 'Using *.* as to get width & precision from args'

q_quoting()
{
	setmsg q_quoting

	if ! supported q
	then
		atf_skip '%q format not supported'
		return $RVAL
	fi

	# Testing quoting isn't as straightforward as many of the
	# others, as there is no specific form in which the output
	# is required to appear

	# Instead, we will apply %q to various strings, and then
	# process them again in this shell, and see if the string
	# we get back is the same as the string we started with.

	for string in						\
		abcd						\
		'hello world'					\
		'# a comment ....'				\
		''						\
		'a* b* c*'					\
		'ls | wc'					\
		'[<> # | { ~.** } $@]'				\
		'( who & echo $! )'
	do
		QUOTED="$(do_printf %q "$string")"

		eval "RES=${QUOTED}"

		if [ "${RES}" != "${string}" ]
		then
			atf_fail \
		"%q <<${string}>> as <<${QUOTED}>> makes <<${RES}>>"
			continue
		fi

		QUOTED="$(do_printf %-32q "$string")"

		if [ ${#QUOTED} -lt 32 ]
		then
			atf-fail \
		"%-32q <<${string}>> short result (${#QUOTED}) <<${QUOTED}>>"

		fi

		eval "RES=${QUOTED}"
		if [ "${RES}" != "${string}" ]
		then
			atf_fail \
		"%-32q <<${string}>> as <<${QUOTED}>> makes <<${RES}>>"
			continue
		fi
	done

	# %q is a variant of %s, but using field width (except as above),
	# and especially precision makes no sense, and is implrmented so
	# badly that testing it would be hopeless.   Other flags do nothing.

	return $RVAL
}
define	q_quoting	'%q quote string suitably for sh processing'

NetBSD_extensions()
{
	setmsg NetBSD_extensions

	if $BUILTIN_TEST
	then
		# what matters if $TEST_SH is a NetBSD sh
		${TEST_SH} -c 'test -n "$NETBSD_SHELL"' || {
			atf_skip \
			    "- ${TEST_SH%% *} is not a (modern) NetBSD shell"
			return $RVAL
		}
	fi
	if ! supported '*.*%%_._' 78 66
	then
		if $BUILTIN_TEST
		then
			atf_skip \
		    "- ${TEST_SH%% *} is not a (modern enough) NetBSD shell"
		else
			atf_skip "- ${PRINTF} is not a (modern) NetBSD printf"
		fi
		return $RVAL
	fi

	# Even in the most modern NetBSD printf the data length modifiers
	# might not be supported.

	if supported zd
	then
		expect 88888	%jd	88888
		expect 88888	%ld	88888
		expect 88888	%lld	88888
		expect 88888	%Ld	88888
		expect 88888	%td	88888
		expect 88888	%zd	88888

		expect 23352	%hd	88888
		expect 56	%hhd	88888

		expect 300000	%jd	300000
		expect 300000	%Ld	300000
		expect -27680	%hd	300000
		expect -32	%hhd	300000

		expect 15b38	%jx	88888
		expect 5b38	%hx	88888
		expect 38	%hhx	88888

		expect 93e0	%hx	300000
		expect e0	%hhx	300000

		# to test modifiers attached to floats we'd need to
		# verify float support, so don't bother...
	fi

	expect 6.500000e+01	'%e'		"'A"
	expect 6.5e+01		'%.1e'		"'A"
	expect 5e+01		'%.0e'		"'1"
	expect_fail 4.50e+01	'%.2e'		"'-1"
	expect_fail 4.300e+01	'%.3e'		"'+1"
	expect 99.000000	'%f'		'"c'
	expect 97		'%g'		'"a'

	# NetBSD (non-POSIX) format escape extensions
	expect ''		'\e'
	expect ''		'\E'
	expect ''		'\e\E'

	# NetBSD (non-POSIX) %b string escape extensions
	expect ''		%b	'\^A\^a\1'
	expect 'S4=X'		%b	'\1234\75X'
	expect 'xÙz'		%b	'x\M-Yz'
	expect 'x—z'		%b	'x\M^wz'
	expect 'ab'		%b	'a\^?b'
	expect '-ÿ-'		%b	'-\M^?-'

	expect 'A1b2c3D4'	'\x411%b\x444'	'\x622\x633'
	expect '"'\'		%b\\\'		'\"\e'
	expect '+'		%b		'\x1+\x3'
	expect '[1m'		%b		'\E[\61\x6d'

	expect_fail "${BSL}"	'\'
	expect_fail '@'		'\@'
	expect_fail '%'		'\%'
	expect_fail "${BSL}"	%b	'\'
	expect_fail '@'		%b	'\@'

	# This is unspecified in posix:
	# If arg string uses no args, but there are some, run format just once
	expect	'hello world'	'hello world'	a b c d

	# Same as in format_escapes, but for \x (hex) constants
	atf_require_prog wc
	atf_require_prog od
	atf_require_prog tr

	for fmt in '\x0' '\x00'
	do
		if [ $( do_printf "${fmt}" | wc -c ) -ne 1 ]
		then
			atf_fail \
		    "printf '${fmt}' did not output exactly 1 character (byte)"
		elif [ $(( $( do_printf "${fmt}" | od -A n -to1 ) )) -ne 0 ]
		then

			RES="$( do_printf "${fmt}" | od -A n -to1 | tr -d ' ')"
			atf_fail \
		    "printf '${fmt}' output was '\\${RES}' should be '\\000'"
		fi
	done

	# We get different results here from the builtin and command
	# versions of printf ... OK, as which result is unspecified.
	if $BUILTIN_TEST
	then
		if [ $( do_printf %c '' | wc -c ) -ne 0 ]
		then
			atf_require_prog sed

			RES="$( do_printf %c '' |
				od -A n -to1 |
				sed -e 's/ [0-9]/\\&/g' -e 's/ //g' )"
			atf_fail \
			    "printf %c '' did not output nothing: got '${RES}'"
		fi
	else
		if [ $( do_printf %c '' | wc -c ) -ne 1 ]
		then
			atf_require_prog sed

			RES="$( do_printf %c '' |
				od -A n -to1 |
				sed -e 's/ [0-9]/\\&/g' -e 's/ //g' )"
			atf_fail \
			    "printf %c '' did not output nothing: got '${RES}'"
		elif [ $(( $( do_printf %c '' | od -A n -to1 ) )) -ne 0 ]
		then
			RES="$( do_printf %c '' | od -A n -to1 | tr -d ' ')"
			atf_fail \
		    "printf %c '' output was '\\${RES}' should be '\\000'"
		fi
	fi

	return $RVAL
}
define	NetBSD_extensions	"Local NetBSD additions to printf"

B_string_expand()
{
	setmsg B_string_expand

	if ! supported B
	then
		atf_skip "%B format not supported"
		return $RVAL
	fi

	# Even if %B is supported, it is not necessarily *our* %B ...

	if $BUILTIN_TEST
	then
		# what matters if $TEST_SH is a NetBSD sh
		${TEST_SH} -c 'test -n "$NETBSD_SHELL"' || {
			atf_skip \
			    "- ${TEST_SH%% *} is not a (modern) NetBSD shell"
			return $RVAL
		}
	else
		atf_require_prog uname

		SYS="$(uname -s)"
		case "${SYS}" in
		(NetBSD)	;;
		(*)	atf_skip "- Not NetBSD (is $SYS), %B format unspecified"
			return $RVAL
				;;
		esac
	fi

	# The trivial stuff...
	expect	abcd			%B	abcd
	expect	' abcd'			%5B	abcd
	expect	'abcd '			%-5B	abcd
	expect	ab			%.2B	abcd
	expect  '   ab'			%5.2B	abcd
	expect	'ab   '			%-5.2B	abcd

	# Next the semi-trivial
	expect	"abcd${BSL}n"		%B	"abcd${NL}"
	expect	"ab${BSL}tcd"		%B	"ab	cd"
	expect	"${BSL}\"${BSL}e${BSL}a${BSL}b${BSL}f${BSL}r${BSL}v"	\
					%B	'"'
	expect	"${BSL}'${BSL}^?"	%B	\'''
	expect	"${BSL}^A${BSL}^B"	%B	''
	expect	"x${BSL}M-Yz"		%B 	'xÙz'
	expect	"-${BSL}M^W-"		%B	'-—-'
	expect	":${BSL}M^?:"		%B	':ÿ:'

	# Then, more or less nonsense
	expect	"   abcd${BSL}n"	%9B	"abcd${NL}"
	expect	"ab${BSL}tcd   "	%-9B	"ab	cd"
	expect	" ${BSL}'${BSL}^?"	%6B	\'''
	expect	"${BSL}^A${BSL}^B "	%-7B	''
	expect	"  -${BSL}M^W-"		%8B	'-—-'
	expect	":${BSL}M^?:  "		%-8B	':ÿ:'

	# and finally, the absurd, ridiculous, and bizarre (useless)
	expect	"abcd${BSL}"		%.5B	"abcd${NL}"
	expect	"ab${BSL}"		%.3B	"ab	cd"
	expect	"${BSL}\"${BSL}"	%.3B	'"'
	expect	"${BSL}"		%.1B	\'''
	expect	"${BSL}^"		%.2B	''
	expect	"x${BSL}M-"		%.4B 	'xÙz'
	expect	"-${BSL}M^"		%.4B	'-—-'
	expect	":${BSL}M"		%.3B	':ÿ:'

	return $RVAL
}
define	B_string_expand		"NetBSD specific %B string expansion"


#############################################################################
#############################################################################
#
# The code to make the tests above actually run starts here...
#

# if setup fails, then ignore any test names on command line
# Just run the (one) test that setup() established
setup || set --

NL='
'
# test how the shell we're running handles quoted patterns in vars
# Note: it is not our task here to diagnose the broken shell
B1='\'
B2='\\'
case "${B1}" in
(${B2})	BSL="${B2}";;		# This one is correct
(${B1}) BSL="${B1}";;		# but some shells can't handle that
(*)	BSL=BROKEN_SHELL;;	# !!!
esac

if $Running_under_ATF
then
	# When in ATF, just add the test cases, and finish, and ATF
	# will take care of running everything

	atf_init_test_cases() {

		for T in $Tests
		do
			atf_add_test_case "$T"
		done
		return 0
	}
	exec 3>&2
else
	# When not in AFT, we need to do it all here...

	Failed=
	Failures=0

	STDERR=$(mktemp ${TMPDIR:-/tmp}/Test-XXXXXX)
	trap "rm -f '${STDERR}'" EXIT
	exec 3>"${STDERR}"

	case "$#" in
	(0)	set -- $Tests ;;
	esac

	for T
	do
		$T || {
			Failed="${Failed}${Failed:+${NL}}	${T} : "
			eval Failed='${Failed}${TEST_'"${T}"'_MSG}'
			Failures=$(( $Failures + 1 ))
		}
	done
	if [ $Failures -gt 0 ]
	then
		s=s
		test $Failures -eq 1 && s=

		exec >&2
		echo
		echo =================================================
		echo
		echo "$Failures test$s failed:"
		echo "$Failed"
		echo
		echo =================================================

		if test -s "${STDERR}"
		then
			echo
			echo The following appeared on stderr during the tests:
			echo
			cat "${STDERR}"
		fi
	fi
fi
