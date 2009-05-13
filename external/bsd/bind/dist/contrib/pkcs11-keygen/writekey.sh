#!/bin/bash --debug

usage="Usage: $0 -x ext -p pin -f keyrootname"
tmp_file=/tmp/cur_pem.$$
while getopts ":x:p:f:" opt; do
  case $opt in
    x  ) ext=$OPTARG ;;
    p  ) pin=$OPTARG ;;
    f  ) root=$OPTARG ;;
    \? ) echo $usage 
	 exit 1 ;;
   esac
done
shift $(($OPTIND -1))

if [ ! "$ext" -o ! "$pin" -o ! "$root" ] ; then
  echo $usage
  exit 1
fi

keyfile=${root}.key
privfile=${root}.private
file=`basename $root | sed 's/^K//'`
zone=`echo $file | awk -F+ '{ print $1 }' | sed 's/\.$//'`
algo=`echo $file | awk -F+ '{ print $2 }'`
tag=`echo $file | awk -F+ '{ print $3 }'`

# debug
echo 'zone: ' $zone
echo 'algo: ' $algo
echo 'tag: ' $tag

if [ ! -r "$keyfile" ] ; then
  echo "can't read " $keyfile
  exit 1
fi
if [ ! -r "$privfile" ] ; then
  echo "can't read " $privfile
  exit 1
fi

if [ "$algo" != "005" ] ; then
  echo 'algorithm must be 005'
  exit 1
fi

# for testing
mypath=.

echo 'Reading key files'
flag=`$mypath/keydump.pl -k $keyfile -p $privfile -o $tmp_file`

if [ "$flag" = "256" ] ; then
  label=$zone,zsk,$ext
elif [ "$flag" = "257" ] ; then
  label=$zone,ksk,$ext
else
  echo 'flag must be 256 or 257'
  rm $tmp_file
  exit 1
fi

echo "Label will be '"$label"'"
$mypath/writekey -p $pin -l $label -i $tag -f $tmp_file

rm $tmp_file

echo 'Now you can add at the end of ' $privfile
/usr/bin/perl <<EOF
use MIME::Base64;
print "Engine: ", encode_base64("pkcs11\0",""), "\n";
print "Label: ", encode_base64("pkcs11:"."$label"."\0",""), "\n";
EOF
