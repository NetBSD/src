# $NetBSD: t_syntax.sh,v 1.4 2017/06/16 07:30:32 kre Exp $
#
# Copyright (c) 2017 The NetBSD Foundation, Inc.
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
: ${TEST_SH:=/bin/sh}

# This set of tests verifies various requirementgs relating to correct
# (and incorrect) syntax of shell input
#
# There is no intent in these tests to verify correct operation
# (though that sometimes cannot be separated from correct parsing.)
# That is (or should be) verified elsewhere.
#
# Also, some very basic syntax is tested in almost every test
# (they cannot work without basic parsing of elementary commands)
# so that is also not explicitly tested here.
#
# Similarly word expansion, field splitting, redirection, all have
# tests of their own (though we do test parsing of redirect ops).
#
# Note that in order to test the basic facilities, other shell operations
# are used - a test failure here does not necessarily mean that the
# operation intended to be tested is faulty, just that something is.

atf_test_case a_basic_tokenisation
a_basic_tokenisation_head() {
	atf_set "descr" "Test the shell correctly finds various tokens"
}
a_basic_tokenisation_body() {
	atf_check -s exit:0 -o 'inline:3\n' -e empty ${TEST_SH} -c \
		'set -- a b c; echo $#'
	atf_check -s exit:0 -o 'inline:2\n' -e empty ${TEST_SH} -c \
		'set -- a""b c; echo $#'
	atf_check -s exit:0 -o 'inline:3\n' -e empty ${TEST_SH} -c \
		'set -- a"" b c; echo $#'
	atf_check -s exit:0 -o 'inline:3\n' -e empty ${TEST_SH} -c \
		'set -- ""a b c\;; echo $#'

	atf_check -s exit:0 -o 'inline:3\n' -e empty ${TEST_SH} -c \
		'set -- set -- c; echo $#'
	atf_check -s exit:0 -o 'inline:1\n' -e empty ${TEST_SH} -c \
		'set --;set -- c; echo $#'
	atf_check -s exit:0 -o 'inline:1\n' -e empty ${TEST_SH} -c \
		'set --&set -- c; echo $#'
	atf_check -s exit:0 -o 'inline:1\n' -e empty ${TEST_SH} -c \
		'set -- a b&&set -- c; echo $#'
	atf_check -s exit:0 -o 'inline:2\n' -e empty ${TEST_SH} -c \
		'set -- a b||set -- c; echo $#'
}

atf_test_case b_comments
b_comments_head() {
	atf_set "descr" "Test the shell correctly handles comments"
}
b_comments_body() {

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c '#'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c '# exit 1'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c 'true # exit 1'
	atf_check -s exit:1 -o empty -e empty ${TEST_SH} -c 'false # exit 0'

	atf_check -s exit:0 -o 'inline:foo\n' -e empty ${TEST_SH} -c \
		'echo foo # bar'
	atf_check -s exit:0 -o 'inline:foo # bar\n' -e empty ${TEST_SH} -c \
		'echo foo \# bar'
	atf_check -s exit:0 -o 'inline:foo\n' -e empty ${TEST_SH} -c \
		'echo foo; # echo bar'
	atf_check -s exit:0 -o 'inline:foo#bar\n' -e empty ${TEST_SH} -c \
		'echo foo#bar'
	atf_check -s exit:0 -o 'inline:foo# bar\n' -e empty ${TEST_SH} -c \
		'echo foo# bar'
	atf_check -s exit:0 -o 'inline:foo\n' -e empty ${TEST_SH} -c \
		'x=foo; echo ${x#bar}'

	atf_check -s exit:0 -o 'inline:#\n' -e empty ${TEST_SH} -c \
		'echo "#"'
	atf_check -s exit:0 -o 'inline:#\n' -e empty ${TEST_SH} -c \
		"echo '#'"
	atf_check -s exit:0 -o 'inline:#\n' -e empty ${TEST_SH} -c \
		'echo \#'
	atf_check -s exit:0 -o 'inline:##\n' -e empty ${TEST_SH} -c \
		'echo "#"#'
	atf_check -s exit:0 -o 'inline:##\n' -e empty ${TEST_SH} -c \
		"echo '#'#"
	atf_check -s exit:0 -o 'inline:##\n' -e empty ${TEST_SH} -c \
		'echo \##'
	atf_check -s exit:0 -o 'inline:##\n' -e empty ${TEST_SH} -c \
		'echo "#"# #"#"'
	atf_check -s exit:0 -o 'inline:##\n' -e empty ${TEST_SH} -c \
		"echo '#'# #'#'"
	atf_check -s exit:0 -o 'inline:##\n' -e empty ${TEST_SH} -c \
		'echo \## #\#'

	cat <<-'DONE'|atf_check -s exit:0 -o inline:'foo\n' -e empty ${TEST_SH}
		# test comments do not provoke synax errors !\
		echo foo # ( { " hello
		while : # that's forever
		do	# the following command list
			# starting with nothing ${unset?error}
			break	# done loop terminate $( echo bar; exit 1 )
		done #####################################################
		# "hello
		exit 0
	DONE
}

