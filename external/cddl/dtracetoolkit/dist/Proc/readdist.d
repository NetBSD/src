#!/usr/sbin/dtrace -s
/*
 * readdist.d - read distribution by process name. DTrace OneLiner.
 *
 * This is a DTrace OneLiner from the DTraceToolkit.
 *
 * $Id: readdist.d,v 1.1.1.1 2015/09/30 22:01:09 christos Exp $
 */

sysinfo:::readch { @dist[execname] = quantize(arg0); }
