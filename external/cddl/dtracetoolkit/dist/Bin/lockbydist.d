#!/usr/sbin/dtrace -s
/*
 * lockbydist.d - lock distrib. by process name. DTrace OneLiner.
 *
 * This is a DTrace OneLiner from the DTraceToolkit.
 *
 * $Id: lockbydist.d,v 1.1.1.1 2015/09/30 22:01:07 christos Exp $
 */

lockstat:::adaptive-block { @time[execname] = quantize(arg1); }
