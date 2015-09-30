#!/usr/sbin/dtrace -s
/*
 * lockbyproc.d - lock time by process name. DTrace OneLiner.
 *
 * This is a DTrace OneLiner from the DTraceToolkit.
 *
 * $Id: lockbyproc.d,v 1.1.1.1 2015/09/30 22:01:06 christos Exp $
 */

lockstat:::adaptive-block { @time[execname] = sum(arg1); }
