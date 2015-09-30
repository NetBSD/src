#!/usr/sbin/dtrace -s
/*
 * readbytes.d - read bytes by process name. DTrace OneLiner.
 *
 * This is a DTrace OneLiner from the DTraceToolkit.
 *
 * $Id: readbytes.d,v 1.1.1.1 2015/09/30 22:01:09 christos Exp $
 */

sysinfo:::readch { @bytes[execname] = sum(arg0); }
