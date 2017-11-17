#! /bin/sh
#

set -e

# Try to synchronize the file system in case $scp_host and $host are
# different (NFS complains about stale file handles and whatnot
# without this).
sync
sleep 1
sync

. ./build-params.sh

# Make a unique temporary directory name.
hostname=`hostname`
tmpdir=build-tmp-$dir-$hostname-$$

rm -rf $tmpdir
mkdir $tmpdir
cp $pkg $tmpdir
cd $tmpdir

gunzip -c $pkg | tar xf -
cd $dir

./tests/build-tests.sh
