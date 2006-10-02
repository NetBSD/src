#!/bin/sh
#
# $NetBSD: ps.sh,v 1.1 2006/10/02 17:59:00 apb Exp $
#
# Test ps(1)

# The source directory, where we can find data files
SRCDIR="$(dirname "$0")"
# the implementation of "ps" to test
: ${TEST_PS:="ps"}
# tab and newline characters
tab="$(printf '\t')"
# nl="$(printf '\n')" doesn't work
nl='
'

#
# Complain and increment error count.  Return false.
#
errors=0
error()
{
	echo "ERROR $*"
	errors=$(( $errors + 1 ))
	false
}

#
# Parse the "keywords" file into a load of shell variables
#
setup_keywords()
{
	# Set variables representing the header text
	# for all normal keywords (except aliases), and
	# for regular expressions to match the text in left- or
	# right-justified columns.
	# For example, head_text_p_cpu="%CPU" head_regexp_p_cpu=" *%CPU".
	while read keyword heading flag
	do
		case "$keyword" in
		''|\#*)	continue
			;;
		esac
		[ x"$flag" = x"ALIAS" ] && continue
		kvar="${keyword}"
		case "${keyword}" in
		%*)	kvar="p_${keyword#%}"
			;;
		esac
		eval head_text_${kvar}=\'"${heading}"\'
		case "${flag}" in
		'')	# right justified
			eval head_regexp_${kvar}=\'" *${heading}"\'
			;;
		LJUST)	# left justified
			eval head_regexp_${kvar}=\'"${heading} *"\'
			;;
		*)	error "unknown flag in keywords"
			;;
		esac
	done <"${SRCDIR}/keywords"

	# Now do the aliases.
	while read keyword heading flag
	do
		case "$keyword" in
		''|\#*)	continue
			;;
		esac
		[ x"$flag" != x"ALIAS" ] && continue
		kvar="${keyword}"
		avar="${heading}"
		case "${keyword}" in
		%*)	kvar="p_${keyword#%}"
			;;
		esac
		case "${heading}" in
		%*)	avar="p_${heading#%}"
			;;
		esac
		eval head_text_${kvar}=\"\$head_text_${avar}\"
		eval head_regexp_${kvar}=\"\$head_regexp_${avar}\"
	done <"${SRCDIR}/keywords"

	# default sets of keywords
	default_keywords='pid tty stat time command'
	j_keywords='user pid ppid pgid sess jobc state tt time command'
	l_keywords='uid pid ppid cpu pri nice vsz rss wchan state tt time command'
	s_keywords='uid pid ppid cpu lid nlwp pri nice vsz rss wchan lstate tt time command'
	u_keywords='user pid %cpu %mem vsz rss tt state start time command'
	v_keywords='pid state time sl re pagein vsz rss lim tsiz %cpu %mem command'
}

# Convert a list of keywords like "pid comm" to a regexp
# like " *PID COMMAND *"
heading_keywords_to_regexp()
{
	local keywords="$1"
	local regexp
	regexp="$(echo "$keywords" | \
		sed -E -e 's/\%/p_/g' -e 's/(^| )/\1\$head_regexp_/g')"
	eval regexp=\""${regexp}"\"
	regexp="^${regexp}\$"
	echo "$regexp"
}

#
# Check that a string matches a regexp; use the specified id
# in error or success messages.
#
check_regexp() {
	local id="$1" string="$2" regexp="$3"
	if ! expr "$string" : "$regexp" >/dev/null
	then
		error "${id}: expected [${regexp}], got [${string}]"
		false
	fi
}

#
# Run "ps $args -p $$"; check that only one line is printed,
# without a preceding header line.
#
check_no_heading_line()
{
	local args="$1"
	local output="$(eval "${TEST_PS} $args -p $$")"
	case "$output" in
	*"$nl"*)
		local firstline="${output%%${nl}*}"
		error "check_no_heading_line [$args] got [$firstline]"
		;;
	*)
		echo "OK no_heading_line [$args]"
		;;
	esac
}

