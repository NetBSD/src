/*	$NetBSD: lvmcmdline.c,v 1.1.1.3 2009/12/02 00:25:52 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
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

#include "tools.h"
#include "lvm2cmdline.h"
#include "label.h"
#include "lvm-version.h"

#include "stub.h"
#include "lvm2cmd.h"
#include "last-path-component.h"

#include <signal.h>
#include <syslog.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/resource.h>

#ifdef HAVE_GETOPTLONG
#  include <getopt.h>
#  define GETOPTLONG_FN(a, b, c, d, e) getopt_long((a), (b), (c), (d), (e))
#  define OPTIND_INIT 0
#else
struct option {
};
extern int optind;
extern char *optarg;
#  define GETOPTLONG_FN(a, b, c, d, e) getopt((a), (b), (c))
#  define OPTIND_INIT 1
#endif

/*
 * Table of valid switches
 */
static struct arg _the_args[ARG_COUNT + 1] = {
#define arg(a, b, c, d, e) {b, "", "--" c, d, e, 0, NULL, 0, 0, INT64_C(0), UINT64_C(0), SIGN_NONE, PERCENT_NONE, NULL},
#include "args.h"
#undef arg
};

static struct cmdline_context _cmdline;

/* Command line args */
/* FIXME: Move static _the_args into cmd? */
unsigned arg_count(const struct cmd_context *cmd __attribute((unused)), int a)
{
	return _the_args[a].count;
}

unsigned arg_is_set(const struct cmd_context *cmd, int a)
{
	return arg_count(cmd, a) ? 1 : 0;
}

const char *arg_value(struct cmd_context *cmd __attribute((unused)), int a)
{
	return _the_args[a].value;
}

const char *arg_str_value(struct cmd_context *cmd, int a, const char *def)
{
	return arg_count(cmd, a) ? _the_args[a].value : def;
}

int32_t arg_int_value(struct cmd_context *cmd, int a, const int32_t def)
{
	return arg_count(cmd, a) ? _the_args[a].i_value : def;
}

uint32_t arg_uint_value(struct cmd_context *cmd, int a, const uint32_t def)
{
	return arg_count(cmd, a) ? _the_args[a].ui_value : def;
}

int64_t arg_int64_value(struct cmd_context *cmd, int a, const int64_t def)
{
	return arg_count(cmd, a) ? _the_args[a].i64_value : def;
}

uint64_t arg_uint64_value(struct cmd_context *cmd, int a, const uint64_t def)
{
	return arg_count(cmd, a) ? _the_args[a].ui64_value : def;
}

const void *arg_ptr_value(struct cmd_context *cmd, int a, const void *def)
{
	return arg_count(cmd, a) ? _the_args[a].ptr : def;
}

sign_t arg_sign_value(struct cmd_context *cmd, int a, const sign_t def)
{
	return arg_count(cmd, a) ? _the_args[a].sign : def;
}

percent_t arg_percent_value(struct cmd_context *cmd, int a, const percent_t def)
{
	return arg_count(cmd, a) ? _the_args[a].percent : def;
}

int arg_count_increment(struct cmd_context *cmd __attribute((unused)), int a)
{
	return _the_args[a].count++;
}

int yes_no_arg(struct cmd_context *cmd __attribute((unused)), struct arg *a)
{
	a->sign = SIGN_NONE;
	a->percent = PERCENT_NONE;

	if (!strcmp(a->value, "y")) {
		a->i_value = 1;
		a->ui_value = 1;
	}

	else if (!strcmp(a->value, "n")) {
		a->i_value = 0;
		a->ui_value = 0;
	}

	else
		return 0;

	return 1;
}

int yes_no_excl_arg(struct cmd_context *cmd __attribute((unused)),
		    struct arg *a)
{
	a->sign = SIGN_NONE;
	a->percent = PERCENT_NONE;

	if (!strcmp(a->value, "e") || !strcmp(a->value, "ey") ||
	    !strcmp(a->value, "ye")) {
		a->i_value = CHANGE_AE;
		a->ui_value = CHANGE_AE;
	}

	else if (!strcmp(a->value, "y")) {
		a->i_value = CHANGE_AY;
		a->ui_value = CHANGE_AY;
	}

	else if (!strcmp(a->value, "n") || !strcmp(a->value, "en") ||
		 !strcmp(a->value, "ne")) {
		a->i_value = CHANGE_AN;
		a->ui_value = CHANGE_AN;
	}

	else if (!strcmp(a->value, "ln") || !strcmp(a->value, "nl")) {
		a->i_value = CHANGE_ALN;
		a->ui_value = CHANGE_ALN;
	}