atf_test_case c_line_wrapping
c_line_wrapping_head() {
	atf_set "descr" "check aspects of command line wrapping"
}
c_line_wrapping_body() {
	atf_require_prog ls
	atf_require_prog printf

	cat <<- 'DONE' | atf_check -s exit:0 -o ignore -e empty ${TEST_SH} -e
		l\
		s
	DONE

	cat <<- 'DONE' | atf_check -s exit:7 -o empty -e empty ${TEST_SH}
		e\
		x\
		it \
		7
	DONE

	# Have to do this twice as cannot say "any exit code but 0 or 7" ...
	cat <<- 'DONE' | atf_check -s not-exit:0 -o empty -e not-empty \
	    ${TEST_SH}
		e\
		x\
		it\
		7
	DONE
	cat <<- 'DONE' | atf_check -s not-exit:7 -o empty -e not-empty \
	    ${TEST_SH}
		e\
		x\
		it\
		7
	DONE

	cat <<- 'DONE' | atf_check -s exit:0 -o empty -e empty  ${TEST_SH}
		wh\
		il\
		e \
		f\a\
		\l\s\e
		do
		:\
		;
		done
	DONE

	cat <<- 'DONE' | atf_check -s exit:0 -o inline:'hellohellohellohello' \
	    -e empty ${TEST_SH}
		V\
		AR=hel\
		lo
		unset U V1
		pri\
		ntf '%s' ${\
		VAR\
		}
		p\
		r\
		i\
		n\
		t\
		f\
		 \
		'%s' \
		$\
		{\
		V\
		A\
		R}
		printf '%s' ${U\
		-\
		"$\
		{V\
		1:\
		=$\
		{V\
		AR+\
		${V\
		AR}\
		}\
		}"}
		printf '%s' ${V\
		1?V1\
		 \
		FAIL}
	DONE

	cat <<- 'DONE' | atf_check -s exit:0 -o inline:'2\n' ${TEST_SH}
		l\
		s=7 bi\
		n\
		=\
		3
		echo $(\
		( ls /bin )\
		)
	DONE

	# Inspired by src/etc/MAKEDEV.tmpl failure with (broken)
	# sh LINENO code...  avoid it happening again...
	for VARS in 1:0:0:0 0:1:0:0 0:0:1:0 0:0:0:1 \
		    1:0:0:1 1:0:1:0 1:1:0:0 0:1:1:0 \
		    0:0:0:0 1:1:0:1 0:1:1:1 1:1:1:1
	do
		eval $(
			IFS=:
			set -- $VARS
			test $(( $1 + $2 + $3 + $4 )) -eq 1 &&
				R=OK || R=BAD
			printf "R=%s;" $R
			for v in a b c d
			do
				case $1 in
				(0)	printf "export %s=false;" $v;;
				(1)	printf "export %s=true;"  $v;;
				esac
				shift
			done
		)

		cat <<- 'DONE' | atf_check -s exit:0 -o inline:"${R}" ${TEST_SH}
			case $(( $($a && echo 1 || echo 0) + \
				 $($b && echo 1 || echo 0) + \
				 $($c && echo 1 || echo 0) + \
				 $($d && echo 1 || echo 0) ))
			in
			(1)	printf OK ;;
			(*)	printf BAD ;;
			esac
		DONE
	done

	# inspired by pkgsrc/pkgtools/cwrappers :: libnbcompat/configure
	# failure with (broken) sh LINENO core .. avoid recurrence
	# This test would have failed.
	cat <<- 'DONE' | atf_check -s exit:0 -o inline:'/tmp\n' ${TEST_SH}
		dn=/tmp/foo

		D=`dirname -- "${dn}" ||
		expr	X"${dn}" : 'X\(.*[^/]\)//*[^/][^/]*/*$' \| \
			X"${dn}" : 'X\(//\)[^/]' \| \
			X"${dn}" : 'X\(//\)$' \| \
			X"${dn}" : 'X\(/\)' \| . 2>/dev/null ||
		echo X"${dn}" |
		    sed '/^X\(.*[^/]\)\/\/*[^/][^/]*\/*$/{
			    s//\1/
			    q
			  }
			  /^X\(\/\/\)[^/].*/{
			    s//\1/
			    q
			  }
			  /^X\(\/\/\)$/{ 
			    s//\1/
			    q
			  } 
			  /^X\(\/\).*/{
			    s//\1/
			    q
			  }
			  s/.*/./; q'`

		echo "${D}"
	DONE
	return 0
}

