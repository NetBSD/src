# $NetBSD: t_redir.sh,v 1.4 2016/03/08 14:26:54 christos Exp $
#
# Copyright (c) 2016 The NetBSD Foundation, Inc.
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
# the implementation of "sh" to test
: ${TEST_SH:="/bin/sh"}

# Any failures in this first test means it is not worth bothering looking
# for causes of failures in any other tests, make this one work first.

# Problems with this test usually mean inadequate ATF_SHELL used for testing.
# (though if all pass but the last, it might be a TEST_SH problem.)

atf_test_case basic_test_method_test
basic_test_method_test_head()
{
	atf_set "descr" "Tests that test method works as expected"
}
basic_test_method_test_body()
{
	cat <<- 'DONE' |
	DONE
	atf_check -s exit:0 -o empty -e empty ${TEST_SH}
	cat <<- 'DONE' |
	DONE
	atf_check -s exit:0 -o match:0 -e empty ${TEST_SH} -c 'wc -l'

	cat <<- 'DONE' |
		echo hello
	DONE
	atf_check -s exit:0 -o match:hello -e empty ${TEST_SH} 
	cat <<- 'DONE' |
		echo hello
	DONE
	atf_check -s exit:0 -o match:1 -e empty ${TEST_SH} -c 'wc -l'

	cat <<- 'DONE' |
		echo hello\
					world
	DONE
	atf_check -s exit:0 -o match:helloworld -e empty ${TEST_SH} 
	cat <<- 'DONE' |
		echo hello\
					world
	DONE
	atf_check -s exit:0 -o match:2 -e empty ${TEST_SH} -c 'wc -l'

	printf '%s\n%s\n%s\n' Line1 Line2 Line3 > File
	atf_check -s exit:0 -o inline:'Line1\nLine2\nLine3\n' -e empty \
		${TEST_SH} -c 'cat File'

	cat <<- 'DONE' |
		set -- X "" '' Y
		echo ARGS="${#}"
		echo '' -$1- -$2- -$3- -$4-
		cat <<EOF
			X=$1
		EOF
		cat <<\EOF
			Y=$4
		EOF
	DONE
	atf_check -s exit:0 -o match:ARGS=4 -o match:'-X- -- -- -Y-' \
		-o match:X=X -o match:'Y=\$4' -e empty ${TEST_SH} 
}