	else if (!strcmp(a->value, "ly") || !strcmp(a->value, "yl")) {
		a->i_value = CHANGE_ALY;
		a->ui_value = CHANGE_ALY;
	}

	else
		return 0;

	return 1;
}

int metadatatype_arg(struct cmd_context *cmd, struct arg *a)
{
	struct format_type *fmt;
	char *format;

	format = a->value;

	dm_list_iterate_items(fmt, &cmd->formats) {
		if (!strcasecmp(fmt->name, format) ||
		    !strcasecmp(fmt->name + 3, format) ||
		    (fmt->alias && !strcasecmp(fmt->alias, format))) {
			a->ptr = fmt;
			return 1;
		}
	}

	return 0;
}

static int _get_int_arg(struct arg *a, char **ptr)
{
	char *val;
	long v;

	a->percent = PERCENT_NONE;

	val = a->value;
	switch (*val) {
	case '+':
		a->sign = SIGN_PLUS;
		val++;
		break;
	case '-':
		a->sign = SIGN_MINUS;
		val++;
		break;
	default:
		a->sign = SIGN_NONE;
	}

	if (!isdigit(*val))
		return 0;

	v = strtol(val, ptr, 10);

	if (*ptr == val)
		return 0;

	a->i_value = (int32_t) v;
	a->ui_value = (uint32_t) v;
	a->i64_value = (int64_t) v;
	a->ui64_value = (uint64_t) v;

	return 1;
}

/* Size stored in sectors */
static int _size_arg(struct cmd_context *cmd __attribute((unused)), struct arg *a, int factor)
{
	char *ptr;
	int i;
	static const char *suffixes = "kmgtpebs";
	char *val;
	double v;
	uint64_t v_tmp, adjustment;

	a->percent = PERCENT_NONE;

	val = a->value;
	switch (*val) {
	case '+':
		a->sign = SIGN_PLUS;
		val++;
		break;
	case '-':
		a->sign = SIGN_MINUS;
		val++;
		break;
	default:
		a->sign = SIGN_NONE;
	}

	if (!isdigit(*val))
		return 0;

	v = strtod(val, &ptr);

	if (ptr == val)
		return 0;

	if (*ptr) {
		for (i = strlen(suffixes) - 1; i >= 0; i--)
			if (suffixes[i] == tolower((int) *ptr))
				break;

		if (i < 0) {
			return 0;
		} else if (i == 7) {
			/* sectors */
			v = v;
		} else if (i == 6) {
			/* bytes */
			v_tmp = (uint64_t) v;
			adjustment = v_tmp % 512;
			if (adjustment) {
				v_tmp += (512 - adjustment);
				log_error("Size is not a multiple of 512. "
					  "Try using %"PRIu64" or %"PRIu64".",
					  v_tmp - 512, v_tmp);
				return 0;
			}
			v /= 512;
		} else {
			/* all other units: kmgtpe */
			while (i-- > 0)
				v *= 1024;
			v *= 2;
		}
	} else
		v *= factor;

	a->i_value = (int32_t) v;
	a->ui_value = (uint32_t) v;
	a->i64_value = (int64_t) v;
	a->ui64_value = (uint64_t) v;

	return 1;
}

int size_kb_arg(struct cmd_context *cmd, struct arg *a)
{
	return _size_arg(cmd, a, 2);
}

int size_mb_arg(struct cmd_context *cmd, struct arg *a)
{
	return _size_arg(cmd, a, 2048);
}

int int_arg(struct cmd_context *cmd __attribute((unused)), struct arg *a)
{
	char *ptr;

	if (!_get_int_arg(a, &ptr) || (*ptr) || (a->sign == SIGN_MINUS))
		return 0;

	return 1;
}

int int_arg_with_sign(struct cmd_context *cmd __attribute((unused)), struct arg *a)
{
	char *ptr;

	if (!_get_int_arg(a, &ptr) || (*ptr))
		return 0;

	return 1;
}

int int_arg_with_sign_and_percent(struct cmd_context *cmd __attribute((unused)),
				  struct arg *a)
{
	char *ptr;

	if (!_get_int_arg(a, &ptr))
		return 0;

	if (!*ptr)
		return 1;

	if (*ptr++ != '%')
		return 0;

	if (!strcasecmp(ptr, "V") || !strcasecmp(ptr, "VG"))
		a->percent = PERCENT_VG;
	else if (!strcasecmp(ptr, "L") || !strcasecmp(ptr, "LV"))
		a->percent = PERCENT_LV;
	else if (!strcasecmp(ptr, "P") || !strcasecmp(ptr, "PV") ||
		 !strcasecmp(ptr, "PVS"))
		a->percent = PERCENT_PVS;
	else if (!strcasecmp(ptr, "F") || !strcasecmp(ptr, "FR") ||
		 !strcasecmp(ptr, "FREE"))
		a->percent = PERCENT_FREE;
	else
		return 0;

