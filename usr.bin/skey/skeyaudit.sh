#!/bin/sh
#
#	$NetBSD: skeyaudit.sh,v 1.3 2000/07/07 15:19:09 mjl Exp $
#
# This script will look thru the skeykeys file for
# people with sequence numbers less than LOWLIMIT=12
# and send them an e-mail reminder to use skeyinit soon
# 

AWK=/usr/bin/awk
GREP=/usr/bin/grep
ECHO=/bin/echo
KEYDB=/etc/skeykeys
LOWLIMIT=12
ADMIN=root
SUBJECT="Reminder: Run skeyinit"
HOST=`/bin/hostname`


if [ "$1" != "" ]
then
 LOWLIMIT=$1
fi


# an skeykeys entry looks like
#   jsw 0076 la13079          ba20a75528de9d3a
#   #oot md5 0005 aspa26398        9432d570ff4421f0  Jul 07,2000 01:36:43
#   mjl sha1 0099 alpha2           459a5dac23d20a90  Jul 07,2000 02:14:17
# the sequence number is the second (or third) entry
#

SKEYS=`$AWK '/^#/ {next} {if($2 ~ /^[0-9]+$/) print $1,$2,$3; else print $1,$3,$4; }' $KEYDB`

set -- ${SKEYS}

while [ "X$1" != "X" ]; do
  USER=$1
  SEQ=$2
  KEY=$3
  shift 3
echo "$USER -- $SEQ -- $KEY"
  if [ $SEQ -lt $LOWLIMIT ]; then
    if [ $SEQ -lt  3 ]; then
      SUBJECT="IMPORTANT action required"
    fi
    (
    $ECHO "You are nearing the end of your current S/Key sequence for account $i"
    $ECHO "on system $HOST."
    $ECHO ""
    $ECHO "Your S/key sequence number is now $SEQ.  When it reaches zero you"
    $ECHO "will no longer be able to use S/Key to login into the system.  "
    $ECHO " "
    $ECHO "Use \"skeyinit -s\" to reinitialize your sequence number."
    $ECHO ""
    ) | /usr/bin/Mail -s "$SUBJECT"  $USER $ADMIN
  fi
done
