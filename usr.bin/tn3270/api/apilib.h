/*	$NetBSD: apilib.h,v 1.4 1998/03/04 13:16:04 christos Exp $	*/

/*-
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)apilib.h	4.2 (Berkeley) 4/26/91
 */

/*
 * What one needs to specify
 */

extern int
    api_sup_errno,			/* Supervisor error number */
    api_sup_fcn_id,			/* Supervisor function id (0x12) */
    api_fcn_errno,			/* Function error number */
    api_fcn_fcn_id;			/* Function ID (0x6b, etc.) */

int api_name_resolve __P((char *));
int api_ps_or_oia_modified __P((void));
int api_query_session_id __P((QuerySessionIdParms *));
int api_query_session_parameters __P((QuerySessionParametersParms *));
int api_query_session_cursor __P((QuerySessionCursorParms *));
int api_connect_to_keyboard __P((ConnectToKeyboardParms *));
int api_disconnect_from_keyboard __P((DisconnectFromKeyboardParms *));
int api_write_keystroke __P((WriteKeystrokeParms *));
int api_disable_input __P((DisableInputParms *));
int api_enable_input __P((EnableInputParms *));
int api_copy_string __P((CopyStringParms *));
int api_read_oia_group __P((ReadOiaGroupParms *));
int api_finish __P((void));
int api_init __P((void));
