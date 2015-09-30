#!/usr/sbin/dtrace -Zs
/*
 * py_flow.d - snoop Python execution showing function flow.
 *             Written for the Python DTrace provider.
 *
 * $Id: py_flow.d,v 1.1.1.1 2015/09/30 22:01:06 christos Exp $
 *
 * This traces Python activity from all Python programs on the system
 * running with Python provider support.
 *
 * USAGE: py_flow.d			# hit Ctrl-C to end
 *
 * This watches Python function entries and returns, and indents child
 * function calls.
 *
 * FIELDS:
 *		C		CPU-id
 *		TIME(us)	Time since boot, us
 *		FILE		Filename that this function belongs to
 *		FUNC		Function name
 *
 * LEGEND:
 *		->		function entry
 *		<-		function return
 *
 * WARNING: Watch the first column carefully, it prints the CPU-id. If it
 * changes, then it is very likely that the output has been shuffled.
 *
 * COPYRIGHT: Copyright (c) 2007 Brendan Gregg.
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
 * 09-Sep-2007	Brendan Gregg	Created this.
 */

#pragma D option quiet
#pragma D option switchrate=10

self int depth;

dtrace:::BEGIN
{
	printf("%3s %-16s %-16s -- %s\n", "C", "TIME(us)", "FILE", "FUNC");
}

python*:::function-entry
{
	printf("%3d %-16d %-16s %*s-> %s\n", cpu, timestamp / 1000, 
	    basename(copyinstr(arg0)), self->depth * 2, "", copyinstr(arg1));
	self->depth++;
}

python*:::function-return
{
	self->depth -= self->depth > 0 ? 1 : 0;
	printf("%3d %-16d %-16s %*s<- %s\n", cpu, timestamp / 1000,
	    basename(copyinstr(arg0)), self->depth * 2, "", copyinstr(arg1));
}
