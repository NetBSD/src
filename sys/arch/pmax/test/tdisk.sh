#! /bin/sh -
cp /vmunix /tmp/vmunix
while cmp /vmunix /tmp/vmunix
do
	echo
done
