#!/usr/sbin/dtrace -s
/*
 * filebyproc.d - snoop files opened by process name. DTrace OneLiner.
 *
 * This is a DTrace OneLiner from the DTraceToolkit.
 *
 * $Id: filebyproc.d,v 1.1.1.1 2015/09/30 22:01:07 christos Exp $
 */

syscall::open*:entry { printf("%s %s", execname, copyinstr(arg0)); }
