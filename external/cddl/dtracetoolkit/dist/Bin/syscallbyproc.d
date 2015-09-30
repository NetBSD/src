#!/usr/sbin/dtrace -s
/*
 * syscallbyproc.d - report on syscalls by process name . DTrace OneLiner.
 *
 * This is a DTrace OneLiner from the DTraceToolkit.
 *
 * $Id: syscallbyproc.d,v 1.1.1.1 2015/09/30 22:01:07 christos Exp $
 */

syscall:::entry { @num[execname] = count(); }
