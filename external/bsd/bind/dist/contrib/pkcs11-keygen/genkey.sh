#!/usr/bin/bash

usage="Usage: $0 -z zone -x ext -p pin -b bits -e engine [-f] -k key_path"
tmp_file=/tmp/cur_key.$$
while getopts ":z:x:p:t:k:b:e:f" opt; do
  case $opt in
    z  ) zone=$OPTARG ;;
    x  ) ext=$OPTARG ;;
    p  ) pin=$OPTARG ;;
    t  ) id=$OPTARG ;;
    f  ) flag="ksk" ;;
    e  ) engine=$OPTARG ;;
    b  ) bits=$OPTARG ;;
    k  ) key_path=$OPTARG ;;
    \? ) echo $usage 
	 exit 1 ;;
   esac
done
shift $(($OPTIND -1))

if [ ! "$zone" -o ! "$ext" -o ! "$pin" -o ! "$engine" -o ! "$bits" -o ! "$key_path" ] ; then
  echo $usage
  exit 1
fi

if [ "$flag" ] ; then
  label="$zone,$flag,$ext"
else
  label="$zone,zsk,$ext"
fi

# for testing
mypath=.

echo "Generating key"
$mypath/genkey -b $bits -l $label -p $pin
if [ $? -ne 0 ] ; then exit 1 ; fi

echo "Exporting public key"
$mypath/PEM_write_pubkey -e $engine -p $pin -k pkcs11:$label -f $tmp_file
if [ $? -ne 0 ] ; then exit 1 ; fi

echo "Generating DNSKEY RR"
if [ "$flag" ] ; then
  keytag=`$mypath/keyconv.pl -a 5 -k -e $engine -l $label -p $key_path -i $tmp_file $zone`
else
  keytag=`$mypath/keyconv.pl -a 5 -e $engine -l $label -p $key_path -i $tmp_file $zone`
fi

if [ ! $keytag ] ; then rm $tmp_file; exit 1 ; fi

echo "Set key id"
$mypath/set_key_id -l $label -n $keytag -p $pin

rm $tmp_file
