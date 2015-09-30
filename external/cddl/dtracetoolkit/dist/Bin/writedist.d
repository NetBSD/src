#!/usr/sbin/dtrace -s
/*
 * writedist.d - write distribution by process name. DTrace OneLiner.
 *
 * This is a DTrace OneLiner from the DTraceToolkit.
 *
 * $Id: writedist.d,v 1.1.1.1 2015/09/30 22:01:07 christos Exp $
 */

sysinfo:::writech { @dist[execname] = quantize(arg0); }
