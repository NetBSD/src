#!/usr/sbin/dtrace -s
/*
 * pidpersec.d - print new PIDs per sec.
 *               Written using DTrace (Solaris 10 3/05)
 *
 * This script prints the number of new processes created per second.
 *
 * $Id: pidpersec.d,v 1.1.1.1 2015/09/30 22:01:07 christos Exp $
 *
 * USAGE: pidpersec.d
 *
 * FIELDS:
 *
 *          TIME        Time, as a string
 *          LASTPID     Last PID created
 *          PID/s       Number of processes created per second
 *
 * SEE ALSO: execsnoop
 *
 * COPYRIGHT: Copyright (c) 2005 Brendan Gregg.
 *
 * CDDL HEADER START
 *
 *  The contents of this file are subject to the terms of the
 *  Common Development and Distribution License, Version 1.0 only
 *  (the "License").  You may not use this file except in compliance
 *  with the License.
 *
 *  You can obtain a copy of the license at Docs/cddl1.txt
 *  or http://www.opensolaris.org/os/licensing.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 * CDDL HEADER END
 *
 * 09-Jun-2005  Brendan Gregg   Created this.
 * 09-Jun-2005	   "      "	Last update.
 */

#pragma D option quiet

dtrace:::BEGIN
{
	printf("%-22s %6s\n", "TIME", "PID/s");
	pids = 0;
}

proc:::exec-success
{
	pids++;
}

profile:::tick-1sec
{
	printf("%-22Y %6d\n", walltimestamp, pids);
	pids = 0;
}
