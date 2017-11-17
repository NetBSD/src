#! /bin/sh
#
# This script gets the version number from the configure.ac file and
# fills it in to other places where the version number is needed but
# cannot be filled in by the configure script.
#

version=`grep AC_INIT configure.ac | cut -d , -f 2 | cut -d ')' -f 1`
version=`echo $version`
version_1=`echo $version | cut -d . -f 1`
version_2=`echo $version | cut -d . -f 2`
version_3=`echo $version | cut -d . -f 3`

for file in python/setup.py \
            win32/tre-config.h; do
  cp $file.in $file.tmp
  for replace in @TRE_VERSION@:$version \
                 @TRE_VERSION_1@:$version_1 \
                 @TRE_VERSION_2@:$version_2 \
                 @TRE_VERSION_3@:$version_3; do
     var=`echo $replace | cut -d : -f 1`
     val=`echo $replace | cut -d : -f 2`
     echo "Replacing $var by $val to $file"
     cat $file.tmp \
       | sed "s/$var/$val/g" \
       > $file.tmp2
     mv $file.tmp2 $file.tmp
  done
  mv $file.tmp $file
done