atf_test_case do_input_redirections
do_input_redirections_head()
{
	atf_set "descr" "Tests that simple input redirection works"
}
do_input_redirections_body()
{
	printf '%s\n%s\n%s\nEND\n' 'First Line' 'Second Line' 'Line 3' >File

	atf_check -s exit:0 -e empty \
		-o inline:'First Line\nSecond Line\nLine 3\nEND\n' \
		${TEST_SH} -c 'cat < File'
	atf_check -s exit:0 -e empty \
		-o inline:'First Line\nSecond Line\nLine 3\nEND\n' \
		${TEST_SH} -c 'cat <File'
	atf_check -s exit:0 -e empty \
		-o inline:'First Line\nSecond Line\nLine 3\nEND\n' \
		${TEST_SH} -c 'cat< File'
	atf_check -s exit:0 -e empty \
		-o inline:'First Line\nSecond Line\nLine 3\nEND\n' \
		${TEST_SH} -c 'cat < "File"'
	atf_check -s exit:0 -e empty \
		-o inline:'First Line\nSecond Line\nLine 3\nEND\n' \
		${TEST_SH} -c '< File cat'

	ln File wc
	atf_check -s exit:0 -e empty \
		-o inline:'First Line\nSecond Line\nLine 3\nEND\n' \
		${TEST_SH} -c '< wc cat'

	mv wc cat
	atf_check -s exit:0 -e empty -o match:4 \
		${TEST_SH} -c '< cat wc'


	cat <<- 'EOF' |
		i=0
		while [ "$i" -lt 3 ]; do
			i=$((i + 1))
			read line < File
			echo "$line"
		done
	EOF
	atf_check -s exit:0 -e empty \
		-o inline:'First Line\nFirst Line\nFirst Line\n' \
		${TEST_SH}

	cat <<- 'EOF' |
		i=0
		while [ "$i" -lt 3 ]; do
			i=$((i + 1))
			read line
			echo "$line"
		done <File
	EOF
	atf_check -s exit:0 -e empty \
		-o inline:'First Line\nSecond Line\nLine 3\n' \
		${TEST_SH}

	cat <<- 'EOF' |
		i=0
		while [ "$i" -lt 3 ]; do
			i=$((i + 1))
			read line < File
			echo "$line"
		done <File
	EOF
	atf_check -s exit:0 -e empty \
		-o inline:'First Line\nFirst Line\nFirst Line\n' \
		${TEST_SH}

	cat <<- 'EOF' |
		line=
		while [ "$line" != END ]; do
			read line || exit 1
			echo "$line"
		done <File
	EOF
	atf_check -s exit:0 -e empty \
		-o inline:'First Line\nSecond Line\nLine 3\nEND\n' \
		${TEST_SH}

	cat <<- 'EOF' |
		while :; do
			read line || exit 0
			echo "$line"
		done <File
	EOF
	atf_check -s exit:0 -e empty \
		-o inline:'First Line\nSecond Line\nLine 3\nEND\n' \
		${TEST_SH}

	cat <<- 'EOF' |
		i=0
		while read line < File
		do
			echo "$line"
			i=$((i + 1))
			[ ${i} -ge 3 ] && break
		done
		echo DONE
	EOF
	atf_check -s exit:0 -e empty \
		-o inline:'First Line\nFirst Line\nFirst Line\nDONE\n' \
		${TEST_SH}

	cat <<- 'EOF' |
		i=0
		while read line
		do
			echo "$line"
		done <File
		echo DONE
	EOF
	atf_check -s exit:0 -e empty \
		-o inline:'First Line\nSecond Line\nLine 3\nEND\nDONE\n' \
		${TEST_SH}

	cat <<- 'EOF' |
		i=0
		while read line
		do
			echo "$line"
			i=$((i + 1))
			[ ${i} -ge 3 ] && break
		done <File
		echo DONE
	EOF
	atf_check -s exit:0 -e empty \
		-o inline:'First Line\nSecond Line\nLine 3\nDONE\n' ${TEST_SH}

	cat <<- 'EOF' |
		i=0
		while read line1 <File
		do
			read line2
			echo "$line1":"$line2"
			i=$((i + 1))
			[ ${i} -ge 2 ] && break
		done <File
		echo DONE
	EOF
	atf_check -s exit:0 -e empty \
	    -o inline:'First Line:First Line\nFirst Line:Second Line\nDONE\n' \
		${TEST_SH}
}

atf_test_case do_output_redirections
do_output_redirections_head()
{
	atf_set "descr" "Test Output redirections"
}
do_output_redirections_body()
{
nl='
'
	T=0
	i() { T=$(($T + 1)); }

	rm -f Output 2>/dev/null || :
	test -f Output && atf_fail "Unable to remove Output file"
#1
	i; atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c '> Output'
	test -f Output || atf_fail "#$T: Did not make Output file"
#2
	rm -f Output 2>/dev/null || :
	i; atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c '>> Output'
	test -f Output || atf_fail "#$T: Did not make Output file"
#3
	rm -f Output 2>/dev/null || :
	i; atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c '>| Output'
	test -f Output || atf_fail "#$T: Did not make Output file"

#4
	rm -f Output 2>/dev/null || :
	i
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c 'echo Hello >Output'
	test -s Output || atf_fail "#$T: Did not make non-empty Output file"
	test "$(cat Output)" = "Hello" ||
	  atf_fail "#$T: Incorrect Output: Should be 'Hello' is '$(cat Output)'"
#5
	i
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c 'echo Hello>!Output'
	test -s Output || atf_fail "#$T: Did not make non-empty Output file"
	test "$(cat Output)" = "Hello" ||
	  atf_fail "#$T: Incorrect Output: Should be 'Hello' is '$(cat Output)'"
#6
	i
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} -c 'echo Bye >>Output'
	test -s Output || atf_fail "#$T: Removed Output file"
	test "$(cat Output)" = "Hello${nl}Bye" || atf_fail \
	  "#$T: Incorrect Output: Should be 'Hello\\nBye' is '$(cat Output)'"
