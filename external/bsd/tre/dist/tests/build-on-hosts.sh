#! /bin/sh

set -e

if test -z "$BUILD_HAVE_SOURCES"; then
  echo -n "Building source code distribution..."
  ./utils/build-sources.sh > /dev/null
  echo "OK"
fi

if test -z "$*"; then
  for f in tests/build-hosts/*; do
    if test -f $f; then
      hosts="$hosts `basename $f`"
    fi
  done
else
  hosts="$*"
fi

for hostfile in $hosts; do
  (
  . tests/build-hosts/$hostfile
  pkg=`ls dist/tre-*.tar.gz`
  pkg=`basename $pkg`
  dir=`basename $pkg .tar.gz`
  if test -z "$scp_host"; then
    echo foo
    scp_host="$host"
  fi

  cat tests/build-hosts/$hostfile > $hostfile.tmp
  echo "pkg=$pkg" >> $hostfile.tmp
  echo "dir=$dir" >> $hostfile.tmp
  chmod +x $hostfile.tmp

  echo "Copying files to $scp_host..."
  scp dist/$pkg tests/build-run.sh $scp_host:build-tmp
  scp $hostfile.tmp $scp_host:build-tmp/build-params.sh
  rm -f $hostfile.tmp
  echo "Starting job on $host..."
  ( ssh $host "cd build-tmp; ./build-run.sh";
    if test $? -ne 0; then
      echo "$host: FAILED:"
      exit 1
    fi )
  ) &
done
wait
