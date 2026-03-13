#!/bin/bash

# Copyright (C) 2022-2024 Free Software Foundation, Inc.
#
# This file is part of GDB.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Used to generate .xml.in files, like so:
# $ ./update-linux-from-src.sh ~/linux-stable.git

pwd=$(pwd -P)

parse_args ()
{
    if [ $# -lt 1 ]; then
	echo "dir argument needed"
	exit 1
    fi

    d="$1"
    shift

    if [ ! -d "$d" ]; then
	echo "cannot find $d"
	exit 1
    fi
}

gen_from_kernel_headers ()
{
    local f
    f="$1"
    local arch
    arch="$2"

    echo "Generating $f"

    local tmpdir
    tmpdir=$(mktemp -d)
    trap 'rm -Rf $tmpdir/*' EXIT

    local build
    build="$tmpdir"/build
    local install
    install="$tmpdir"/install
    local usr
    usr="$install"/usr
    local include
    include="$usr"/include

    mkdir -p "$build" "$usr"

    (
	cd "$build" || exit 1

	make \
	    -f "$d"/Makefile \
	    ARCH="$arch" \
	    INSTALL_HDR_PATH="$usr" \
	    headers_install \
	    > "$build"/header_install.log \
	    2>&1

	"$pwd"/update-linux.sh \
	    "$pwd"/"$f" \
	    -nostdinc \
	    -isystem "$include"
    )

    trap '' EXIT
    rm -Rf "$tmpdir"
}

pre ()
{
    local f
    f="$1"
    local start_date
    start_date="$2"
    local h
    h="$3"

    local prefix
    prefix="     "

    if [ "$h" != "" ]; then
	file_files="files"
	belong_belongs="belong"
	files=$(echo -e "$prefix$f\n$prefix$h")
    else
	file_files="file"
	belong_belongs="belongs"
	files="$prefix$f"
    fi

    local year
    year=$(date +%Y)

    cat <<EOF
<?xml version="1.0"?>
<!-- Copyright (C) $start_date-$year Free Software Foundation, Inc.

     Copying and distribution of this file, with or without modification,
     are permitted in any medium without royalty provided the copyright
     notice and this notice are preserved.  -->

<!DOCTYPE feature SYSTEM "gdb-syscalls.dtd">

<!-- This file was generated using the following $file_files:

$files

     The $file_files mentioned above $belong_belongs to the Linux Kernel.  -->


EOF

    echo '<syscalls_info>'
}


post ()
{
    echo '</syscalls_info>'
}

one ()
{
    local f
    f="$1"
    local select_abis
    select_abis="$2"
    local start_date
    start_date="$3"
    local offset
    offset="$4"
    local h
    h="$5"

    tmp=$(mktemp)
    trap 'rm -f $tmp' EXIT

    pre "$f" "$start_date" "$h"

    # Print out num, abi, name.
    grep -v "^#" "$d/$f" \
	| awk '{print $1, $2, $3}' \
	      > "$tmp"

    local decimal
    decimal="[0-9][0-9]*"
    # Print out num, "removed", name.
    grep -E "^# $decimal was sys_*" "$d/$f" \
	| awk '{print $2, "removed", gensub("^sys_", "", 1, $4)}' \
	      >> "$tmp"

    case $h in
	arch/arm/include/uapi/asm/unistd.h)
	    grep '#define __ARM_NR_[a-z].*__ARM_NR_BASE\+' "$d/$h" \
		| sed 's/#define //;s/__ARM_NR_BASE+//;s/[()]//g;s/__ARM_NR_/ARM_/' \
		| awk '{print $2 + 0x0f0000, "private", $1}' \
		      >> "$tmp"
	    ;;
    esac

    local nums
    declare -a nums
    local abis
    declare -a abis
    local names
    declare -a names
    local name_exists
    declare -A name_exists

    local i
    i=0
    local _num
    local _abi
    local _name
    while read -r -a line; do
	_num="${line[0]}"
	_abi="${line[1]}"
	_name="${line[2]}"

	local found
	found=false
	for a in $select_abis; do
	    if [ "$a" = "$_abi" ]; then
		found=true
		break
	    fi
	done
	if ! $found; then
	    continue
	fi

	if [ "${_name/reserved+([0-9])/}" = "" ]; then
	    continue
	fi

	if [ "${_name/unused+([0-9])/}" = "" ]; then
	    continue
	fi

	nums[i]="$_num"
	abis[i]="$_abi"
	names[i]="$_name"

	if [ "$_abi" != "removed" ]; then
	    name_exists[$_name]=1
	fi

	i=$((i + 1))
    done < <(sort -V "$tmp")

    local n
    n=$i

    for ((i = 0 ; i < n ; i++)); do
	_name=${names[$i]}
	_abi=${abis[$i]}
	_num=$((${nums[$i]} + offset))

	if [ "$_abi" = "removed" ] && [ "${name_exists[$_name]}" = 1 ]; then
	    _name=old$_name
	fi

	echo -n "  <syscall name=\"$_name\" number=\"$_num\"/>"

	if [ "$_abi" = "removed" ]; then
	    echo " <!-- removed -->"
	else
	    echo
	fi
    done

    post
}

