#!/usr/sbin/dtrace -s
/*
 * creatbyproc.d - file creat()s by process name. DTrace OneLiner.
 *
 * This is a DTrace OneLiner from the DTraceToolkit.
 *
 * $Id: creatbyproc.d,v 1.1.1.1 2015/09/30 22:01:09 christos Exp $
 */

syscall::creat*:entry { printf("%s %s", execname, copyinstr(arg0)); }