	return 1;
}

int minor_arg(struct cmd_context *cmd __attribute((unused)), struct arg *a)
{
	char *ptr;

	if (!_get_int_arg(a, &ptr) || (*ptr) || (a->sign == SIGN_MINUS))
		return 0;

	if (a->i_value > 255) {
		log_error("Minor number outside range 0-255");
		return 0;
	}

	return 1;
}

int major_arg(struct cmd_context *cmd __attribute((unused)), struct arg *a)
{
	char *ptr;

	if (!_get_int_arg(a, &ptr) || (*ptr) || (a->sign == SIGN_MINUS))
		return 0;

	if (a->i_value > 255) {
		log_error("Major number outside range 0-255");
		return 0;
	}

	/* FIXME Also Check against /proc/devices */

	return 1;
}

int string_arg(struct cmd_context *cmd __attribute((unused)),
	       struct arg *a __attribute((unused)))
{
	return 1;
}

int tag_arg(struct cmd_context *cmd __attribute((unused)), struct arg *a)
{
	char *pos = a->value;

	if (*pos == '@')
		pos++;

	if (!validate_name(pos))
		return 0;

	a->value = pos;

	return 1;
}

int permission_arg(struct cmd_context *cmd __attribute((unused)), struct arg *a)
{
	a->sign = SIGN_NONE;

	if ((!strcmp(a->value, "rw")) || (!strcmp(a->value, "wr")))
		a->ui_value = LVM_READ | LVM_WRITE;

	else if (!strcmp(a->value, "r"))
		a->ui_value = LVM_READ;

	else
		return 0;

	return 1;
}

int alloc_arg(struct cmd_context *cmd __attribute((unused)), struct arg *a)
{
	alloc_policy_t alloc;

	a->sign = SIGN_NONE;

	alloc = get_alloc_from_string(a->value);
	if (alloc == ALLOC_INVALID)
		return 0;

	a->ui_value = (uint32_t) alloc;

	return 1;
}

int segtype_arg(struct cmd_context *cmd, struct arg *a)
{
	if (!(a->ptr = (void *) get_segtype_from_string(cmd, a->value)))
		return 0;

	return 1;
}

/*
 * Positive integer, zero or "auto".
 */
int readahead_arg(struct cmd_context *cmd __attribute((unused)), struct arg *a)
{
	if (!strcasecmp(a->value, "auto")) {
		a->ui_value = DM_READ_AHEAD_AUTO;
		return 1;
	}

	if (!strcasecmp(a->value, "none")) {
		a->ui_value = DM_READ_AHEAD_NONE;
		return 1;
	}

	if (!_size_arg(cmd, a, 1))
		return 0;

	if (a->sign == SIGN_MINUS)
		return 0;

	return 1;
}

static void __alloc(int size)
{
	if (!(_cmdline.commands = dm_realloc(_cmdline.commands, sizeof(*_cmdline.commands) * size))) {
		log_fatal("Couldn't allocate memory.");
		exit(ECMD_FAILED);
	}

	_cmdline.commands_size = size;
}

static void _alloc_command(void)
{
	if (!_cmdline.commands_size)
		__alloc(32);

	if (_cmdline.commands_size <= _cmdline.num_commands)
		__alloc(2 * _cmdline.commands_size);
}

static void _create_new_command(const char *name, command_fn command,
				unsigned flags,
				const char *desc, const char *usagestr,
				int nargs, int *args)
{
	struct command *nc;

	_alloc_command();

	nc = _cmdline.commands + _cmdline.num_commands++;

	nc->name = name;
	nc->desc = desc;
	nc->usage = usagestr;
	nc->fn = command;
	nc->flags = flags;
	nc->num_args = nargs;
	nc->valid_args = args;
}

static void _register_command(const char *name, command_fn fn, const char *desc,
			      unsigned flags, const char *usagestr, ...)
{
	int nargs = 0, i;
	int *args;
	va_list ap;

	/* count how many arguments we have */
	va_start(ap, usagestr);
	while (va_arg(ap, int) >= 0)
		 nargs++;
	va_end(ap);

	/* allocate space for them */
	if (!(args = dm_malloc(sizeof(*args) * nargs))) {
		log_fatal("Out of memory.");
		exit(ECMD_FAILED);
	}

	/* fill them in */
	va_start(ap, usagestr);
	for (i = 0; i < nargs; i++)
		args[i] = va_arg(ap, int);
	va_end(ap);

	/* enter the command in the register */
	_create_new_command(name, fn, flags, desc, usagestr, nargs, args);
}

