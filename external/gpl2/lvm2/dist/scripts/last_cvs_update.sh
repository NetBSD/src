#!/bin/sh
#$Header: /cvsroot/src/external/gpl2/lvm2/dist/scripts/last_cvs_update.sh,v 1.1.1.1 2008/12/22 00:18:57 haad Exp $
################################################################################
##
##    Copyright 2001 Sistina Software, Inc.
##
##    This is free software released under the GNU General Public License.
##    There is no warranty for this software.  See the file COPYING for
##    details.
##
##    See the file CONTRIBUTORS for a list of contributors.
##
##    This file is maintained by:
##      AJ Lewis <lewis@sistina.com>
## 
##    File name: last_cvs_update.sh
##
##    Description: displays the last file updated by CVS commands and the date
##                 it was updated.  May be given a relative or absolute path.
##                 Based on this information, you should be able to do a 
##                 cvs co -D $date GFS and get the same version of the source
##                 tree as this tool was run on.
##                 
##                 Will also give you the CVS tag the source tree is based off
##                 off when appropriate
## 
##                 Output format:
##                 [Tag:  $TAG]
##                 The last file updated by CVS was:
##                 $path/$file
##                 on
##                 $date
##
################################################################################

if [[ -z $1 ]];
 then path=.;
else
 if [[ $1 == "-h" ]];
   then echo "usage: $0 [ -h | path ]"
        exit 0;
 else
   if [[ -d $1 ]]
     then path=$1;
   else
     echo "usage: $0 [ -h | path ]"
     exit 0;
   fi
 fi
fi

# grab the tag from the path passed in
if [[ -f $path/CVS/Tag ]];
  then echo "Tag: " `cat $path/CVS/Tag | sed -e 's/^[NT]//'`
fi

awk '
BEGIN {                            
  FS = "/"                         
}
{
    # find the path for the Entries file
    path = FILENAME
    sub(/^\.\//, "", path)
    
    # remove the CVS part of it
    sub(/CVS\/Entries/, "", path)
    
    # add the path the the filename that was modified, and put the date it was
    # modified in as well
    print path $2 " " $4

}' `find $path -name "Entries" -printf "%h/%f "` | awk '
# converts string name of month the a numeral
function cvt_month(month) {
  if(month == "Jan")
    return 0
  if(month == "Feb")
    return 1
  if(month == "Mar")
    return 2
  if(month == "Apr")
    return 3
  if(month == "May")
    return 4
  if(month == "Jun")
    return 5
  if(month == "Jul")
    return 6
  if(month == "Aug")
    return 7
  if(month == "Sep")
    return 8
  if(month == "Oct")
    return 9
  if(month == "Nov")
    return 10
  if(month == "Dec")
    return 11
  return -1
}
BEGIN {                            
  FS = " "                         
  latest=""
  maxyear = 0
  maxdate = 0
  maxmonth = "Jan"
  maxtime = "00:00:00"
}
{
   # check year first
   if (maxyear < $6) {
      date = $2 " " $3 " " $4 " " $5 " " $6
      file = $1
      maxyear = $6
      maxmonth = $3
      maxdate = $4
      maxtime = $5
   }
   else {
      if (maxyear == $6) {
        # then month if year is the same
        if (cvt_month(maxmonth) < cvt_month($3)) {
          date = $2 " " $3 " " $4 " " $5 " " $6
          file = $1
	  maxmonth = $3
	  maxdate = $4
	  maxtime = $5
        }
        else {
          if (cvt_month(maxmonth) == cvt_month($3)) {
	    #then date if month is the same
            if (maxdate < $4) {
              date = $2 " " $3 " " $4 " " $5 " " $6
              file = $1
	      maxdate = $4
	      maxtime = $5
	    }
	    else {
	      if (maxdate == $4) {
	        # then time if date is the same
	        if (maxtime < $5) {
		  date = $2 " " $3 " " $4 " " $5 " " $6
                  file = $1
		  maxtime = $5
		}
              }
	    }
	  }
        }
      }
   }
}

END {
   # strip leading "./" from filename
   sub(/^\.\//, "", file)
   print "The last file updated by CVS was:"
   print file 
   print "on"
   print date " GMT"
}'

