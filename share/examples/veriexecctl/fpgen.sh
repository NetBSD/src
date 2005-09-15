#!/bin/sh
#
#	$NetBSD: fpgen.sh,v 1.3.2.2 2005/09/15 20:39:12 tron Exp $	
#
# Copyright 2005 Brett Lymn <blymn@netbsd.org>
# Copyright 2005 Elad Efrat <elad@netbsd.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
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
#
# This script will scan FFS filesystems for files, adding digital
# fingerprints of files to a text database. It also makes sure that the
# same file will not be added twice - ie., hardlinks.
#
# Options:
#   -d <dir>	start scanning from <dir> (absolute path)
#   -r		do recursive scanning
#   -a		scan all files (not only executables)
#   -q		be quiet (no output)
#
# Usage pattern: fpgen.sh [flags] <hash> <database file>
# Examples:
#   ./fpgen.sh -r md5 vexec.conf (scan recursively from current dir)
#   ./fpgen.sh -ar sha1 vexec.conf (all files, recursively from curent dir)
#   ./fpgen.sh -d /bin rmd160 vexec.conf (only executables in /bin)

DIGEST="/usr/pkg/bin/digest"
CKSUM="/usr/bin/cksum"

# Get inode of given file.
inode() {
	local _inode=`/bin/ls -li $1 | /usr/bin/cut -d' ' -f1`
	echo "$_inode"
}

# Go through the provided file, getting filenames. (first field)
# For each filename, get its inode, and compare with the inode we want
# to add.
# Returns 0 if we can add it or 1 if it's already added.
check_dup() {
	local _file="$1"
	local _inode="$2"
	local _ret="0"

	while read line; do
		if [ ${line} ]; then
			_ret="1"
			#entry=`echo $line | /usr/bin/cut -d' ' -f1`
			#entry_inode=`inode $entry`
			#if [ "${_inode}" = "${entry_inode}" ]; then
			#	_ret="1"
			#	break
			#fi
		fi
	done < $_file

	echo "$_ret"
}

# End of subroutines.

# Defaults
find_pattern="-perm -0100 -or -perm -0010 -or -perm -0001"
recurse="-maxdepth 1"
rootdir="$PWD"
howdeep="non-recursive"
scanwhat="executables only"
quiet="0"

# Sanity
if [ $# -lt 2 -o $# -gt 6 ]; then
	echo "Usage: $0 [-a|-r|-d <dir>] [-q] <hash_type> <fingerprint_file>"
	echo "(a)ll files; (r)ecursive scan; (d)irectory root."
	echo ""
	echo "$0 will use the command \"digest\" from pkgsrc, if available"
	echo "or use the cksum command if \"digest\" is not available"
	echo "see the man pages for cksum and digest for the supported"
	echo "hash types.  Hash type names are of the form md5, sha1,"
	echo "rmd160 and so forth"
	exit 2
fi

# Parse arguments: Build search pattern and set root dir.
set -- `getopt qard: $*`
for i do
	case "$i" in
		-a) scanwhat="all files"; find_pattern="! -perm 0000"; shift;;
		-d) rootdir=$2; shift; shift;;
		-r) howdeep="recursive"; recurse=; shift;;
		-q) quiet="1"; shift;;
	esac
done

# Set hash type and command (ghetto) and output file
hashtype=$2;
dbfile=$3;

# Make sure we have a hash type
if [ -z "${hashtype}" ]; then
	echo "ERROR: No hash algorithm: Aborting."
	exit;
fi

# Make sure we have a target file.
if [ -z "${dbfile}" ]; then
	echo "ERROR: No DB file. Aborting."
	exit
fi

# If we have digest, we can use it for all algorithms. If we don't, we
# fallback to cksum supporting only MD5, SHA1, and RMD160.
if [ -e ${DIGEST} ]; then
	hashcmd="${DIGEST} ${hashtype}"
	echo "foo foo foo" | ${hashcmd} > /dev/null 2>&1
	if [ "$?" != 0 ]; then
		echo "ERROR: ${DIGEST} does not support the hash ${hashtype}"
		exit 1
	fi
else
	if [ ${quiet} = "0" ]; then
		echo "WARNING: No digest; Falling back to cksum."
	fi

	if [ ! -e ${hashcmd_cksum} ]; then
		echo "ERROR: No cksum! Aborting."
	fi

	cksum_flag="no"
	case "$hashtype" in
	md5)
		cksum_flag="-5"
		;;

	sha1)
		cksum_flag="-1"
		;;

	rmd160)
		cksum_flag="-6"
		;;
	esac

	if [ "${cksum_flag}" = "no" ]; then
		echo "ERROR: Unsupported hashing algorithm. Use 'md5'," \
		"'sha1', or 'rmd160' only when using cksum."
		exit 1
	fi

	hashcmd="${CKSUM} ${cksum_flag}"
fi

if [ "${quiet}" = "0" ]; then
	echo "Verified Exec Fingerprint Generator, v0.1"
	echo
	echo "Command : \"${hashcmd}\""
	echo "Root dir: \"${rootdir}\" [${howdeep}]"
	echo "DB file : \"${dbfile}\""
	echo "Mode    : ${scanwhat}"
	echo
fi

# Backup existing file (if got one)
if [ -e "${dbfile}" ]; then
	if [ "${quiet}" = "0" ]; then
		echo "NOTE: Backing up existing fingerprint file to \"${dbfile}.old\"."
		echo
	fi

	mv ${dbfile} ${dbfile}.old
fi

echo "# Generated by `whoami`, `date`, for ${rootdir}" > ${dbfile}

# Create temporary file.
done="0"
tmpfile=`mktemp /tmp/vexecctl.XXXXXXXXXX`

# XXX: egrep -v ^/kern
# XXX: do we need the lib*so* thing?
find $rootdir $recurse \( \( -fstype ffs -and -type f -and \( $find_pattern \) \) \
-or -name "lib*so*" \) | egrep -v '^/proc.*' > $tmpfile

while read file; do
	hash=`${hashcmd} < ${file} 2>/dev/null`
	flag=""

	if [ -z "${hash}" ]; then
		continue
	fi

	# Non-executables need the "FILE" flag
	if [ ! -x $file ]; then
		flag=" FILE"
	fi

	# Check if already in database under a different name. (hardlink)
	_ret="0"
	_inode=`inode $file`
	while read line; do
		line2=`echo ${line} | awk -F# '{print $1}'`
		if [ -n "${line2}" ]; then
			entry=`echo ${line2} | /usr/bin/cut -d' ' -f1`
			entry_inode=`inode ${entry}`
			if [ X"${_inode}" = X"${entry_inode}" ]; then
				_ret="1"
				break
			fi
		fi
	done < $dbfile

	# Add to the database.
	if [ X"${_ret}" = X"0" -a -n "${hash}" -a -n "${file}" ]; then
		if [ $quiet = "0" ]; then
			echo "Adding $file."
		fi

		echo "$file ${hashtype} ${hash}${flag}" >> $dbfile
	fi
done < $tmpfile

echo >> ${dbfile}

if [ $quiet = "0" ]; then
	echo
	echo "##################################################################"
	echo "    PLEASE VERIFY CONTENTS OF $dbfile AND SET THE \"INDIRECT\"    "
	echo "    FLAG WHERE APPROPRIATE (HINT: INTERPRETERS SUCH AS PERL).   "
	echo "##################################################################"
fi

rm -f ${tmpfile}