void lvm_register_commands(void)
{
#define xx(a, b, c, d...) _register_command(# a, a, b, c, ## d, \
					    driverloaded_ARG, \
					    debug_ARG, help_ARG, help2_ARG, \
					    version_ARG, verbose_ARG, \
					    quiet_ARG, config_ARG, -1);
#include "commands.h"
#undef xx
}

static struct command *_find_command(const char *name)
{
	int i;
	const char *base;

	base = last_path_component(name);

	for (i = 0; i < _cmdline.num_commands; i++) {
		if (!strcmp(base, _cmdline.commands[i].name))
			break;
	}

	if (i >= _cmdline.num_commands)
		return 0;

	return _cmdline.commands + i;
}

static void _short_usage(const char *name)
{
	log_error("Run `%s --help' for more information.", name);
}

static int _usage(const char *name)
{
	struct command *com = _find_command(name);

	if (!com) {
		log_print("%s: no such command.", name);
		return 0;
	}

	log_print("%s: %s\n\n%s", com->name, com->desc, com->usage);
	return 1;
}

/*
 * Sets up the short and long argument.  If there
 * is no short argument then the index of the
 * argument in the the_args array is set as the
 * long opt value.  Yuck.  Of course this means we
 * can't have more than 'a' long arguments.
 */
static void _add_getopt_arg(int arg, char **ptr, struct option **o)
{
	struct arg *a = _cmdline.the_args + arg;

	if (a->short_arg) {
		*(*ptr)++ = a->short_arg;

		if (a->fn)
			*(*ptr)++ = ':';
	}
#ifdef HAVE_GETOPTLONG
	if (*(a->long_arg + 2)) {
		(*o)->name = a->long_arg + 2;
		(*o)->has_arg = a->fn ? 1 : 0;
		(*o)->flag = NULL;
		if (a->short_arg)
			(*o)->val = a->short_arg;
		else
			(*o)->val = arg;
		(*o)++;
	}
#endif
}

static struct arg *_find_arg(struct command *com, int opt)
{
	struct arg *a;
	int i, arg;

	for (i = 0; i < com->num_args; i++) {
		arg = com->valid_args[i];
		a = _cmdline.the_args + arg;

		/*
		 * opt should equal either the
		 * short arg, or the index into
		 * the_args.
		 */
		if ((a->short_arg && (opt == a->short_arg)) ||
		    (!a->short_arg && (opt == arg)))
			return a;
	}

	return 0;
}

static int _process_command_line(struct cmd_context *cmd, int *argc,
				 char ***argv)
{
	int i, opt;
	char str[((ARG_COUNT + 1) * 2) + 1], *ptr = str;
	struct option opts[ARG_COUNT + 1], *o = opts;
	struct arg *a;

	for (i = 0; i < ARG_COUNT; i++) {
		a = _cmdline.the_args + i;

		/* zero the count and arg */
		a->count = 0;
		a->value = 0;
		a->i_value = 0;
		a->ui_value = 0;
		a->i64_value = 0;
		a->ui64_value = 0;
	}

	/* fill in the short and long opts */
	for (i = 0; i < cmd->command->num_args; i++)
		_add_getopt_arg(cmd->command->valid_args[i], &ptr, &o);

	*ptr = '\0';
	memset(o, 0, sizeof(*o));

	/* initialise getopt_long & scan for command line switches */
	optarg = 0;
	optind = OPTIND_INIT;
	while ((opt = GETOPTLONG_FN(*argc, *argv, str, opts, NULL)) >= 0) {

		if (opt == '?')
			return 0;

		a = _find_arg(cmd->command, opt);

		if (!a) {
			log_fatal("Unrecognised option.");
			return 0;
		}

		if (a->count && !(a->flags & ARG_REPEATABLE)) {
			log_error("Option%s%c%s%s may not be repeated",
				  a->short_arg ? " -" : "",
				  a->short_arg ? : ' ',
				  (a->short_arg && a->long_arg) ?
				  "/" : "", a->long_arg ? : "");
			return 0;
		}

		if (a->fn) {
			if (!optarg) {
				log_error("Option requires argument.");
				return 0;
			}

			a->value = optarg;

			if (!a->fn(cmd, a)) {
				log_error("Invalid argument %s", optarg);
				return 0;
			}
		}

		a->count++;
	}

	*argc -= optind;
	*argv += optind;
	return 1;
}

