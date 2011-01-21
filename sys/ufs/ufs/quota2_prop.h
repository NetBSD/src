/* $NetBSD: quota2_prop.h,v 1.1.2.2 2011/01/21 16:58:06 bouyer Exp $ */
/*-
  * Copyright (c) 2010 Manuel Bouyer
  * All rights reserved.
  * This software is distributed under the following condiions
  * compliant with the NetBSD foundation policy.
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

#include <prop/proplib.h>
#include <ufs/ufs/quota2.h>

prop_dictionary_t prop_dictionary_get_dict(prop_dictionary_t, const char *);
int quota2_dict_get_q2v_limits(prop_dictionary_t, struct quota2_val *, bool);
int quota2_dict_get_q2v_usage(prop_dictionary_t, struct quota2_val *);
int quota2_dict_get_q2e_usage(prop_dictionary_t, struct quota2_entry *);
int quota2_get_cmds(prop_dictionary_t, prop_array_t *);

bool prop_array_add_and_rel(prop_array_t, prop_object_t);
bool prop_dictionary_set_and_rel(prop_dictionary_t, const char *,
     prop_object_t);
prop_dictionary_t quota2_prop_create(void);
bool quota2_prop_add_command(prop_array_t, const char *, const char *,
    prop_array_t);
prop_dictionary_t q2vtoprop(struct quota2_val *);
prop_dictionary_t q2etoprop(struct quota2_entry *, int);
