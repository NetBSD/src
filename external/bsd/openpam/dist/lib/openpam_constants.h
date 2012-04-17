/*	$NetBSD: openpam_constants.h,v 1.2.4.2 2012/04/17 00:03:58 yamt Exp $	*/

/*-
 * Copyright (c) 2011 Dag-Erling Smørgrav
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Id: openpam_constants.h 491 2011-11-12 00:12:32Z des
 */

#ifndef OPENPAM_CONSTANTS_INCLUDED
#define OPENPAM_CONSTANTS_INCLUDED

extern const char *pam_err_name[PAM_NUM_ERRORS];
extern const char *pam_item_name[PAM_NUM_ITEMS];
extern const char *pam_facility_name[PAM_NUM_FACILITIES];
extern const char *pam_control_flag_name[PAM_NUM_CONTROL_FLAGS];
extern const char *pam_func_name[PAM_NUM_PRIMITIVES];
extern const char *pam_sm_func_name[PAM_NUM_PRIMITIVES];

#endif