static int _merge_synonym(struct cmd_context *cmd, int oldarg, int newarg)
{
	const struct arg *old;
	struct arg *new;

	if (arg_count(cmd, oldarg) && arg_count(cmd, newarg)) {
		log_error("%s and %s are synonyms.  Please only supply one.",
			  _cmdline.the_args[oldarg].long_arg, _cmdline.the_args[newarg].long_arg);
		return 0;
	}

	if (!arg_count(cmd, oldarg))
		return 1;

	old = _cmdline.the_args + oldarg;
	new = _cmdline.the_args + newarg;

	new->count = old->count;
	new->value = old->value;
	new->i_value = old->i_value;
	new->ui_value = old->ui_value;
	new->i64_value = old->i64_value;
	new->ui64_value = old->ui64_value;
	new->sign = old->sign;

	return 1;
}

int version(struct cmd_context *cmd __attribute((unused)),
	    int argc __attribute((unused)),
	    char **argv __attribute((unused)))
{
	char vsn[80];

	log_print("LVM version:     %s", LVM_VERSION);
	if (library_version(vsn, sizeof(vsn)))
		log_print("Library version: %s", vsn);
	if (driver_version(vsn, sizeof(vsn)))
		log_print("Driver version:  %s", vsn);

	return ECMD_PROCESSED;
}

static int _get_settings(struct cmd_context *cmd)
{
	cmd->current_settings = cmd->default_settings;

	if (arg_count(cmd, debug_ARG))
		cmd->current_settings.debug = _LOG_FATAL +
		    (arg_count(cmd, debug_ARG) - 1);

	if (arg_count(cmd, verbose_ARG))
		cmd->current_settings.verbose = arg_count(cmd, verbose_ARG);

	if (arg_count(cmd, quiet_ARG)) {
		cmd->current_settings.debug = 0;
		cmd->current_settings.verbose = 0;
	}

	if (arg_count(cmd, test_ARG))
		cmd->current_settings.test = arg_count(cmd, test_ARG);

	if (arg_count(cmd, driverloaded_ARG)) {
		cmd->current_settings.activation =
		    arg_int_value(cmd, driverloaded_ARG,
				  cmd->default_settings.activation);
	}

	cmd->current_settings.archive = arg_int_value(cmd, autobackup_ARG, cmd->current_settings.archive);
	cmd->current_settings.backup = arg_int_value(cmd, autobackup_ARG, cmd->current_settings.backup);
	cmd->current_settings.cache_vgmetadata = cmd->command->flags & CACHE_VGMETADATA ? 1 : 0;
	cmd->partial_activation = 0;

	if (arg_count(cmd, partial_ARG)) {
		cmd->partial_activation = 1;
		log_print("Partial mode. Incomplete volume groups will "
			  "be activated read-only.");
	}

	if (arg_count(cmd, ignorelockingfailure_ARG))
		init_ignorelockingfailure(1);
	else
		init_ignorelockingfailure(0);

	if (arg_count(cmd, nosuffix_ARG))
		cmd->current_settings.suffix = 0;

	if (arg_count(cmd, units_ARG))
		if (!(cmd->current_settings.unit_factor =
		      units_to_bytes(arg_str_value(cmd, units_ARG, ""),
				     &cmd->current_settings.unit_type))) {
			log_error("Invalid units specification");
			return EINVALID_CMD_LINE;
		}

	if (arg_count(cmd, trustcache_ARG)) {
		if (arg_count(cmd, all_ARG)) {
			log_error("--trustcache is incompatible with --all");
			return EINVALID_CMD_LINE;
		}
		init_trust_cache(1);
		log_warn("WARNING: Cache file of PVs will be trusted.  "
			  "New devices holding PVs may get ignored.");
	} else
		init_trust_cache(0);

	if (arg_count(cmd, noudevsync_ARG))
		cmd->current_settings.udev_sync = 0;

	/* Handle synonyms */
	if (!_merge_synonym(cmd, resizable_ARG, resizeable_ARG) ||
	    !_merge_synonym(cmd, allocation_ARG, allocatable_ARG) ||
	    !_merge_synonym(cmd, allocation_ARG, resizeable_ARG) ||
	    !_merge_synonym(cmd, virtualoriginsize_ARG, virtualsize_ARG) ||
	    !_merge_synonym(cmd, metadatacopies_ARG, pvmetadatacopies_ARG))
		return EINVALID_CMD_LINE;

	/* Zero indicates success */
	return 0;
}

static int _process_common_commands(struct cmd_context *cmd)
{
	if (arg_count(cmd, help_ARG) || arg_count(cmd, help2_ARG)) {
		_usage(cmd->command->name);
		return ECMD_PROCESSED;
	}

	if (arg_count(cmd, version_ARG)) {
		return version(cmd, 0, (char **) NULL);
	}

	/* Zero indicates it's OK to continue processing this command */
	return 0;
}

static void _display_help(void)
{
	int i;

	log_error("Available lvm commands:");
	log_error("Use 'lvm help <command>' for more information");
	log_error(" ");

	for (i = 0; i < _cmdline.num_commands; i++) {
		struct command *com = _cmdline.commands + i;

		log_error("%-16.16s%s", com->name, com->desc);
	}
}

