/* $NetBSD: opt-P.0.pro,v 1.1 2021/03/06 17:56:33 rillig Exp $ */
/* $FreeBSD$ */

/*
 * It's syntactically possible to specify a profile file inside another
 * profile file.  Such a profile file is ignored since only a single
 * profile file is ever loaded.
 */

-P/nonexistent
