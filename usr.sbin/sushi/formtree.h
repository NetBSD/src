/*      $NetBSD: formtree.h,v 1.2 2001/01/10 03:05:48 garbled Exp $       */

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *  
 * Copyright (c) 2000 Tim Rightnour <garbled@netbsd.org>  
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _FORMTREE_H_
#define _FORMTREE_H_

#define DATAT_ENTRY	1	/* static entry */
#define DATAT_LIST	2	/* static list */
#define DATAT_BLANK	3	/* blank entry */
#define DATAT_FUNC	4	/* function generated list */
#define DATAT_SCRIPT	5	/* script generated list */
#define DATAT_NOEDIT	6	/* uneditable field */
#define DATAT_INVIS	7	/* invisible field */
#define DATAT_INTEGER	8	/* an integer field */
#define DATAT_MLIST	9	/* multiple selection static list */
#define DATAT_MFUNC	10	/* multiple selection funcgen list */
#define DATAT_MSCRIPT	11	/* multiple selection scriptgen list */
#define DATAT_ESCRIPT	12	/* script generated entry */
#define DATAT_NESCRIPT	13	/* script generated uneditable field */

CIRCLEQ_HEAD(cqForm, formentry);

typedef struct formentry {
	struct cqForm cqSubFormHead;	/* CIRCLEQ_HEAD */
	CIRCLEQ_ENTRY(formentry) cqFormEntries;

	char	*desc;
	int	type;
	int	required;
	int	elen;
	char	*data;
	char	**list; /* optional list entry */
} FTREE_ENTRY;

extern struct cqForm *cqFormHeadp;

#define MAX_FIELD	512
#define QUIT	(REQ_MAX_COMMAND+1)
#define BAIL	(QUIT+1)
#define GENLIST	(BAIL+1)
#define SHOWHELP (GENLIST+1)
#define DUMPSCREEN (SHOWHELP+1)
#define REFRESH	(DUMPSCREEN+1)
#define SHELL (REFRESH+1)
#define RESET (SHELL+1)
#define EDIT (RESET+1)
#define COMMAND (EDIT+1)
#define FASTBAIL (COMMAND+1)
#define TREE_ISEMPTY(cqf)	(CIRCLEQ_FIRST(cqf) == (void *)cqf)

int handle_preform __P((char *, char *));
int handle_form __P((char *, char *, char **));
int form_entries __P((struct cqForm *));
FTREE_ENTRY *form_getentry __P((struct cqForm *, int));
void form_printtree __P((struct cqForm *));
int process_form __P((FORM *, char *));

#endif	/* _FORMTREE_H_ */