regen ()
{
    local f
    f="$1"

    local start_date
    start_date=2009
    local offset
    offset=0
    local h
    h=

    local t
    local abi
    case $f in
	amd64-linux.xml.in)
	    t="arch/x86/entry/syscalls/syscall_64.tbl"
	    abi="common 64"
	    ;;
	i386-linux.xml.in)
	    t="arch/x86/entry/syscalls/syscall_32.tbl"
	    abi=i386
	    ;;
	ppc64-linux.xml.in)
	    t="arch/powerpc/kernel/syscalls/syscall.tbl"
	    abi="common 64 nospu"
	    ;;
	ppc-linux.xml.in)
	    t="arch/powerpc/kernel/syscalls/syscall.tbl"
	    abi="common 32 nospu"
	    ;;
	s390-linux.xml.in)
	    t="arch/s390/kernel/syscalls/syscall.tbl"
	    abi="common 32"
	    ;;
	s390x-linux.xml.in)
	    t="arch/s390/kernel/syscalls/syscall.tbl"
	    abi="common 64"
	    ;;
	sparc64-linux.xml.in)
	    t="arch/sparc/kernel/syscalls/syscall.tbl"
	    abi="common 64"
	    start_date="2010"
	    ;;
	sparc-linux.xml.in)
	    t="arch/sparc/kernel/syscalls/syscall.tbl"
	    abi="common 32"
	    start_date="2010"
	    ;;
	mips-n32-linux.xml.in)
	    t="arch/mips/kernel/syscalls/syscall_n32.tbl"
	    abi="n32"
	    start_date="2011"
	    offset=6000
	    ;;
	mips-n64-linux.xml.in)
	    t="arch/mips/kernel/syscalls/syscall_n64.tbl"
	    abi="n64"
	    start_date="2011"
	    offset=5000
	    ;;
	mips-o32-linux.xml.in)
	    t="arch/mips/kernel/syscalls/syscall_o32.tbl"
	    abi="o32"
	    start_date="2011"
	    offset=4000
	    ;;
	bfin-linux.xml.in)
	    echo "Skipping $f, no longer supported"
	    return
	    ;;
	aarch64-linux.xml.in)
	    # No syscall.tbl.
	    gen_from_kernel_headers "$f" arm64
	    return
	    ;;
	arm-linux.xml.in)
	    t="arch/arm/tools/syscall.tbl"
	    h="arch/arm/include/uapi/asm/unistd.h"
	    abi="common eabi oabi removed private"
	    ;;
	loongarch-linux.xml.in)
	    echo "Skipping $f, no syscall.tbl"
	    return
	    ;;
	linux-defaults.xml.in)
	    return
	    ;;
	*)
	    echo "Don't know how to generate $f"
	    return
	    ;;
    esac

    echo "Generating $f"
    one "$t" "$abi" "$start_date" "$offset" "$h" > "$f"
}

main ()
{
    shopt -s extglob

    # Set global d.
    parse_args "$@"

    local f
    for f in *.xml.in; do
	regen "$f"
    done
}

main "$@"