atf_test_case d_redirects
d_redirects_head() {
	atf_set "descr" "Check parsing of redirect operators"
}
d_redirects_body() {

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'>/dev/null'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'</dev/null'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'>>/dev/null'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'<>/dev/null'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'</dev/null>/dev/null'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'>|/dev/null'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'>/dev/null>/dev/null>/dev/null'

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'echo hello >/dev/null'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'echo >/dev/null hello'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'>/dev/null echo hello'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'echo hello >/dev/null world'
	atf_check -s exit:0 -o 'inline:hello world\n' -e empty ${TEST_SH} -c \
		'echo hello </dev/null world'

	atf_check -s exit:0 -o 'inline:hello\n' -e empty ${TEST_SH} -c \
		'echo hello </dev/null'
	atf_check -s exit:0 -o 'inline:hello\n' -e empty ${TEST_SH} -c \
		'echo hello 3</dev/null'
	atf_check -s exit:0 -o 'inline:hello 3\n' -e empty ${TEST_SH} -c \
		'echo hello 3 </dev/null'
	atf_check -s exit:0 -o 'inline:hello 3\n' -e empty ${TEST_SH} -c \
		'echo hello \3</dev/null'
	atf_check -s exit:0 -o 'inline:hello\n' -e empty ${TEST_SH} -c \
		'echo hello</dev/null'
	atf_check -s exit:0 -o 'inline:3\n' -e empty ${TEST_SH} -c \
		'hello=3; echo ${hello}</dev/null'

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'2>&1'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'2>& 1'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'FD=1; 2>&"${FD}"'
	atf_check -s exit:0 -o 'inline:hello\n' -e empty ${TEST_SH} -c \
		'FD=1; echo hello 2>&"${FD}" >&2'

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'2>&- 3<&- 4>&-'

	return 0
}

