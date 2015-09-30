#!/usr/sbin/dtrace -s
/*
 * putnexts.d - stream putnext() tracing with stacks. Solaris, DTrace.
 *
 * This shows who is calling putnext() to who, by listing the destination
 * queue and the calling stack, by frequency count. This is especially useful
 * for understanding streams based frameworks, such as areas of the Solaris
 * TCP/IP stack.
 *
 * $Id: putnexts.d,v 1.1.1.1 2015/09/30 22:01:09 christos Exp $
 *
 * USAGE:	putnext.d
 *
 * BASED ON: /usr/demo/dtrace/putnext.d
 *
 * PORTIONS: Copyright (c) 2007 Brendan Gregg.
 *
 * CDDL HEADER START
 *
 *  The contents of this file are subject to the terms of the
 *  Common Development and Distribution License, Version 1.0 only
 *  (the "License").  You may not use this file except in compliance
 *  with the License.
 *
 *  You can obtain a copy of the license at Docs/cddl1.txt
 *  or http://www.opensolaris.org/os/licensing.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 * CDDL HEADER END
 *
 * 12-Jun-2005  Brendan Gregg   Created this.
 */

fbt::putnext:entry
{
	@[stringof(args[0]->q_qinfo->qi_minfo->mi_idname), stack(5)] = count();
}