#7
	i; atf_check -s exit:0 -o inline:'line 1\nline 2\n' -e empty \
		${TEST_SH} -c \
		'echo line 1 > Output; echo line 2 >> Output; cat Output'
	test "$(cat Output)" = "line 1${nl}line 2" || atf_fail \
	 "#$T: Incorrect Output: Should be 'line 1\\nline 2' is '$(cat Output)'"
#8
	i; atf_check -s exit:0 -o inline:'line 2\n' -e empty \
		${TEST_SH} -c 'echo line 1 > Output; echo line 2'
	test "$(cat Output)" = "line 1" || atf_fail \
	    "#$T: Incorrect Output: Should be 'line 1' is '$(cat Output)'"
#9
	i; atf_check -s exit:0 -o empty -e empty \
		${TEST_SH} -c '(echo line 1; echo line 2 > Out2) > Out1'
	test "$(cat Out1)" = "line 1" || atf_fail \
	    "#$T: Incorrect Out1: Should be 'line 1' is '$(cat Out1)'"
	test "$(cat Out2)" = "line 2" || atf_fail \
	    "#$T: Incorrect Out2: Should be 'line 2' is '$(cat Out2)'"
#10
	i; atf_check -s exit:0 -o empty -e empty \
		${TEST_SH} -c '{ echo line 1; echo line 2 > Out2;} > Out1'
	test "$(cat Out1)" = "line 1" || atf_fail \
	    "#$T: Incorrect Out1: Should be 'line 1' is '$(cat Out1)'"
	test "$(cat Out2)" = "line 2" || atf_fail \
	    "#$T: Incorrect Out2: Should be 'line 2' is '$(cat Out2)'"
#11
	i; rm -f Out1 Out2 2>/dev/null || :
	cat <<- 'EOF' |
		for arg in 'line 1' 'line 2' 'line 3'
		do
			echo "$arg"
			echo "$arg" > Out1
		done > Out2
	EOF
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} 
	test "$(cat Out1)" = "line 3" || atf_fail \
		"#$T:  Incorrect Out1: Should be 'line 3' is '$(cat Out1)'"
	test "$(cat Out2)" = "line 1${nl}line 2${nl}line 3" || atf_fail \
    "#$T: Incorrect Out2: Should be 'line 1\\nline 2\\nline 3' is '$(cat Out2)'"
#12
	i; rm -f Out1 Out2 2>/dev/null || :
	cat <<- 'EOF' |
		for arg in 'line 1' 'line 2' 'line 3'
		do
			echo "$arg"
			echo "$arg" >> Out1
		done > Out2
	EOF
	atf_check -s exit:0 -o empty -e empty ${TEST_SH} 
	test "$(cat Out1)" = "line 1${nl}line 2${nl}line 3" || atf_fail \
    "#$T: Incorrect Out1: Should be 'line 1\\nline 2\\nline 3' is '$(cat Out1)'"
	test "$(cat Out2)" = "line 1${nl}line 2${nl}line 3" || atf_fail \
    "#$T: Incorrect Out2: Should be 'line 1\\nline 2\\nline 3' is '$(cat Out2)'"
}

atf_test_case fd_redirections
fd_redirections_head()
{
	atf_set "descr" "Tests redirections to/from specific descriptors"
}
fd_redirections_body()
{
	# Or it will one day...
} 