atf_test_case f_variable_syntax
f_variable_syntax_head() {
	atf_set "descr" "Check that var names of all legal forms work"
}
f_variable_syntax_body() {
	# don't test _ as a variable, it can be "unusual"
	for vname in a ab _a _9 a123 a_1_2_3 __ ___ ____ __1__ _0 \
	    A AA AAA AaBb _A_a A_a_ a1_ abc_123 ab_12_cd_ef_34_99 \
	    abcdefghijklmnopqrstuvwzyz ABCDEFGHIJKLMNOPQRSTUVWXYZ_ \
	    A_VERY_LONG_VARIABLE_NAME_that_is_probably_longer_than_most_used_in_the_average_shell_script_already_about_100_chars_in_this_one_but_there_is_not_supposed_to_be_any_limit_on_the_length_at_all xyzzy \
	    _____________________________________________________________
	do
		atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
			"unset ${vname}"
		atf_check -s exit:0 -o match:OK -e empty ${TEST_SH} -c \
			"unset ${vname}; printf %s \${${vname}-OK}"
		atf_check -s exit:0 -o match:GOOD -e empty ${TEST_SH} -c \
			"${vname}=GOOD; printf %s \${${vname}-OK}"
		atf_check -s exit:0 -o match:GOOD -e empty ${TEST_SH} -c \
			"${vname}=GOOD; printf %s \$${vname}"
		atf_check -s exit:0 -o match:GOOD -e empty ${TEST_SH} -c \
			"unset ${vname};${vname}=GOOD;printf %s \${${vname}-OK}"
		atf_check -s exit:0 -o match:OK -e empty ${TEST_SH} -c \
			"${vname}=GOOD;unset ${vname};printf %s \${${vname}-OK}"
		atf_check -s exit:0 -o match:GOOD -e empty ${TEST_SH} -c \
			"${vname}=GOOD; unset ${vname}x; printf %s \$${vname}"
		atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
			"unset ${vname}x; ${vname}=GOOD; printf %s \$${vname}x"
	done

	# don't test '.' in var names, some shells permit that (in ${} anyway)
	# this test also cannot check for embedded - + ? = : % or #
	for vname in ,x -p +h :def 0_1 'x*x' '()' '"' "'" 'a b c' '?!' ';'
	do
		atf_check -s not-exit:0 -o ignore -e not-empty ${TEST_SH} -c \
			"echo \${${vname}}"
	done

	for vname in ,x -p +h :def 0_1 'x*x' '()' '"' "'" 'a b c' x,y,z '?!' \
	    ';' a-b a+b 'a?b' 'a:b' 'a%b' 'a#b' 0 1 99 @ '*' '!' '?'
	do
		# failure modes will differ, but they should all fail somehow
		atf_check -s not-exit:0 -o ignore -e not-empty ${TEST_SH} -c \
			"${vname}="
	done

}

atf_test_case g_var_assign
g_var_assign_head() {
	atf_set "descr" "Check var assignments "
}
g_var_assign_body() {
	atf_check -s exit:0 -e empty -o empty ${TEST_SH} -c \
		'a=b'
	atf_check -s not-exit:0 -e not-empty -o empty ${TEST_SH} -c \
		'\a=b'
	atf_check -s exit:0 -e empty -o empty ${TEST_SH} -c \
		'a=b c=d'
	atf_check -s exit:0 -e empty -o 'inline:e=f\n' ${TEST_SH} -c \
		'a=b c=d echo e=f'
	atf_check -s exit:0 -e empty -o 'inline:e=f\n' ${TEST_SH} -c \
		'a=b 2>/dev/null c=d </dev/null echo e=f'

	# We need to make sure that we do not accidentally
	# find a command called 'a=b' ...

	for d in /foo /foo/bar /not-dir /no/such/directory '/!!!' \
		'/-/--/#' '/"/""/"""' - 
	do
		test -d "${d}" || break
	done
	test "${#d}" -gt 1 || atf-skip 'Wacky directories seem to exist!'

	atf_check -s exit:0 -e empty -o empty ${TEST_SH} -c \
		"PATH='${d}';"'a=\b'
	atf_check -s not-exit:0 -e not-empty -o empty ${TEST_SH} -c \
		"PATH='${d}';"'a\=b'
	atf_check -s not-exit:0 -e not-empty -o empty ${TEST_SH} -c \
		"PATH='${d}';"'\a=b'
	atf_check -s not-exit:0 -e not-empty -o empty ${TEST_SH} -c \
		"PATH='${d}';"'X=; c=d ${X} a=b'
}

