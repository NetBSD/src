# $NetBSD: t_mtree.sh,v 1.4.2.2 2012/04/17 00:09:23 yamt Exp $
#
# Copyright (c) 2009, 2012 The NetBSD Foundation, Inc.
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

# Postprocess mtree output, canonicalising portions that
# are expected to differ from one run to another.
#
h_postprocess()
{
	sed -e '
		/^#	   user: /s/:.*/: x/
		/^#	machine: /s/:.*/: x/
		/^#	   tree: /s/:.*/: x/
		/^#	   date: /s/:.*/: x/
		'
}

h_check()
{
        diff -Nru "$1" "$2" || atf_fail "files $1 and $2 differ"
}

atf_test_case create
create_head()
{
	atf_set "descr" "Create a specfile describing a directory tree"
}

create_setup()
{
	# create some directories
	mkdir -p create/a/1 create/a/2 create/b
	# create some files
	for file in create/top.file.1 \
		    create/a/a.file.1 \
		    create/a/a.file.2 \
		    create/a/1/a1.file.1 \
		    create/b/b.file.1 \
		    create/b/b.file.2
	do
		echo "$file" >$file
	done
	# hard link to file in same dir
	ln create/b/b.file.1 create/b/b.hardlink.1
	# hard link to file in another dir
	ln create/b/b.file.2 create/a/a.hardlink.b2
	# symlink to file
	ln -s a.file.1 create/a.symlink.1
	# symlink to dir
	ln -s b create/top.symlink.b
	# dangling symlink
	ln -s nonexistent create/top.dangling
}

create_body()
{
	create_setup

	# run mtree and check output
	( cd create && mtree -c -k type,nlink,link,size,sha256 ) >output.raw \
	|| atf_fail "mtree exit status $?"
	h_postprocess <output.raw >output
	h_check "$(atf_get_srcdir)/d_create.out" output
}

atf_test_case check
check_head()
{
	atf_set "descr" "Check a directory tree against a specfile"
}

check_body()
{
	# we use the same directory tree and specfile as in the "create" test
	create_setup

	# run mtree and check output
	( cd create && mtree ) <"$(atf_get_srcdir)/d_create.out" >output \
	|| atf_fail "mtree exit status $?"
	h_check /dev/null output
}

atf_test_case convert_C
convert_C_head()
{
	atf_set "descr" "Convert a specfile to mtree -C format, unsorted"
}

convert_C_body()
{
	mtree -C -K all <"$(atf_get_srcdir)/d_convert.in" >output
	h_check "$(atf_get_srcdir)/d_convert_C.out" output
}

atf_test_case convert_C_S
convert_C_S_head()
{
	atf_set "descr" "Convert a specfile to mtree -C format, sorted"
}

convert_C_S_body()
{
	mtree -C -S -K all <"$(atf_get_srcdir)/d_convert.in" >output
	h_check "$(atf_get_srcdir)/d_convert_C_S.out" output
}

atf_test_case convert_D
convert_D_head()
{
	atf_set "descr" "Convert a specfile to mtree -D format, unsorted"
}

convert_D_body()
{
	mtree -D -K all <"$(atf_get_srcdir)/d_convert.in" >output
	h_check "$(atf_get_srcdir)/d_convert_D.out" output
}

atf_test_case convert_D_S
convert_D_S_head()
{
	atf_set "descr" "Convert a specfile to mtree -D format, sorted"
}

convert_D_S_body()
{
	mtree -D -S -K all <"$(atf_get_srcdir)/d_convert.in" >output
	h_check "$(atf_get_srcdir)/d_convert_D_S.out" output
}

atf_test_case ignore
ignore_head()
{
	atf_set "descr" "Test that -d ignores symlinks (PR bin/41061)"
}

ignore_body()
{
	mkdir newdir
	mtree -c | mtree -Ck uid,gid,mode > mtree.spec
	ln -s newdir otherdir

	# This yields "extra: otherdir" even with -d.
	# (PR bin/41061)
	atf_check -s ignore -o empty -e empty -x "mtree -d < mtree.spec"

	# Delete the symlink and re-verify.
	#
	rm otherdir
	atf_check -s ignore -o empty -e empty -x "mtree -d < mtree.spec"
}

atf_test_case merge
merge_head()
{
	atf_set "descr" "Merge records of different type"
}

merge_body()
{
	mtree -C -M -K all <"$(atf_get_srcdir)/d_merge.in" >output
	h_check "$(atf_get_srcdir)/d_merge_C_M.out" output
	# same again, with sorting
	mtree -C -M -S -K all <"$(atf_get_srcdir)/d_merge.in" >output
	h_check "$(atf_get_srcdir)/d_merge_C_M_S.out" output
}

atf_test_case nonemptydir
nonemptydir_head()
{
	atf_set "descr" "Test that new non-empty " \
			"directories are recorded (PR bin/25693)"
}

nonemptydir_body()
{
	mkdir testdir
	cd testdir

	mtree -c > mtree.spec

	if [ ! -f mtree.spec ]; then
		atf_fail "mtree failed"
	fi

	touch bar
	atf_check -s ignore -o save:output -x "mtree -f mtree.spec"

	if [ ! -n "$(egrep "extra: bar" output)" ]; then
		atf_fail "mtree did not record changes (PR bin/25693)"
	fi
}

atf_init_test_cases()
{
	atf_add_test_case create
	atf_add_test_case check
	atf_add_test_case convert_C
	atf_add_test_case convert_C_S
	atf_add_test_case convert_D
	atf_add_test_case convert_D_S
	atf_add_test_case ignore
	atf_add_test_case merge
	atf_add_test_case nonemptydir
}