atf_test_case local_redirections
local_redirections_head()
{
	atf_set "descr" \
	    "Tests that exec can reassign file descriptors in the shell itself"
}
local_redirections_body()
{
	# Or it will one day...
}

atf_test_case redir_in_case
redir_in_case_head()
{
	atf_set "descr" "Tests that sh(1) allows just redirections " \
	                "in case statements. (PR bin/48631)"
}
redir_in_case_body()
{
	atf_check -s exit:0 -o empty -e empty \
	    ${TEST_SH} -c 'case x in (whatever) >foo;; esac'

	atf_check -s exit:0 -o empty -e empty \
	    ${TEST_SH} -c 'case x in (whatever) >foo 2>&1;; esac'

	atf_check -s exit:0 -o empty -e empty \
	    ${TEST_SH} -c 'case x in (whatever) >foo 2>&1 </dev/null;; esac'

	atf_check -s exit:0 -o empty -e empty \
	    ${TEST_SH} -c 'case x in (whatever) >${somewhere};; esac'
}

atf_test_case incorrect_redirections
incorrect_redirections_head()
{
	atf_set "descr" "Tests that sh(1) correctly ignores non-redirections"
}
incorrect_redirections_body() {

	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c 'echo foo>'
	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c 'read foo<'
	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c 'echo foo<>'
	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'echo x > '"$nl"
	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'read x < '"$nl"
	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'echo x <> '"$nl"
	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'echo x >< anything'
	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'echo x >>< anything'
	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'echo x >|< anything'
	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'echo x > ; read x < /dev/null || echo bad'
	atf_check -s not-exit:0 -o empty -e not-empty ${TEST_SH} -c \
		'read x < & echo y > /dev/null; wait && echo bad'

	rm -f Output 2>/dev/null || :
	atf_check -s exit:0 -e empty -o inline:'A Line > Output\n' \
		${TEST_SH} -c 'echo A Line \> Output'
	test -f Output && atf_file "File 'Output' appeared and should not have"

	rm -f Output 2>/dev/null || :
	atf_check -s exit:0 -e empty -o empty \
		${TEST_SH} -c 'echo A Line \>> Output'
	test -f Output || atf_file "File 'Output' not created when it should"
	test "$(cat Output)" = 'A Line >' || atf_fail \
		"Output file contains '$(cat Output)' instead of '"'A Line >'\'

	rm -f Output \> 2>/dev/null || :
	atf_check -s exit:0 -e empty -o empty \
		${TEST_SH} -c 'echo A Line >\> Output'
	test -f Output && atf_file "File 'Output' appeared and should not have"
	test -f '>' || atf_file "File '>' not created when it should"
	test "$(cat '>')" = 'A Line Output' || atf_fail \
	    "Output file ('>') contains '$(cat '>')' instead of 'A Line Output'"
}

# Many more tests in t_here, so here we have just rudimentary checks
atf_test_case redir_here_doc
redir_here_doc_head()
{
	atf_set "descr" "Tests that sh(1) correctly processes 'here' doc " \
	                "input redirections"
}
redir_here_doc_body()
{
	# nb: the printf is not executed, it is data
	cat <<- 'DONE' |
		cat <<EOF
			printf '%s\n' 'hello\n'
		EOF
	DONE
	atf_check -s exit:0 -o match:printf -o match:'hello\\n' \
		-e empty ${TEST_SH} 
}

atf_test_case subshell_redirections
subshell_redirections_head()
{
	atf_set "descr" "Tests redirection interactions between shell and " \
			"its sub-shell(s)"
}
subshell_redirections_body()
{
	# Or will, one day
}

atf_init_test_cases() {
	atf_add_test_case basic_test_method_test
	atf_add_test_case do_input_redirections
	atf_add_test_case do_output_redirections
	atf_add_test_case fd_redirections
	atf_add_test_case local_redirections
	atf_add_test_case incorrect_redirections
	atf_add_test_case redir_here_doc
	atf_add_test_case redir_in_case
	atf_add_test_case subshell_redirections
}
