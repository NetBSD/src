#! /bin/sh

# $NetBSD: linkfarm.sh,v 1.1.2.4 2003/07/13 20:38:55 jlam Exp $

#
# Copyright (c) 2002 Alistair G. Crooks.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by Alistair G. Crooks.
# 4. The name of the author may not be used to endorse or promote
#    products derived from this software without specific prior written
#    permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# set -x

# set up program definitions
findprog=/usr/bin/find
grepprog=/usr/bin/grep
lnprog=/bin/ln
mkdirprog=/bin/mkdir
rmprog=/bin/rm
rmdirprog=/bin/rmdir
sedprog=/usr/bin/sed
sortprog=/usr/bin/sort

usage() {
	echo 'Usage: linkfarm [options] package'
	exit 1
}

version() {
	echo "20020419"
	exit 0
}

ignorefiles=${PLIST_IGNORE_FILES:-info/dir}
linktype=-s

# default action: create a linkfarm in $target from $stowdir/$1
# i.e. linkfarm --target=${prefix}/${view} --dir=${prefix}/packages $1
doit=""
target=${LOCALBASE:-/usr/pkg}
stowdir=${target}/packages
verbose=0

# default action is to create
check=no
delete=no
create=yes

# process args - can't use getopt(1) because of '--' style args
while [ $# -gt 0 ]; do
	case "$1" in
	-D)		delete=yes; create=no ;;
	-R)		delete=yes; create=yes ;;
	-V)		version ;;
	-c)		check=yes; doit=":" ;;
	-d)		stowdir=$2; shift ;;
	-d*)		stowdir=`echo $1 | $sedprog -e 's|-d||'` ;;
	-t)		target=$2; shift ;;
	-t*)		target=`echo $1 | $sedprog -e 's|-t||'` ;;
	-n)		doit=":" ;;
	-v)		verbose=`expr $verbose + 1` ;;

	--delete)	delete=yes; create=no ;;
	--dir=*)	stowdir=`echo $1 | $sedprog -e 's|--dir=||'` ;;
	--restow)	delete=yes; create=yes ;;
	--target=*)	target=`echo $1 | $sedprog -e 's|--target=||'` ;;
	--version)	version ;;

	--)		shift; break ;;
	*)		break ;;
	esac
	shift
done

# make sure stowdir has a full pathname
case $stowdir in
/*)	;;
*)	stowdir=`pwd`/$stowdir ;;
esac

# make sure target has a full pathname
case $target in
/*)	;;
*)	target=`pwd`/$target ;;
esac

# set the package name
package=$1

# if we're checking the entries, check, then exit
case $check in
yes)
	(cd $stowdir/$package; 
	ex=0;
	for f in `$findprog . ! -type d -print`; do
		newf=`echo $f | $sedprog -e 's|^\./||'`
		if [ -e $target/$newf ]; then
			ignore=no
			for i in $ignorefiles; do
				if [ $i = $newf ]; then
					ignore=yes
					break
				fi
			done
			case $ignore in
			no)	
				echo "${newf}"; ex=1 ;;
			esac
		fi
	done
	exit $ex) || exit 1
	;;
esac

# if we need to get rid of old linkfarms, do it
case $delete in
yes)	
	(cd $stowdir/$package;
	for f in `$findprog . ! -type d -print`; do
		newf=`echo $f | $sedprog -e 's|^\./||'`
		ignore=no
		for i in $ignorefiles; do
			if [ $i = $newf ]; then
				ignore=yes
				break
			fi
		done
		case $ignore in
		no)	
			if [ $verbose -gt 0 ]; then
				echo "$rmprog $target/$newf"
			fi
			$doit $rmprog $target/$f ;;
		esac
	done
	for d in `$findprog . -type d -print | $sortprog -r`; do
		if [ $verbose -gt 0 ]; then
			echo "$rmdirprog $target/$d"
		fi
		$doit $rmdirprog $target/$d > /dev/null 2>&1
	done)
	;;
esac

# if we need to create new linkfarms, do it
case $create in
yes)
	(cd $stowdir/$package; 
	for d in `$findprog . -type d -print`; do
		newd=`echo $d | $sedprog -e 's|^\./||'`
		case "$d" in
		"")	continue ;;
		esac
		if [ $verbose -gt 0 ]; then
			echo "$mkdirprog -p $target/$newd"
		fi
		$doit $mkdirprog -p $target/$newd > /dev/null 2>&1
	done
	for f in `$findprog . ! -type d -print`; do
		newf=`echo $f | $sedprog -e 's|^\./||'`
		ignore=no
		for i in $ignorefiles; do
			if [ $i = $newf ]; then
				ignore=yes
				break
			fi
		done
		case $ignore in
		no)
			if [ $verbose -gt 0 ]; then
				echo "$lnprog ${linktype} $stowdir/$package/$newf $target/$newf"
			fi
			$doit $lnprog ${linktype} $stowdir/$package/$newf $target/$f ;;
		esac
	done)
	;;
esac

exit 0