int help(struct cmd_context *cmd __attribute((unused)), int argc, char **argv)
{
	int ret = ECMD_PROCESSED;

	if (!argc)
		_display_help();
	else {
		int i;
		for (i = 0; i < argc; i++)
			if (!_usage(argv[i]))
				ret = EINVALID_CMD_LINE;
	}

	return ret;
}

static void _apply_settings(struct cmd_context *cmd)
{
	init_debug(cmd->current_settings.debug);
	init_verbose(cmd->current_settings.verbose + VERBOSE_BASE_LEVEL);
	init_test(cmd->current_settings.test);
	init_full_scan_done(0);
	init_mirror_in_sync(0);

	init_msg_prefix(cmd->default_settings.msg_prefix);
	init_cmd_name(cmd->default_settings.cmd_name);

	archive_enable(cmd, cmd->current_settings.archive);
	backup_enable(cmd, cmd->current_settings.backup);

	set_activation(cmd->current_settings.activation);

	cmd->fmt = arg_ptr_value(cmd, metadatatype_ARG,
				 cmd->current_settings.fmt);
	cmd->handles_missing_pvs = 0;
}

static const char *_copy_command_line(struct cmd_context *cmd, int argc, char **argv)
{
	int i, space;

	/*
	 * Build up the complete command line, used as a
	 * description for backups.
	 */
	if (!dm_pool_begin_object(cmd->mem, 128))
		goto_bad;

	for (i = 0; i < argc; i++) {
		space = strchr(argv[i], ' ') ? 1 : 0;

		if (space && !dm_pool_grow_object(cmd->mem, "'", 1))
			goto_bad;

		if (!dm_pool_grow_object(cmd->mem, argv[i], strlen(argv[i])))
			goto_bad;

		if (space && !dm_pool_grow_object(cmd->mem, "'", 1))
			goto_bad;

		if (i < (argc - 1))
			if (!dm_pool_grow_object(cmd->mem, " ", 1))
				goto_bad;
	}

	/*
	 * Terminate.
	 */
	if (!dm_pool_grow_object(cmd->mem, "\0", 1))
		goto_bad;

	return dm_pool_end_object(cmd->mem);

      bad:
	log_error("Couldn't copy command line.");
	dm_pool_abandon_object(cmd->mem);
	return NULL;
}

int lvm_run_command(struct cmd_context *cmd, int argc, char **argv)
{
	int ret = 0;
	int locking_type;

	init_error_message_produced(0);

	/* each command should start out with sigint flag cleared */
	sigint_clear();

	if (!(cmd->cmd_line = _copy_command_line(cmd, argc, argv))) {
		stack;
		return ECMD_FAILED;
	}

	log_debug("Parsing: %s", cmd->cmd_line);

	if (!(cmd->command = _find_command(argv[0])))
		return ENO_SUCH_CMD;

	if (!_process_command_line(cmd, &argc, &argv)) {
		log_error("Error during parsing of command line.");
		return EINVALID_CMD_LINE;
	}

	set_cmd_name(cmd->command->name);

	if (arg_count(cmd, config_ARG))
		if ((ret = override_config_tree_from_string(cmd,
			     arg_str_value(cmd, config_ARG, "")))) {
			ret = EINVALID_CMD_LINE;
			goto_out;
		}

	if (arg_count(cmd, config_ARG) || !cmd->config_valid || config_files_changed(cmd)) {
		/* Reinitialise various settings inc. logging, filters */
		if (!refresh_toolcontext(cmd)) {
			log_error("Updated config file invalid. Aborting.");
			return ECMD_FAILED;
		}
	}

	if ((ret = _get_settings(cmd)))
		goto_out;
	_apply_settings(cmd);

	log_debug("Processing: %s", cmd->cmd_line);

#ifdef O_DIRECT_SUPPORT
	log_debug("O_DIRECT will be used");
#endif

	if ((ret = _process_common_commands(cmd)))
		goto_out;

	if (arg_count(cmd, nolocking_ARG))
		locking_type = 0;
	else
		locking_type = -1;

	if (!init_locking(locking_type, cmd)) {
		log_error("Locking type %d initialisation failed.",
			  locking_type);
		ret = ECMD_FAILED;
		goto out;
	}

	ret = cmd->command->fn(cmd, argc, argv);

	fin_locking();

      out:
	if (test_mode()) {
		log_verbose("Test mode: Wiping internal cache");
		lvmcache_destroy(cmd, 1);
	}

	if (cmd->cft_override) {
		destroy_config_tree(cmd->cft_override);
		cmd->cft_override = NULL;
		/* Move this? */
		if (!refresh_toolcontext(cmd))
			stack;
	}

	/* FIXME Move this? */
	cmd->current_settings = cmd->default_settings;
	_apply_settings(cmd);

	if (ret == EINVALID_CMD_LINE && !_cmdline.interactive)
		_short_usage(cmd->command->name);

	log_debug("Completed: %s", cmd->cmd_line);

	/*
	 * free off any memory the command used.
	 */
	dm_pool_empty(cmd->mem);

	reset_lvm_errno(1);

	return ret;
}

