#! /bin/sh
# Check for fnmatch problems in glibc 2.1.95.

. ./preset
. $srcdir/before

set -e
touch file1
mkdir directory
mkdir directory/subdirectory
touch directory/file1
touch directory/file2
touch directory/subdirectory/file1
touch directory/subdirectory/file2
tar -cf archive ./file1 directory
tar -tf archive \
  --exclude='./*1' \
  --exclude='d*/*1' \
  --exclude='d*/s*/*2' | sort

out="\
directory/
directory/file2
directory/subdirectory/
"

. $srcdir/after
