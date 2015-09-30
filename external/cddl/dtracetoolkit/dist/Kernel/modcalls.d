#!/usr/sbin/dtrace -s
/*
 * modcalls.d - kernel function calls by module. DTrace OneLiner.
 *
 * This is a DTrace OneLiner from the DTraceToolkit.
 *
 * $Id: modcalls.d,v 1.1.1.1 2015/09/30 22:01:09 christos Exp $
 */

fbt:::entry { @calls[probemod] = count(); }