int lvm_split(char *str, int *argc, char **argv, int max)
{
	char *b = str, *e;
	*argc = 0;

	while (*b) {
		while (*b && isspace(*b))
			b++;

		if ((!*b) || (*b == '#'))
			break;

		e = b;
		while (*e && !isspace(*e))
			e++;

		argv[(*argc)++] = b;
		if (!*e)
			break;
		*e++ = '\0';
		b = e;
		if (*argc == max)
			break;
	}

	return *argc;
}

static const char *_get_cmdline(pid_t pid)
{
	static char _proc_cmdline[32];
	char buf[256];
	int fd;

	snprintf(buf, sizeof(buf), DEFAULT_PROC_DIR "/%u/cmdline", pid);
	if ((fd = open(buf, O_RDONLY)) > 0) {
		read(fd, _proc_cmdline, sizeof(_proc_cmdline) - 1);
		_proc_cmdline[sizeof(_proc_cmdline) - 1] = '\0';
		close(fd);
	} else
		_proc_cmdline[0] = '\0';

	return _proc_cmdline;
}

static const char *_get_filename(int fd)
{
	static char filename[PATH_MAX];
	char buf[32];	/* Assumes short DEFAULT_PROC_DIR */
	int size;

	snprintf(buf, sizeof(buf), DEFAULT_PROC_DIR "/self/fd/%u", fd);

	if ((size = readlink(buf, filename, sizeof(filename) - 1)) == -1)
		filename[0] = '\0';
	else
		filename[size] = '\0';

	return filename;
}

static void _close_descriptor(int fd, unsigned suppress_warnings,
			      const char *command, pid_t ppid,
			      const char *parent_cmdline)
{
	int r;
	const char *filename;

	/* Ignore bad file descriptors */
	if (fcntl(fd, F_GETFD) == -1 && errno == EBADF)
		return;

	if (!suppress_warnings)
		filename = _get_filename(fd);

	r = close(fd);
	if (suppress_warnings)
		return;

	if (!r)
		fprintf(stderr, "File descriptor %d (%s) leaked on "
			"%s invocation.", fd, filename, command);
	else if (errno == EBADF)
		return;
	else
		fprintf(stderr, "Close failed on stray file descriptor "
			"%d (%s): %s", fd, filename, strerror(errno));

	fprintf(stderr, " Parent PID %" PRIpid_t ": %s\n", ppid, parent_cmdline);
}

static void _close_stray_fds(const char *command)
{
	struct rlimit rlim;
	int fd;
	unsigned suppress_warnings = 0;
	pid_t ppid = getppid();
	const char *parent_cmdline = _get_cmdline(ppid);

	if (getrlimit(RLIMIT_NOFILE, &rlim) < 0) {
		fprintf(stderr, "getrlimit(RLIMIT_NOFILE) failed: %s\n",
			strerror(errno));
		return;
	}

	if (getenv("LVM_SUPPRESS_FD_WARNINGS"))
		suppress_warnings = 1;

	for (fd = 3; fd < rlim.rlim_cur; fd++)
		_close_descriptor(fd, suppress_warnings, command, ppid,
				  parent_cmdline);
}

struct cmd_context *init_lvm(void)
{
	struct cmd_context *cmd;

	_cmdline.the_args = &_the_args[0];

	if (!(cmd = create_toolcontext(0, NULL)))
		return_NULL;

	if (stored_errno()) {
		destroy_toolcontext(cmd);
		return_NULL;
	}

	return cmd;
}

static void _fin_commands(void)
{
	int i;

	for (i = 0; i < _cmdline.num_commands; i++)
		dm_free(_cmdline.commands[i].valid_args);

	dm_free(_cmdline.commands);
}

void lvm_fin(struct cmd_context *cmd)
{
	_fin_commands();
	destroy_toolcontext(cmd);
}

