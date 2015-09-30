#!/usr/sbin/dtrace -s
/*
 * minfbyproc.d - minor faults by process name. DTrace OneLiner.
 *
 * This is a DTrace OneLiner from the DTraceToolkit.
 *
 * $Id: minfbyproc.d,v 1.1.1.1 2015/09/30 22:01:09 christos Exp $
 */

vminfo:::as_fault { @mem[execname] = sum(arg0); }
