/*	$NetBSD: complete.h,v 1.5.8.1 2009/05/13 19:19:56 jym Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Anon Ymous.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef __COMPLETE_H__
#define __COMPLETE_H__

#include <histedit.h>

typedef struct {
	EditLine	*el;			/* editline(3) editline structure */
	History		*hist;			/* editline(3) history structure */
} el_mode_t;

struct el_modes_s {
	el_mode_t command;
	el_mode_t string;
	el_mode_t filec;
	el_mode_t mime_enc;
};

extern struct el_modes_s elm;

char *my_gets(el_mode_t *, const char *, char *);
void init_editline(void);

/*
 * User knobs: environment names used by this module.
 */
#define ENAME_EL_COMPLETION_KEYS	"el-completion-keys"
#define ENAME_EL_EDITOR			"el-editor"
#define ENAME_EL_HISTORY_SIZE		"el-history-size"

#endif /* __COMPLETE_H__ */
