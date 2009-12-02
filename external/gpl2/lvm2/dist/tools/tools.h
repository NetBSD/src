/*	$NetBSD: tools.h,v 1.1.1.2 2009/12/02 00:25:56 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _LVM_TOOLS_H
#define _LVM_TOOLS_H

#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include <configure.h>
#include <assert.h>
#include <libdevmapper.h>

#include "lvm-types.h"
#include "lvm-logging.h"
#include "activate.h"
#include "archiver.h"
#include "lvmcache.h"
#include "config.h"
#include "defaults.h"
#include "dev-cache.h"
#include "device.h"
#include "display.h"
#include "errors.h"
#include "filter.h"
#include "filter-composite.h"
#include "filter-persistent.h"
#include "filter-regex.h"
#include "metadata-exported.h"
#include "locking.h"
#include "lvm-exec.h"
#include "lvm-file.h"
#include "lvm-string.h"
#include "segtype.h"
#include "str_list.h"
#include "toolcontext.h"
#include "toollib.h"

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <sys/types.h>

#define CMD_LEN 256
#define MAX_ARGS 64

/* command functions */
typedef int (*command_fn) (struct cmd_context * cmd, int argc, char **argv);

#define xx(a, b...) int a(struct cmd_context *cmd, int argc, char **argv);
#include "commands.h"
#undef xx

/* define the enums for the command line switches */
enum {
#define arg(a, b, c, d, e) a ,
#include "args.h"
#undef arg
};

typedef enum {
	SIGN_NONE = 0,
	SIGN_PLUS = 1,
	SIGN_MINUS = 2
} sign_t;

typedef enum {
	PERCENT_NONE = 0,
	PERCENT_VG,
	PERCENT_FREE,
	PERCENT_LV,
	PERCENT_PVS
} percent_t;

enum {
	CHANGE_AY = 0,
	CHANGE_AN = 1,
	CHANGE_AE = 2,
	CHANGE_ALY = 3,
	CHANGE_ALN = 4
};

#define ARG_REPEATABLE 0x00000001

/* a global table of possible arguments */
struct arg {
	const char short_arg;
	char _padding[7];
	const char *long_arg;

	int (*fn) (struct cmd_context * cmd, struct arg * a);
	uint32_t flags;

	unsigned count;
	char *value;
	int32_t i_value;
	uint32_t ui_value;
	int64_t i64_value;
	uint64_t ui64_value;
	sign_t sign;
	percent_t percent;
	void *ptr;
};

#define CACHE_VGMETADATA 0x00000001

/* a register of the lvm commands */
struct command {
	const char *name;
	const char *desc;
	const char *usage;
	command_fn fn;

	unsigned flags;

	int num_args;
	int *valid_args;
};

void usage(const char *name);

/* the argument verify/normalise functions */
int yes_no_arg(struct cmd_context *cmd, struct arg *a);
int yes_no_excl_arg(struct cmd_context *cmd, struct arg *a);
int size_kb_arg(struct cmd_context *cmd, struct arg *a);
int size_mb_arg(struct cmd_context *cmd, struct arg *a);
int int_arg(struct cmd_context *cmd, struct arg *a);
int int_arg_with_sign(struct cmd_context *cmd, struct arg *a);
int int_arg_with_sign_and_percent(struct cmd_context *cmd, struct arg *a);
int major_arg(struct cmd_context *cmd, struct arg *a);
int minor_arg(struct cmd_context *cmd, struct arg *a);
int string_arg(struct cmd_context *cmd, struct arg *a);
int tag_arg(struct cmd_context *cmd, struct arg *a);
int permission_arg(struct cmd_context *cmd, struct arg *a);
int metadatatype_arg(struct cmd_context *cmd, struct arg *a);
int units_arg(struct cmd_context *cmd, struct arg *a);
int segtype_arg(struct cmd_context *cmd, struct arg *a);
int alloc_arg(struct cmd_context *cmd, struct arg *a);
int readahead_arg(struct cmd_context *cmd, struct arg *a);

/* we use the enums to access the switches */
unsigned arg_count(const struct cmd_context *cmd, int a);
unsigned arg_is_set(const struct cmd_context *cmd, int a);
const char *arg_value(struct cmd_context *cmd, int a);
const char *arg_str_value(struct cmd_context *cmd, int a, const char *def);
int32_t arg_int_value(struct cmd_context *cmd, int a, const int32_t def); 
uint32_t arg_uint_value(struct cmd_context *cmd, int a, const uint32_t def);
int64_t arg_int64_value(struct cmd_context *cmd, int a, const int64_t def);
uint64_t arg_uint64_value(struct cmd_context *cmd, int a, const uint64_t def);
const void *arg_ptr_value(struct cmd_context *cmd, int a, const void *def);
sign_t arg_sign_value(struct cmd_context *cmd, int a, const sign_t def);
percent_t arg_percent_value(struct cmd_context *cmd, int a, const percent_t def);
int arg_count_increment(struct cmd_context *cmd, int a);

const char *command_name(struct cmd_context *cmd);

int pvmove_poll(struct cmd_context *cmd, const char *pv, unsigned background);
int lvconvert_poll(struct cmd_context *cmd, struct logical_volume *lv, unsigned background);

#endif
