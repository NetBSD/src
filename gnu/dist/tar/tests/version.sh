#! /bin/sh
# Check if the proper version is being tested.

. ./preset
PATH=../src:$PATH

if test -n "`$PACKAGE --version | sed -n s/$PACKAGE.*$VERSION/OK/p`"; then
  banner="Regression testing for GNU $PACKAGE, version $VERSION"
  dashes=`echo $banner | sed s/./=/g`
  echo $dashes
  echo $banner
  echo $dashes
else
  echo '=============================================================='
  echo 'WARNING: Not using the proper version, *all* checks dubious...'
  echo '=============================================================='
  exit 1
fi
