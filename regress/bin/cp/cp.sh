#! /bin/sh
#
# $Id: cp.sh,v 1.4 2007/07/15 09:23:53 jmmv Exp $
#
# Test cp(1)

FILES="file file2 file3 link dir dir2 dirlink target"

cleanup() {
        rm -fr ${FILES}
}

error() {
	if [ ! -z "$2" ]; then
		msg=": $2"
	fi
	echo Failed $1$msg
	exit 1
}

cp_compare() {
	cmp $2 $3
	if [ $? -gt 0 ]; then
		error $1 "$2 not the same as $3"
	fi
}

reset() {
	cleanup
	echo "I'm a file" > file
	echo "I'm a file, 2" > file2
	echo "I'm a file, 3" > file3
	ln -s file link
	mkdir dir
	ln -s dir dirlink
}

file_to_file() {
	reset

	file_to_file_simple
	file_to_file_preserve
	file_to_file_noflags
}

file_to_file_simple() {
	rm -f file2
	umask 022
	chmod 777 file
	cp file file2 || error file_to_file_simple
	cp_compare file_to_file_simple file file2
	if [ `stat -f "%Lp" file2` != "755" ]; then
		error file_to_file_simple "new file not created with umask"
	fi

	chmod 644 file
	chmod 777 file2
	cp_compare file_to_file_simple file file2
	if [ `stat -f "%Lp" file2` != "777" ]; then
		error file_to_file_simple "existing files permissions not retained"
	fi
	
}

file_to_file_preserve() {
	rm file3
	chmod 644 file
	chflags nodump file
	cp -p file file3 || error file_to_file_preserve
	finfo=`stat -f "%p%u%g%m%z%f" file`
	f3info=`stat -f "%p%u%g%m%z%f" file3`
	if [ $finfo != $f3info ]; then
		error file_to_file_preserve "attributes not preserved"
	fi
}

file_to_file_noflags() {
	rm file3
	chmod 644 file
	chflags nodump file
	cp -p -N file file3 || error file_to_file_noflags
	finfo=`stat -f "%f" file`
	f3info=`stat -f "%f" file3`
	if [ $finfo = $f3info ]; then
		error file_to_file_noflags "-p -N preserved file flags"
	fi
}

file_to_link() {
	reset
	cp file2 link || error file_to_link
	cp_compare file_to_link file file2
}

link_to_file() {
	reset
	# file and link are identical (not copied).
	cp link file 2>/dev/null && 	\
		error link_to_file "should have failed, but didn't"
	cp link file2 || error link_to_file
	cp_compare link_to_file file file2
}

file_over_link() {
	reset
	cp -P file link || error file_over_link
	cp_compare file_over_link file link
}

link_over_file() {
	reset
	cp -P link file || error link_over_file
	if [ `readlink link` != `readlink file` ]; then
		error link_over_file 
	fi
}

files_to_dir() {
	reset
	# can't copy multiple files to a file
	cp file file2 file3 2>/dev/null && \
		error files_to_dir "should have failed, but didn't"
	cp file file2 link dir || error files_to_dir
	cp_compare files_to_dir file "dir/file"
}

dir_to_file() {
	reset
	# can't copy a dir onto a file
	cp dir file 2>/dev/null &&	\
		error dir_to_file "should have failed, but didn't"
	cp -R dir file 2>/dev/null &&	\
		error dir_to_file "should have failed, but didn't"
}

file_to_linkdir() {
	reset
	cp file dirlink || error file_to_linkdir
	cp_compare file_to_linkdir file "dir/file"

	# overwrite the link
	cp -P file dirlink || error file_to_linkdir "with -P"
	readlink dirlink && error file_to_linkdir "-P didn't overwrite the link"
	cp_compare file_to_linkdir file dirlink
}

linkdir_to_file() {
	reset
	cp dirlink file 2>/dev/null
	if [ $? = 0 ]; then
		error linkdir_to_file "copied a dir onto a file"
	fi

	# overwrite the link
	cp -P dirlink file || error linkdir_to_file "with -P"
	if [ `readlink file` != `readlink dirlink` ]; then
		error linkdir_to_file "link not copied"
	fi
}

dir_to_dne_no_R() {
	cp dir dir2 2>/dev/null &&	\
		error dir_to_dne_no_R "should have failed, but didn't"
}

dir_to_dne() {
	cp -R dir dir2 || error dir_do_dne "in dir_to_dne"
	cp_compare dir_to_dne "dir/file" "dir2/file"
	readlink dir2/link >/dev/null
	if [ $? -gt 0 ]; then
		error dir_to_dne "-R didn't copy a link as a link"
	fi
}

dir_to_dir_H() {
	dir_to_dir_setup
	cp -R dir dir2

	chmod 777 dir

	# copy a dir into a dir, only command-line links are followed
	cp -R -H dirlink dir2 || error dir_do_dir "in dir_to_dir_H"
	cp_compare dir_to_dir_H "dir/file" "dir2/dirlink/file"
	readlink dir2/dirlink/link >/dev/null
	if [ $? -gt 0 ]; then
		error dir_to_dir_H "didn't copy a link as a link"
	fi

	# Created directories have the same mode as the corresponding
        # source directory, unmodified by the process's umask.
	if [ `stat -f "%Lp" dir2/dirlink` != "777" ]; then
		error dir_to_dir_H "-R modified dir perms with umask"
	fi
}

dir_to_dir_L() {
	dir_to_dir_setup
	cp -R dir dir2
	cp -R -H dirlink dir2

	# copy a dir into a dir, following all links
	cp -R -H -L dirlink dir2/dirlink || error dir_do_dir "in dir_to_dir_L"
	cp_compare dir_to_dir_L "dir/file" "dir2/dirlink/dirlink/file"
	readlink dir2/dirlink/dirlink/link >/dev/null &&	\
		error dir_to_dir_L "-R -L copied a link as a link"
}

dir_to_dir_subdir_exists() {
	# recursively copy a dir into another dir, with some subdirs already
	# existing
	cleanup

	mkdir -p dir/1 dir/2 dir/3 target/2
	echo "file" > dir/2/file
	cp -R dir/* target || error dir_to_dir "in dir_to_dir_subdir_exists"
	cp_compare dir_to_dir_subdir_exists "dir/2/file" "target/2/file"
}

dir_to_dir_setup() {
	reset
	umask 077
	cp -P file file2 file3 link dir
}

dir_to_dir() {
	dir_to_dir_setup
	dir_to_dne_no_R
	dir_to_dne
	dir_to_dir_H
	dir_to_dir_L
	dir_to_dir_subdir_exists
}


trap cleanup 0 1 2 3 15

file_to_file
file_to_link
link_to_file
file_over_link
link_over_file
files_to_dir
file_to_linkdir
linkdir_to_file
dir_to_file
dir_to_dir

cleanup
exit 0
