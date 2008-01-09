/* 	$NetBSD: envstat.h,v 1.1.2.3 2008/01/09 02:01:59 matt Exp $	*/

/*-
 * Copyright (c) 2007 Juan Romero Pardines.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ENVSTAT_H_
#define _ENVSTAT_H_

#include <stdio.h>
#include <sys/queue.h>
#include <prop/proplib.h>

struct sensor_block {
	SLIST_ENTRY(sensor_block) sb_head;
	prop_dictionary_t dict;	/* dictionary associated */
};

struct device_block {
	SLIST_ENTRY(device_block) db_head;
	prop_array_t array;	/* array associated */
	const char *dev_key;	/* keyword to find device array */
};

void config_dict_add_prop(const char *, char *);
void config_dict_adddev_prop(const char *, const char *, int);
void config_dict_destroy(prop_dictionary_t);
void config_dict_dump(prop_dictionary_t);
void config_dict_fulldump(void);
void config_dict_mark(void);
prop_dictionary_t config_dict_parsed(void);

prop_number_t convert_val_to_pnumber(prop_dictionary_t, const char *,
				     const char *, char *);

void config_devblock_add(const char *, prop_dictionary_t);
void config_devblock_check_sensorprops(prop_dictionary_t,
				       prop_dictionary_t,
				       const char *);
prop_dictionary_t config_devblock_getdict(const char *, const char *);

/* 
 * The yacc function that parses the configuration file and builds
 * the dictionary for the ENVSYS_SETDICTIONARY ioctl.
 */
void config_parse(FILE *, prop_dictionary_t);

#endif /* _ENVSTAT_H_ */