atf_test_case i_pipelines
i_pipelines_head() {
	atf_set "descr" "Check pipelines"
}
i_pipelines_body() {

	cmd='printf "%s\n" foo'
	for n in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
	do
		atf_check -s exit:0 -o inline:'foo\n' -e empty ${TEST_SH} -c \
			"${cmd}"
		cmd="${cmd} | cat"
	done

	cmd='printf "%s\n" foo'
	for n in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
	do
		atf_check -s exit:0 -o inline:'foo\n' -e empty ${TEST_SH} -c \
			"${cmd}"
		cmd="${cmd} |
		cat"
	done

	cmd='printf "%s\n" foo'
	for n in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
	do
		atf_check -s exit:0 -o inline:'foo\n' -e empty ${TEST_SH} -c \
			"${cmd}"
		cmd="${cmd} |




		cat"
	done

	cmd='! printf "%s\n" foo'
	for n in 1 2 3 4 5 6 7 8 9 10
	do
		atf_check -s exit:1 -o inline:'foo\n' -e empty ${TEST_SH} -c \
			"${cmd}"
		cmd="${cmd} | cat"
	done

	cmd='exec 4>&2 3<&0; printf "%s\n" foo'
	for n in 1 2 3
	do
	    pfx=
	    for xtra in 'x=y' 'a=b' '6>&1' '5<&3'
	    do
		atf_check -s exit:0 -o inline:'foo\n' -e empty ${TEST_SH} -c \
			"${cmd} | ${xtra} cat"

		atf_check -s exit:0 -o inline:'foo\n' -e empty ${TEST_SH} -c \
			"${cmd} | ${pfx} cat"

		pfx="${pfx} ${xtra}"
	    done
	    cmd="${cmd} | ${pfx} cat"
	done

	# pipelines are not required to contain commands ...
	# they don't do anything useful (at all) but the syntax is legal
	base='4>&2'; cmd="${base}"
	for pipe in 'a=b' '3<&0' '>>/dev/null' 'a= b= c=' '${x}' 'cat'
	do
		atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
			"${base} | ${pipe}"

		atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
			"${cmd} | ${pipe}"

		cmd="${cmd} | ${pipe}"
	done

	# but the command cannot be empty, or a reserved word used improperly
	base='printf "%s\n" foo'; cmd="${base}"
	for pipe in '' do done then else fi esac
	do
		atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
			"${base} | ${pipe}"

		atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
			"${pipe} | ${base}"

		atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
			"${cmd} | ${pipe}"

		cmd="${cmd} | ${pipe}"
	done
}

atf_test_case j_and_or_lists
j_and_or_lists_head() {
	atf_set "descr" "Check && and || command lists"
}
j_and_or_lists_body() {
	and=true
	or=false
	and_or=false
	for i in 1 2 3 4 5 6 7 8 9 10
	do
		atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
			"${and}"

		atf_check -s exit:1 -o empty -e empty ${TEST_SH} -c \
			"${or}"

		atf_check -s exit:1 -o empty -e empty ${TEST_SH} -c \
			"${and_or}"

		and="${and} && true"
		or="${or} || false"
		and_or="${and_or} || true && false"
	done

	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'true &&'
	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'&& true'
	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'|| true'
	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'true ||'
	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'true || && false'
	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'false || && true'

	cmd='printf "%s" foo | cat | cat>/dev/null'
	line="${cmd}"
	for i in 1 2 3 4
	do
		line="${line} && ! ${cmd} || ${cmd}"

		atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
			"${line}"
	done

}

atf_test_case k_lists
k_lists_head() {
	atf_set "descr" "Check ; command lists"
}
k_lists_body() {
	line=
	for N in 1 2 3 4
	do
		for cmd in \
			true false : 'cat</dev/null>/dev/null' x=3 'exec 4>&-'
		do
			line="${line}${line:+;}${cmd}"
			atf_check -s exit:0 -o 'inline:hello\nworld\n' \
				-e empty ${TEST_SH} -c \
					"echo hello; ${line}; echo world"
			atf_check -s exit:0 -o 'inline:hello\nworld\n' \
				-e empty ${TEST_SH} -c \
					"echo hello; ${line}; echo world;"
		done
	done

	for cmd in ';' ';;' 'false ;; true' 'false; ;true' '; true'
	do
		atf_check -s not-exit:0 -o ignore -e not-empty \
			${TEST_SH} -c "${cmd}"
	done
}

