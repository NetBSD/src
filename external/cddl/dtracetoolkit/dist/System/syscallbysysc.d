#!/usr/sbin/dtrace -s
/*
 * syscallbysysc.d - report on syscalls by syscall. DTrace OneLiner.
 *
 * This is a DTrace OneLiner from the DTraceToolkit.
 *
 * $Id: syscallbysysc.d,v 1.1.1.1 2015/09/30 22:01:09 christos Exp $
 */

syscall:::entry { @num[probefunc] = count(); }
