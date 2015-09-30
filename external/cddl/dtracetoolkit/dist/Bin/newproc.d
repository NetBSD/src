#!/usr/sbin/dtrace -s
/*
 * newproc.d - snoop new processes as they are executed. DTrace OneLiner.
 *
 * This is a DTrace OneLiner from the DTraceToolkit.
 *
 * $Id: newproc.d,v 1.1.1.1 2015/09/30 22:01:07 christos Exp $
 */

proc:::exec-success { trace(curpsinfo->pr_psargs); }
