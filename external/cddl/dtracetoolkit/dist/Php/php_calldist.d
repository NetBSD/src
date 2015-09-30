#!/usr/sbin/dtrace -Zs
/*
 * php_calldist.d - measure PHP elapsed times for functions.
 *                  Written for the PHP DTrace provider.
 *
 * $Id: php_calldist.d,v 1.1.1.1 2015/09/30 22:01:09 christos Exp $
 *
 * This traces PHP activity from all programs running on the system with
 * PHP provider support.
 *
 * USAGE: php_calldist.d 	# hit Ctrl-C to end
 *
 * This script prints distribution plots of elapsed time for PHP
 * operations. Use php_calltime.d for summary reports.
 *
 * FIELDS:
 *		1		Filename of the PHP program
 *		2		Type of call (func)
 *		3		Name of call
 *
 * Filename and function names are printed if available.
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

dtrace:::BEGIN
{
	printf("Tracing... Hit Ctrl-C to end.\n");
}

php*:::function-entry
/arg0/
{
	self->depth++;
	self->exclude[self->depth] = 0;
	self->function[self->depth] = timestamp;
}

php*:::function-return
/arg0 && self->function[self->depth]/
{
	this->elapsed_incl = timestamp - self->function[self->depth];
	this->elapsed_excl = this->elapsed_incl - self->exclude[self->depth];
	self->function[self->depth] = 0;
	self->exclude[self->depth] = 0;
	this->file = basename(copyinstr(arg1));
	this->name = copyinstr(arg0);

	@types_incl[this->file, "func", this->name] =
	    quantize(this->elapsed_incl / 1000);
	@types_excl[this->file, "func", this->name] =
	    quantize(this->elapsed_excl / 1000);

	self->depth--;
	self->exclude[self->depth] += this->elapsed_incl;
}

dtrace:::END
{
	printf("\nExclusive function elapsed times (us),\n");
	printa("   %s, %s, %s %@d\n", @types_excl);

	printf("\nInclusive function elapsed times (us),\n");
	printa("   %s, %s, %s %@d\n", @types_incl);
}
