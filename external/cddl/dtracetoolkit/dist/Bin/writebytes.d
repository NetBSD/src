#!/usr/sbin/dtrace -s
/*
 * writebytes.d - write bytes by process name. DTrace OneLiner.
 *
 * This is a DTrace OneLiner from the DTraceToolkit.
 *
 * $Id: writebytes.d,v 1.1.1.1 2015/09/30 22:01:06 christos Exp $
 */

sysinfo:::writech { @bytes[execname] = sum(arg0); }