atf_test_case l_async_lists
l_async_lists_head() {
	atf_set "descr" "Check & command lists"
}
l_async_lists_body() {
	line=
	for N in 1 2 3 4
	do
		for cmd in \
			true false : 'cat</dev/null>/dev/null' x=3 'exec 4>&-'
		do
			line="${line:+${line}&}${cmd}"
			atf_check -s exit:0 -o 'inline:hello\nworld\n' \
				-e empty ${TEST_SH} -c \
					"echo hello; ${line}& echo world"
			atf_check -s exit:0 -o 'inline:hello\nworld\n' \
				-e empty ${TEST_SH} -c \
					"echo hello; ${line}& echo world"
		done
	done

	for cmd in '&' ';&' '&;'  '& true' 'false& &true'
	do
		atf_check -s not-exit:0 -o ignore -e not-empty \
			${TEST_SH} -c "${cmd}"
	done
}

atf_test_case m_compound_lists
m_compound_lists_head() {
	atf_set "descr" "Check subshells () and { ;} command grouping"
}
m_compound_lists_body() {
	# Note: (( is an unspecified (reserved) operator, don't use it...
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'( true )'
	atf_check -s exit:1 -o empty -e empty ${TEST_SH} -c \
		'( false )'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'( (:) )'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'( ( true ))'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'( ( ( ( (  true )))))'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'( ( ( ( (true);:));true))'

	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'()'
	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'(   )'

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'{ true; }'
	atf_check -s exit:1 -o empty -e empty ${TEST_SH} -c \
		'{ false; }'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'{ { :; };}'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'{ { { { {  :;};};};};}'

	atf_check -s exit:0 -o 'inline:}\n' -e empty ${TEST_SH} -c \
		'{ echo } ; }'
	atf_check -s exit:0 -o 'inline:{\n' -e empty ${TEST_SH} -c \
		'{ echo { ; }'
}

atf_test_case q_for_loop
q_for_loop_head() {
	atf_set "descr" "Check for loop parsing"
}
q_for_loop_body() {
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'for x; do : ; done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'for x in ; do : ; done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'for x in a b c ; do : ; done'

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'for x in in;do : ; done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'for x in for;do : ; done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'for x in do;do : ; done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'for for in in;do :;done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'for for in for;do :; done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'for for in do;do : ;done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'for in in in;do : ; done'
	atf_check -s exit:0 -o 'inline:do\nin\ndo\n' -e empty ${TEST_SH} -c \
   'for in in in do in;do case $in in in)echo do;;do)echo in;;esac; done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'for in in for;do : ; done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'for in in do;do : ; done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'for do in in;do : ; done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'for do in for;do : ; done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'for do in do;do : ; done'
	atf_check -s exit:0 -o 'inline:dodo\n' -e empty ${TEST_SH} -c \
		'for do in do;do echo ${do}do ; done'
}

atf_test_case r_case
r_case_head() {
	atf_set "descr" "Check case statement parsing"
}
r_case_body() {
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in  esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in x) esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in (x) esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in x) ;; esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in (x) ;; esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in x|y) ;; esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in (x|y) ;; esac'

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in x|esac) ;; esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in x|esac|y) ;; esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in (x|esac) ;; esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in (x|esac|y) ;; esac'

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in in) esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in in) ;; esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in x|in) ;; esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in x|in|y) ;; esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in (x|in) ;; esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in (in|x) ;; esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in (x|in|y) ;; esac'

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case case in case) esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case in in in) esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case esac in (in) esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case in in esac|cat'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case esac in esac|cat'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case in in esac|case x in u) echo foo;; esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case esac in esac|case x in u) echo foo;; esac'
	atf_check -s exit:0 -o 'inline:foo\n' -e empty ${TEST_SH} -c \
		'case in in esac|case x in x) echo foo;; esac'

	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'case in in (esac|cat'

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in x );;esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in ( x );;esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in x | y );;esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in ( x | y );;esac'

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x
		 in
		 (    x    |    y    )

			;;


		esac
		'
}