static int _run_script(struct cmd_context *cmd, int argc, char **argv)
{
	FILE *script;

	char buffer[CMD_LEN];
	int ret = 0;
	int magic_number = 0;
	char *script_file = argv[0];

	if ((script = fopen(script_file, "r")) == NULL)
		return ENO_SUCH_CMD;

	while (fgets(buffer, sizeof(buffer), script) != NULL) {
		if (!magic_number) {
			if (buffer[0] == '#' && buffer[1] == '!')
				magic_number = 1;
			else {
				ret = ENO_SUCH_CMD;
				break;
			}
		}
		if ((strlen(buffer) == sizeof(buffer) - 1)
		    && (buffer[sizeof(buffer) - 1] - 2 != '\n')) {
			buffer[50] = '\0';
			log_error("Line too long (max 255) beginning: %s",
				  buffer);
			ret = EINVALID_CMD_LINE;
			break;
		}
		if (lvm_split(buffer, &argc, argv, MAX_ARGS) == MAX_ARGS) {
			buffer[50] = '\0';
			log_error("Too many arguments: %s", buffer);
			ret = EINVALID_CMD_LINE;
			break;
		}
		if (!argc)
			continue;
		if (!strcmp(argv[0], "quit") || !strcmp(argv[0], "exit"))
			break;
		ret = lvm_run_command(cmd, argc, argv);
		if (ret != ECMD_PROCESSED) {
			if (!error_message_produced()) {
				log_debug("Internal error: Failed command did not use log_error");
				log_error("Command failed with status code %d.", ret);
			}
			break;
		}
	}

	if (fclose(script))
		log_sys_error("fclose", script_file);

	return ret;
}

/*
 * Determine whether we should fall back and exec the equivalent LVM1 tool
 */
static int _lvm1_fallback(struct cmd_context *cmd)
{
	char vsn[80];
	int dm_present;

	if (!find_config_tree_int(cmd, "global/fallback_to_lvm1",
			     DEFAULT_FALLBACK_TO_LVM1) ||
	    strncmp(cmd->kernel_vsn, "2.4.", 4))
		return 0;

	log_suppress(1);
	dm_present = driver_version(vsn, sizeof(vsn));
	log_suppress(0);

	if (dm_present || !lvm1_present(cmd))
		return 0;

	return 1;
}

static void _exec_lvm1_command(char **argv)
{
	char path[PATH_MAX];

	if (dm_snprintf(path, sizeof(path), "%s.lvm1", argv[0]) < 0) {
		log_error("Failed to create LVM1 tool pathname");
		return;
	}

	execvp(path, argv);
	log_sys_error("execvp", path);
}

static void _nonroot_warning(void)
{
	if (getuid() || geteuid())
		log_warn("WARNING: Running as a non-root user. Functionality may be unavailable.");
}

int lvm2_main(int argc, char **argv)
{
	const char *base;
	int ret, alias = 0;
	struct cmd_context *cmd;

	base = last_path_component(argv[0]);
	if (strcmp(base, "lvm") && strcmp(base, "lvm.static") &&
	    strcmp(base, "initrd-lvm"))
		alias = 1;

	_close_stray_fds(base);

	if (is_static() && strcmp(base, "lvm.static") &&
	    path_exists(LVM_SHARED_PATH) &&
	    !getenv("LVM_DID_EXEC")) {
		setenv("LVM_DID_EXEC", base, 1);
		execvp(LVM_SHARED_PATH, argv);
		unsetenv("LVM_DID_EXEC");
	}

	if (!(cmd = init_lvm()))
		return -1;

	cmd->argv = argv;
	lvm_register_commands();

	if (_lvm1_fallback(cmd)) {
		/* Attempt to run equivalent LVM1 tool instead */
		if (!alias) {
			argv++;
			argc--;
			alias = 0;
		}
		if (!argc) {
			log_error("Falling back to LVM1 tools, but no "
				  "command specified.");
			return ECMD_FAILED;
		}
		_exec_lvm1_command(argv);
		return ECMD_FAILED;
	}
#ifdef READLINE_SUPPORT
	if (!alias && argc == 1) {
		_nonroot_warning();
		ret = lvm_shell(cmd, &_cmdline);
		goto out;
	}
#endif

	if (!alias) {
		if (argc < 2) {
			log_fatal("Please supply an LVM command.");
			_display_help();
			ret = EINVALID_CMD_LINE;
			goto out;
		}

		argc--;
		argv++;
	}

	_nonroot_warning();
	ret = lvm_run_command(cmd, argc, argv);
	if ((ret == ENO_SUCH_CMD) && (!alias))
		ret = _run_script(cmd, argc, argv);
	if (ret == ENO_SUCH_CMD)
		log_error("No such command.  Try 'help'.");

	if ((ret != ECMD_PROCESSED) && !error_message_produced()) {
		log_debug("Internal error: Failed command did not use log_error");
		log_error("Command failed with status code %d.", ret);
	}

      out:
	lvm_fin(cmd);
	if (ret == ECMD_PROCESSED)
		ret = 0;
	return ret;
}
