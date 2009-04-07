#! /bin/sh

# Test that a getdate executable meets its specification.
#
# Copyright (C) 2004 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */



###
### Globals
###
LOGFILE=`pwd`/getdate.log
if test -f "$LOGFILE"; then
	mv $LOGFILE $LOGFILE~
fi



###
### Functions
###
verify ()
{
	echo >>getdate-got
	if cmp getdate-expected getdate-got >getdate.cmp; then
		echo "PASS: $1" >>$LOGFILE
	else
		cat getdate.cmp >>$LOGFILE
		echo "** expected: " >>$LOGFILE
		cat getdate-expected >>$LOGFILE
		echo "** got: " >>$LOGFILE
		cat getdate-got >>$LOGFILE
		echo "FAIL: $1" | tee -a $LOGFILE >&2
		echo "Failed!  See $LOGFILE for more!" >&2
		exit 1
	fi
}



skip ()
{
	echo "SKIP: $1"${2+" ($2)"} >>$LOGFILE
}



# Prep for future calls to valid_timezone().
#
# This should set $UTZ to three spaces, `GMT', `Unrecognized/Unrecognized', or
# possibly the empty string, depending on what system we are running on.  With
# any luck, this will catch any other existing variations as well.  The way it
# is used later does have the disadvantage of rejecting at least the
# `Europe/London' timezone for half the year when $UTZ gets set to `GMT', like
# happens on NetBSD, but, since I haven't come up with any better ideas and
# since rejecting a timezone just causes a few tests to be skipped, this will
# have to do for now.
#
# UTZ stands for Unrecognized Time Zone.
UTZ=`TZ=Unrecognized/Unrecognized date +%Z`

# The following function will return true if $1 is a valid timezone.  It will
# return false and set $skipreason, otherwise.
#
# Clobbers $NTZ & $skipreason.
#
# SUS2 says `date +%Z' will return `no characters' if `no timezone is
# determinable'.  It is, unfortunately, not very specific about what
# `determinable' means.  On GNU/Linux, `date +%Z' returns $TZ when $TZ is not
# recognized.  NetBSD 1.6.1 "determines" that an unrecognizable value in $TZ
# really means `GMT'.  On Cray, the standard is ignored and `date +%Z' returns
# three spaces when $TZ is not recognized.  We test for all three cases, plus
# the empty string for good measure, though I know of no set of conditions
# which will actually cause `date +%Z' to return the empty string SUS2
# specifies.
#
# Due to the current nature of this test, this will not work for the
# three-letter zone codes on some systems.  e.g.:
#
#	test `TZ=EST date +%Z` = "EST"
#
# should, quite correctly, evaluate to true on most systems, but:
#
#	TZ=Asia/Calcutta date +%Z
#
# would return `IST' on GNU/Linux, and hopefully any system which understands
# the `Asia/Calcutta' timezone, and `   ' on Cray.  Similarly:
#
#	TZ=Doesnt_Exist/Doesnt_Exist date +%Z
#
# returns `Doesnt_Exist/Doesnt_Exist' on GNU/Linux and `   ' on Cray.
#
# Unfortunately, the %z date format string (-HHMM format time zone) supported
# by the GNU `date' command is not part of any standard I know of and,
# therefore, is probably not portable.
#
valid_timezone ()
{
	NTZ=`TZ=$1 date +%Z`
	if test "$NTZ" = "$UTZ" || test "$NTZ" = "$1"; then
		skipreason="$1 is not a recognized timezone on this system"
		return `false`
	else
		return `:`
	fi
}



###
### Tests
###

# Why are these dates tested?
#
# February 29, 2003
#   Is not a leap year - should be invalid.
#
# 2004-12-40
#   Make sure get_date does not "roll" date forward to January 9th.  Some
#   versions have been known to do this.
#
# Dec-5-1972
#   This is my birthday.  :)
#
# 3/29/1974
# 1996/05/12 13:57:45
#   Because.
#
# 12-05-12
#   This will be my 40th birthday.  Ouch.  :)
#
# 05/12/96
#   Because.
#
# third tuesday in March, 2078
#   Wanted this to work.
#
# 1969-12-32 2:00:00 UTC
# 1970-01-01 2:00:00 UTC
# 1969-12-32 2:00:00 +0400
# 1970-01-01 2:00:00 +0400
# 1969-12-32 2:00:00 -0400
# 1970-01-01 2:00:00 -0400
#   Playing near the UNIX Epoch boundry condition to make sure date rolling
#   is also disabled there.
#
# 1996-12-12 1 month
#   Test a relative date.



# The following tests are currently being skipped for being unportable:
#
# Tue Jan 19 03:14:07 2038 +0000
#   For machines with 31-bit time_t, any date past this date will be an
#   invalid date. So, any test date with a value greater than this
#   time is not portable.
#
# Feb. 29, 2096 4 years
#   4 years from this date is _not_ a leap year, so Feb. 29th does not exist.
#
# Feb. 29, 2096 8 years
#   8 years from this date is a leap year, so Feb. 29th does exist,
#   but on many hosts with 32-bit time_t types time, this test will
#   fail. So, this is not a portable test.
#

TZ=UTC0; export TZ

