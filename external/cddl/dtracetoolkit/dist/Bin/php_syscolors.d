#!/usr/sbin/dtrace -Zs
/*
 * php_syscolors.d - trace PHP function flow plus syscalls, in color.
 *                   Written for the PHP DTrace provider.
 *
 * $Id: php_syscolors.d,v 1.1.1.1 2015/09/30 22:01:07 christos Exp $
 *
 * USAGE: php_syscolors.d	# hit Ctrl-C to end
 *
 * This watches PHP function entries and returns, and indents child
 * function calls.
 *
 * FIELDS:
 *		C		CPU-id
 *		PID		Process ID
 *		DELTA(us)	Elapsed time from previous line to this line
 *		FILE		Filename of the PHP program
 *		LINE		Line number of filename
 *		TYPE		Type of call (func/syscall)
 *		NAME		PHP function or syscall name
 *
 * Filename and function names are printed if available.
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
        color_php = "\033[2;35m";		/* violet, faint */
        color_syscall = "\033[2;32m";		/* green, faint */
        color_off = "\033[0m";			/* default */

	self->depth = 0;
	printf("%s %6s/%-4s %10s  %16s:%-4s %-8s -- %s\n", "C", "PID", "TID",
	    "DELTA(us)", "FILE", "LINE", "TYPE", "NAME");
}

php*:::function-entry,
php*:::function-return
/self->last == 0/
{
	self->last = timestamp;
}

php*:::function-entry
/arg0/
{
	this->delta = (timestamp - self->last) / 1000;
	printf("%s%d %6d/%-4d %10d  %16s:%-4d %-8s %*s-> %s%s\n", color_php,
	    cpu, pid, tid, this->delta, basename(copyinstr(arg1)), arg2, "func",
	    self->depth * 2, "", copyinstr(arg0), color_off);
	self->depth++;
	self->last = timestamp;
}

php*:::function-return
/arg0/
{
	this->delta = (timestamp - self->last) / 1000;
	this->name = strjoin(strjoin(copyinstr(arg1), "::"), copyinstr(arg0));
	self->depth -= self->depth > 0 ? 1 : 0;
	printf("%s%d %6d/%-4d %10d  %16s:%-4d %-8s %*s<- %s%s\n", color_php,
	    cpu, pid, tid, this->delta, basename(copyinstr(arg1)), arg2, "func",
	    self->depth * 2, "", copyinstr(arg0), color_off);
	self->last = timestamp;
}

syscall:::entry
/self->last/
{
	this->delta = (timestamp - self->last) / 1000;
	printf("%s%d %6d/%-4d %10d  %16s:-    %-8s %*s-> %s%s\n", color_syscall,
	    cpu, pid, tid, this->delta, "\"", "syscall", self->depth * 2, "",
	    probefunc, color_off);
	self->last = timestamp;
}

syscall:::return
/self->last/
{
	this->delta = (timestamp - self->last) / 1000;
	printf("%s%d %6d/%-4d %10d  %16s:-    %-8s %*s<- %s%s\n", color_syscall,
	    cpu, pid, tid, this->delta, "\"", "syscall", self->depth * 2, "",
	    probefunc, color_off);
	self->last = timestamp;
}

proc:::exit
/pid == $target/
{
	exit(0);
}
