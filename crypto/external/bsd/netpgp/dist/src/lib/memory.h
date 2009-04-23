/*
 * Copyright (c) 2005-2008 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer. The Contributors have asserted
 * their moral rights under the UK Copyright Design and Patents Act 1988 to
 * be recorded as the authors of this copyright work.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** \file
 */
#ifndef OPS_MEMORY_H
#define OPS_MEMORY_H

#include <sys/types.h>

#include "packet.h"

/** __ops_memory_t
 */
typedef struct __ops_memory_t {
	unsigned char  *buf;
	size_t          length;
	size_t          allocated;
}               __ops_memory_t;


__ops_memory_t   *__ops_memory_new(void);
void            __ops_memory_free(__ops_memory_t *);
void            __ops_memory_init(__ops_memory_t *, size_t);
void            __ops_memory_pad(__ops_memory_t *, size_t);
void            __ops_memory_add(__ops_memory_t *, const unsigned char *, size_t);
void __ops_memory_place_int(__ops_memory_t *, unsigned, unsigned, size_t);
void            __ops_memory_make_packet(__ops_memory_t *, __ops_content_tag_t);
void            __ops_memory_clear(__ops_memory_t *);
void            __ops_memory_release(__ops_memory_t *);

void            __ops_writer_set_memory(__ops_create_info_t *, __ops_memory_t *);

size_t          __ops_memory_get_length(const __ops_memory_t *);
void           *__ops_memory_get_data(__ops_memory_t *);

void            __ops_random(void *, size_t);

#endif