atf_test_case s_if
s_if_head() {
	atf_set "descr" "Check if statement parsing"
}
s_if_body() {
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'if :; then :; fi'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'if :; then :; else :; fi'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'if :; then :; elif :; then :; else :; fi'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'if :; then :; elif :; then :; elif :; then :; else :; fi'

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'if :; then : else :; fi'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'if : then :; then :; fi'

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'if if :;then :;fi then :;fi'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'if if :;then if :;then :;fi fi;then :;fi'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'if if :;then :;fi;then :;else if :;then :;fi fi'

	for a in true false; do
		for b in true false; do
			for c in true false; do

				$a && out=a || {
				$b && out=b || {
				$c && out=c || {
				out=d; };};}

				atf_check -s exit:0 -e empty \
					-o "inline:${out}"'\n' \
					${TEST_SH} -c \
						"if $a; then echo a
						elif $b; then echo b
						elif $c; then echo c
						else echo d
						fi"
			done
		done
	done
}

atf_test_case t_loops
t_loops_head() {
	atf_set "descr" "Check while/until loop parsing"
}
t_loops_body() {
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'while false; do :; done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'while false; do \done; done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'until :; do :; done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'until :; do \done; done'

	atf_check -s exit:0 -o 'inline:x\n1\n' -e empty ${TEST_SH} -c \
		':; while (exit) do echo x; false; done; echo $?'
	atf_check -s exit:0 -o 'inline:x\n0\n' -e empty ${TEST_SH} -c \
		'false; until (exit) do echo x; done; echo $?'
}

atf_test_case u_case_cont
u_case_cont_head() {
	atf_set "descr" "Check case stmt parsing using ;& [optional]"
}
u_case_cont_body() {

	${TEST_SH} -c 'case x in (x) false ;& (y) : ;; esac' 2>/dev/null ||
		atf_skip ";& case list terminator unsupported in ${TEST_SH}"

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in x) ;& esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in (x) ;& esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in x|y) ;& esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in (x|y) ;& esac'

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in x );&esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in ( x );&esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in x | y );&esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in ( x | y );&esac'

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in x) ;& (y) esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in (x) ;& esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in x|y) ;& esac'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'case x in (x|y) ;& esac'
}

atf_test_case x_functions
x_functions_head() {
	atf_set "descr" "Check function definition parsing"
}
x_functions_body() {
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'func() { :; }'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'func() { :; }; func'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'func()(:)'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'func()if :; then false; else true; fi'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'func()while false; do :;done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'func () for a in b c; do :; done'
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'func() case x in (y) esac'

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'f1() { f2() { :; }; }; f1; f2'

	# f2 should be not found, but f1 clears $?
	atf_check -s exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'f1() { f2() { :; }; }; f2; f1'

	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c \
		'f1() { eval "$1() { :; }"; }; f1 f2; f2'
}

atf_init_test_cases() {
	atf_add_test_case a_basic_tokenisation
	atf_add_test_case b_comments
	atf_add_test_case c_line_wrapping
	atf_add_test_case d_redirects
	atf_add_test_case f_variable_syntax
	atf_add_test_case g_var_assign
	atf_add_test_case i_pipelines
	atf_add_test_case j_and_or_lists
	atf_add_test_case k_lists
	atf_add_test_case l_async_lists
	atf_add_test_case m_compound_lists
	atf_add_test_case q_for_loop
	atf_add_test_case r_case
	atf_add_test_case s_if
	atf_add_test_case t_loops
	atf_add_test_case u_case_cont
	atf_add_test_case x_functions
}