#
# Run "ps $args"; check that the heading matches the expected regexp.
#
check_heading_regexp()
{
	args="$1"
	regexp="$2"
	actual="$( eval "${TEST_PS} $args" | sed -e 1q )"
	check_regexp "heading [$args]" "${actual}" "${regexp}" \
	&& echo "OK check_heading [$args]"
}

#
# Run "ps $args"; check that the heading matches a regexp constructed
# from the specified keywords.
#
check_heading_keywords()
{
	args="$1"
	keywords="$2"
	check_heading_regexp "$args" "$(heading_keywords_to_regexp "$keywords")"
}

#
# Try several variations on "ps $flag", "ps -$flag", etc.,
# and check that the heading always has the correct keywords.
#
check_heading_variations()
{
	flag="$1"
	keywords="$2"
	for args in "$flag" "-$flag" "-$flag$flag -$flag"; do
		check_heading_keywords "$args" "$keywords"
	done
}

#
# Check that the default set of columns is correct;
# also check that the columns printed with "ps -j", "ps -l",
# "ps -s", "ps -u" and "ps -v" are correct.
#
check_default_columns()
{
	check_heading_keywords '' "$default_keywords"
	check_heading_variations 'j' "$j_keywords"
	check_heading_variations 'l' "$l_keywords"
	check_heading_variations 's' "$s_keywords"
	check_heading_variations 'u' "$u_keywords"
	check_heading_variations 'v' "$v_keywords"
}

#
# Check that "ps -O foo" inserts columns just after the pid column.
#
check_minus_O()
{
	check_heading_keywords '-O %cpu,%mem' \
		"$(echo "${default_keywords}" | sed -e 's/pid/pid %cpu %mem/')"
	check_heading_keywords '-O %cpu -O %mem' \
		"$(echo "${default_keywords}" | sed -e 's/pid/pid %cpu %mem/')"
	check_heading_keywords '-O%cpu -O%mem' \
		"$(echo "${default_keywords}" | sed -e 's/pid/pid %cpu %mem/')"
}

#
# Check simple cases of "ps -o foo" to control which columns are printed.
# This does not test header overriding via "ps -o foo=BAR".
#
check_minus_o()
{
	# Keywords for "-o name" override the default display
	check_heading_keywords '-o pid,%cpu,%mem' \
		"pid %cpu %mem"
	check_heading_keywords '-o pid -o %cpu,%mem' \
		"pid %cpu %mem"
	check_heading_keywords '-opid -o %cpu,%mem' \
		"pid %cpu %mem"
	# Space works like comma
	check_heading_keywords '-opid -o "%cpu %mem"' \
		"pid %cpu %mem"
	# Check missing pid
	check_heading_keywords '-o comm' \
		"comm"
	# Check pid present but not first
	check_heading_keywords '-o comm,pid' \
		"comm pid"
}

#
# Test simple uses of header overriding via "ps -o foo=BAR".
# This does not test columns with null headings, or headings with
# embedded space, "," or "=".
#
check_override_heading_simple()
{
	check_heading_regexp '-o pid=PPP -o comm' \
		'^ *PPP '"${head_text_comm}"'$' # no trailing space
	check_heading_regexp '-o pid=PPP -o comm=CCC' \
		'^ *PPP CCC$'
	check_heading_regexp '-o pid,comm=CCC' \
		'^'"${head_regexp_pid}"' CCC$'
	check_heading_regexp '-o pid -o comm=CCC' \
		'^'"${head_regexp_pid}"' CCC$'
	# Check missing pid
	check_heading_regexp '-o comm=CCC' \
		'^CCC$'
	# Check pid present but not first
	check_heading_regexp '-o comm=CCC -o pid=PPP' \
		'^CCC  *PPP$'
	check_heading_regexp '-o comm,pid=PPP' \
		'^'"${head_regexp_comm}"'  *PPP$'
}

