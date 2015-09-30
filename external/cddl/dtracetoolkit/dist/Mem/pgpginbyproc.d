#!/usr/sbin/dtrace -s
/*
 * pgpginbyproc.d - pages paged in by process name. DTrace OneLiner.
 *
 * This is a DTrace OneLiner from the DTraceToolkit.
 *
 * $Id: pgpginbyproc.d,v 1.1.1.1 2015/09/30 22:01:09 christos Exp $
 */

vminfo:::pgpgin { @pg[execname] = sum(arg0); }
