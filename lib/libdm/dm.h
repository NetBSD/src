/*
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Hamsik.
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


#ifndef _LIB_DM_H_
#define _LIB_DM_H_

__BEGIN_DECLS

#define DM_DEVICE_PATH "/dev/mapper/control"

#define IOCTL_TYPE_IN  0x1
#define IOCTL_TYPE_OUT 0x2

struct libdm_task;
struct libdm_cmd;
struct libdm_target;
struct libdm_table;
struct libdm_dev;
struct libdm_iter;

typedef struct libdm_task* libdm_task_t;
typedef struct libdm_cmd* libdm_cmd_t;
typedef struct libdm_target* libdm_target_t;
typedef struct libdm_table* libdm_table_t;
typedef struct libdm_dev* libdm_dev_t;
typedef struct libdm_iter* libdm_iter_t;

struct cmd_version {
	const char *cmd;
	uint32_t version[3];
};

void libdm_iter_destroy(libdm_iter_t);

libdm_task_t libdm_task_create(const char *);
void libdm_task_destroy(libdm_task_t);

char *libdm_task_get_command(libdm_task_t);
int32_t libdm_task_get_cmd_version(libdm_task_t, uint32_t *, size_t);

int libdm_task_set_name(const char *, libdm_task_t);
char *libdm_task_get_name(libdm_task_t);

int libdm_task_set_uuid(const char *, libdm_task_t);
char *libdm_task_get_uuid(libdm_task_t);

int libdm_task_set_minor(uint32_t, libdm_task_t);
uint32_t libdm_task_get_minor(libdm_task_t);

uint32_t libdm_task_get_flags(libdm_task_t);
void libdm_task_set_flags(libdm_task_t, uint32_t);

void libdm_task_set_suspend_flag(libdm_task_t);
void libdm_task_del_suspend_flag(libdm_task_t);

void libdm_task_set_status_flag(libdm_task_t);
void libdm_task_del_status_flag(libdm_task_t);

void libdm_task_set_exists_flag(libdm_task_t);
void libdm_task_del_exists_flag(libdm_task_t);

void libdm_task_set_nocount_flag(libdm_task_t);
void libdm_task_del_nocount_flag(libdm_task_t);

uint32_t libdm_task_get_target_num(libdm_task_t);
int32_t libdm_task_get_open_num(libdm_task_t);
uint32_t libdm_task_get_event_num(libdm_task_t);

int libdm_task_set_cmd(libdm_cmd_t, libdm_task_t);
libdm_cmd_t libdm_task_get_cmd(libdm_task_t);

/* cmd_data dictionary entry manipulating functions */
libdm_cmd_t libdm_cmd_create(void);
void libdm_cmd_destroy(libdm_cmd_t);
libdm_iter_t libdm_cmd_iter_create(libdm_cmd_t);

int libdm_cmd_set_table(libdm_table_t, libdm_cmd_t);

/* cmd is array of table dictionaries */
libdm_table_t libdm_cmd_get_table(libdm_iter_t);

/* cmd is array of dev dictionaries */
libdm_dev_t libdm_cmd_get_dev(libdm_iter_t);

/* cmd is array of target dictonaries */
libdm_target_t libdm_cmd_get_target(libdm_iter_t);
/* cmd is array of dev_t's */
uint64_t libdm_cmd_get_deps(libdm_iter_t);

/* Table functions. */
libdm_table_t libdm_table_create(void);
void libdm_table_destroy(libdm_table_t);

int libdm_table_set_start(uint64_t, libdm_table_t);
uint64_t libdm_table_get_start(libdm_table_t);

int libdm_table_set_length(uint64_t, libdm_table_t);
uint64_t libdm_table_get_length(libdm_table_t);

int libdm_table_set_target(const char *, libdm_table_t);
char * libdm_table_get_target(libdm_table_t);

int libdm_table_set_params(const char *, libdm_table_t);
char * libdm_table_get_params(libdm_table_t);

int32_t libdm_table_get_status(libdm_table_t);

/* Target manipulating functions. */
void libdm_target_destroy(libdm_target_t);

char * libdm_target_get_name(libdm_target_t);
int32_t libdm_target_get_version(libdm_target_t, uint32_t *, size_t);

/* Dev manipulating functions. */
void libdm_dev_destroy(libdm_dev_t);

char * libdm_dev_get_name(libdm_dev_t);
uint32_t libdm_dev_get_minor(libdm_dev_t);

int libdm_dev_set_newname(const char *, libdm_cmd_t);

/* task run routine */
int libdm_task_run(libdm_task_t);

__END_DECLS

#endif /* _LIB_DM_H_ */