#
# Test header overriding with embedded space, "," or "=".
# Everything after the first "=" is part of the heading.
#
check_override_heading_embedded_specials()
{
	# Check embedded "," or "=" in override header.
	check_heading_regexp '-o comm,pid==' \
		'^'"${head_regexp_comm}"'  *=$'
	check_heading_regexp '-o comm,pid=,' \
		'^'"${head_regexp_comm}"'  *,$'
	check_heading_regexp '-o pid=PPP,comm' \
		'^ *PPP,comm$' # not like '-o pid=PPP -o comm'
	check_heading_regexp '-o pid=PPP,comm=CCC' \
		'^ *PPP,comm=CCC$' # not like '-o pid=PPP -o comm=CCC'
	check_heading_regexp '-o comm,pid=PPP,QQQ' \
		'^'"${head_regexp_comm}"'  *PPP,QQQ$'
	check_heading_regexp '-o comm,pid=ppid,tty=state' \
		'^'"${head_regexp_comm}"'  *ppid,tty=state$'
	# Check embedded space or tab in override header.
	check_heading_regexp '-o comm,pid="PPP QQQ"' \
		'^'"${head_regexp_comm}"'  *PPP QQQ$'
	check_heading_regexp '-o comm,pid="PPP${tab}QQQ"' \
		'^'"${head_regexp_comm}"'  *PPP'"${tab}"'QQQ$'
}

#
# Test simple uses of null column headings overriding via
# "ps -o foo=BAR -o baz=".  This does not test the case where all
# columns have null headings.
#
check_override_heading_some_null()
{
	check_heading_regexp '-o pid=PPP -o comm=' \
		'^ *PPP *$'
	check_heading_regexp '-o pid= -o comm=CCC' \
		'^ * CCC$'
	check_heading_regexp '-o pid -o comm=' \
		'^'"${head_regexp_pid}"' *$'
	# Check missing pid
	check_heading_regexp '-o ppid= -o comm=CCC' \
		'^ * CCC$'
	check_heading_regexp '-o ppid=PPP -o comm=' \
		'^ *PPP *$'
	# Check pid present but not first
	check_heading_regexp '-o comm= -o pid=PPP' \
		'^ * PPP$'
	check_heading_regexp '-o comm,pid=' \
		'^'"${head_regexp_comm}"' *$'
}

#
# Test the use of "ps -o foo= -o bar=" (with a null heading for every column).
# The heading should not be printed at all in this case.
#
check_override_heading_all_null()
{
	# A heading with a space is not a null heading,
	# so should not be suppressed
	check_heading_regexp '-o comm=" "' \
		'^ *$'
	# Null headings should be suppressed
	check_no_heading_line '-o pid= -o comm='
	check_no_heading_line '-o pid= -o comm='
	# Check missing pid
	check_no_heading_line '-o ppid='
	check_no_heading_line '-o comm='
	check_no_heading_line '-o command='
	check_no_heading_line '-o ppid= -o comm='
	check_no_heading_line '-o comm= -o ppid='
	# Check pid present but not first
	check_no_heading_line '-o comm= -o pid='
	check_no_heading_line '-o ppid= -o pid= -o command='
}

#
# Test the use of -o options to display the same column more than once.
#
check_duplicate_column()
{
	# two custom headers
	check_heading_regexp '-o pid=PPP -o pid=QQQ' \
		'^ *PPP  *QQQ$'
	# one custom header, before and after default header
	check_heading_regexp '-o pid=PPP -o pid' \
		'^ *PPP '"${head_regexp_pid}"'$'
	check_heading_regexp '-o pid -o pid=QQQ' \
		'^'"${head_regexp_pid}"'  *QQQ$'
	# custom headers both before and after default header
	check_heading_regexp '-o pid=PPP -o pid -o pid=QQQ' \
		'^ *PPP '"${head_regexp_pid}"'  *QQQ$'
}

# main
setup_keywords
check_default_columns
check_minus_O
check_minus_o
check_override_heading_simple
check_override_heading_embedded_specials
check_override_heading_some_null
check_override_heading_all_null
check_duplicate_column

if [ x"$errors" = x"0" ]; then
	echo "$0: no errors"
	exit 0
else
	echo "$0: $errors errors"
	exit 1
fi