cat >getdate-expected <<EOF
Enter date, or blank line to exit.
	> Bad format - couldn't convert.
	> Bad format - couldn't convert.
	> 1972-12-05 00:00:00.000000000
	> 1974-03-29 00:00:00.000000000
	> 1996-05-12 13:57:45.000000000
	> 2012-05-12 00:00:00.000000000
	> 1996-05-12 00:00:00.000000000
	> Bad format - couldn't convert.
	> Bad format - couldn't convert.
	> 1970-01-01 02:00:00.000000000
	> Bad format - couldn't convert.
	> 1969-12-31 22:00:00.000000000
	> Bad format - couldn't convert.
	> 1970-01-01 06:00:00.000000000
	> 1997-01-12 00:00:00.000000000
	> 
EOF

./getdate >getdate-got <<EOF
February 29, 2003
2004-12-40
Dec-5-1972
3/29/1974
1996/05/12 13:57:45
12-05-12
05/12/96
third tuesday in March, 2078
1969-12-32 2:00:00 UTC
1970-01-01 2:00:00 UTC
1969-12-32 2:00:00 +0400
1970-01-01 2:00:00 +0400
1969-12-32 2:00:00 -0400
1970-01-01 2:00:00 -0400
1996-12-12 1 month
EOF

verify getdate-1



# Why are these dates tested?
#
# Ian Abbot reported these odd boundry cases.  After daylight savings time went
# into effect, non-daylight time zones would cause
# "Bad format - couldn't convert." errors, even when the non-daylight zone
# happened to be a universal one, like GMT.

TZ=Europe/London; export TZ
if valid_timezone $TZ; then
	cat >getdate-expected <<EOF
Enter date, or blank line to exit.
	> 2005-03-01 00:00:00.000000000
	> 2005-03-27 00:00:00.000000000
	> 2005-03-28 01:00:00.000000000
	> 2005-03-28 01:00:00.000000000
	> 2005-03-29 01:00:00.000000000
	> 2005-03-29 01:00:00.000000000
	> 2005-03-30 01:00:00.000000000
	> 2005-03-30 01:00:00.000000000
	> 2005-03-31 01:00:00.000000000
	> 2005-03-31 01:00:00.000000000
	> 2005-04-01 01:00:00.000000000
	> 2005-04-01 01:00:00.000000000
	> 2005-04-10 01:00:00.000000000
	> 2005-04-10 01:00:00.000000000
	> 2005-04-01 00:00:00.000000000
	> 
EOF

	./getdate >getdate-got <<EOF
2005-3-1 GMT
2005-3-27 GMT
2005-3-28 GMT
2005-3-28 UTC0
2005-3-29 GMT
2005-3-29 UTC0
2005-3-30 GMT
2005-3-30 UTC0
2005-3-31 GMT
2005-3-31 UTC0
2005-4-1 GMT
2005-4-1 UTC0
2005-4-10 GMT
2005-4-10 UTC0
2005-4-1 BST
EOF

	verify getdate-2
else
	skip getdate-2 "$skipreason"
fi



# Many of the following cases were also submitted by Ian Abbott, but the same
# errors are not exhibited.  The original problem had a similar root, but
# managed to produce errors with GMT, which is considered a "Universal Zone".
# This was fixed.
#
# The deeper problem has to do with "local zone" processing in getdate.y
# that causes local daylight zones to be excluded when local standard time is
# in effect and vice versa.  This used to cause trouble with GMT in Britian
# when British Summer Time was in effect, but this was overridden for the
# "Universal Timezones" (GMT, UTC, & UT), that might double as a local zone in
# some locales.  We still see in these tests the local daylight/standard zone
# exclusion in EST/EDT.  According to Paul Eggert in a message to
# bug-gnulib@gnu.org on 2005-04-12, this is considered a bug but may not be
# fixed soon due to its complexity.

TZ=America/New_York; export TZ
if valid_timezone $TZ; then
	cat >getdate-expected <<EOF
Enter date, or blank line to exit.
	> 2005-03-01 00:00:00.000000000
	> 2005-02-28 18:00:00.000000000
	> 2005-04-01 00:00:00.000000000
	> Bad format - couldn't convert.
	> 2005-04-30 19:00:00.000000000
	> 2005-04-30 20:00:00.000000000
	> 2005-05-01 00:00:00.000000000
	> 2005-04-30 20:00:00.000000000
	> Bad format - couldn't convert.
	> 2005-05-31 19:00:00.000000000
	> 2005-05-31 20:00:00.000000000
	> 2005-06-01 00:00:00.000000000
	> 2005-05-31 20:00:00.000000000
	> 
EOF

	./getdate >getdate-got <<EOF
2005-3-1 EST
2005-3-1 BST
2005-4-1 EST
2005-5-1 EST
2005-5-1 BST
2005-5-1 GMT
2005-5-1 EDT
2005-5-1 UTC0
2005-6-1 EST
2005-6-1 BST
2005-6-1 GMT
2005-6-1 EDT
2005-6-1 UTC0
EOF

	verify getdate-3
else
	skip getdate-3 "$skipreason"
fi



rm getdate-expected getdate-got getdate.cmp
exit 0
