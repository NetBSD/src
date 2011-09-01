/*	Id: driver.c,v 1.7 2011/06/03 15:34:01 plunky Exp 	*/	
/*	$NetBSD: driver.c,v 1.1.1.1 2011/09/01 12:47:04 plunky Exp $	*/

/*-
 * Copyright (c) 2011 Joerg Sonnenberger <joerg@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/wait.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "driver.h"
#include "xalloc.h"

#include "config.h"

static volatile sig_atomic_t exit_now;
static volatile sig_atomic_t child;

static void
sigterm_handler(int signum)
{
	exit_now = 1;
	if (child)
		kill(child, SIGTERM);
}

static const char versionstr[] = VERSSTR;

enum phases { DEFAULT, PREPROCESS, COMPILE, ASSEMBLE, LINK } last_phase =
    DEFAULT;

const char *isysroot = NULL;
const char *sysroot = "";
const char *preprocessor;
const char *compiler;
const char *assembler;
const char *linker;

struct strlist crtdirs;
static struct strlist user_sysincdirs;
struct strlist sysincdirs;
struct strlist includes;
struct strlist incdirs;
struct strlist libdirs;
struct strlist progdirs;
struct strlist preprocessor_flags;
struct strlist compiler_flags;
struct strlist assembler_flags;
struct strlist early_linker_flags;
struct strlist middle_linker_flags;
struct strlist late_linker_flags;
struct strlist stdlib_flags;
struct strlist early_program_csu_files;
struct strlist late_program_csu_files;
struct strlist early_dso_csu_files;
struct strlist late_dso_csu_files;
struct strlist temp_outputs;

const char *final_output;
static char *temp_directory;
static struct strlist inputs;

int pic_mode; /* 0: no PIC, 1: -fpic, 2: -fPIC */
int save_temps;
int debug_mode;
int profile_mode;
int nostdinc;
int nostdlib;
int nostartfiles;
int static_mode;
int shared_mode;
int use_pthread;
int verbose_mode;

void
error(const char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);
	vfprintf(stderr, fmt, arg);
	putc('\n', stderr);
	va_end(arg);
	exit(1);
}

static void
warning(const char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);
	vfprintf(stderr, fmt, arg);
	putc('\n', stderr);
	va_end(arg);
}

static void
set_last_phase(enum phases phase)
{
	assert(phase != DEFAULT);
	if (last_phase != DEFAULT && phase != last_phase)
		error("conflicting compiler options specified");
	last_phase = phase;
}

static void
expand_sysroot(void)
{
	struct string *s;
	struct strlist *lists[] = { &crtdirs, &sysincdirs, &incdirs,
	    &user_sysincdirs, &libdirs, &progdirs, NULL };
	const char *sysroots[] = { sysroot, isysroot, isysroot, isysroot,
	    sysroot, sysroot, NULL };
	size_t i, sysroot_len, value_len;
	char *path;

	assert(sizeof(lists) / sizeof(lists[0]) ==
	       sizeof(sysroots) / sizeof(sysroots[0]));

	for (i = 0; lists[i] != NULL; ++i) {
		STRLIST_FOREACH(s, lists[i]) {
			if (s->value[0] != '=')
				continue;
			sysroot_len = strlen(sysroots[i]);
			/* Skipped '=' compensates additional space for '\0' */
			value_len = strlen(s->value);
			path = xmalloc(sysroot_len + value_len);
			memcpy(path, sysroots[i], sysroot_len);
			memcpy(path + sysroot_len, s->value + 1, value_len);
			free(s->value);
			s->value = path;
		}
	}
}

static void
missing_argument(const char *argp)
{
	error("Option `%s' required an argument", argp);
}

static void
split_and_append(struct strlist *l, char *arg)
{
	char *next;

	for (; arg != NULL; arg = NULL) {
		next = strchr(arg, ',');
		if (next != NULL)
			*next++ = '\0';
		strlist_append(l, arg);
	}
}

static int
strlist_exec(struct strlist *l)
{
	char **argv;
	size_t argc;
	int result;

	strlist_make_array(l, &argv, &argc);
	if (verbose_mode) {
		printf("Calling ");
		strlist_print(l, stdout);
		printf("\n");
	}

	if (exit_now)
		return 1;

	switch ((child = fork())) {
	case 0:
		execvp(argv[0], argv);
		result = write(STDERR_FILENO, "Exec of ", 8);
		result = write(STDERR_FILENO, argv[0], strlen(argv[0]));
		result = write(STDERR_FILENO, "failed\n", 7);
		(void)result;
		_exit(127);
	case -1:
		error("fork failed");
	default:
		while (waitpid(child, &result, 0) == -1 && errno == EINTR)
			/* nothing */(void)0;
		result = WEXITSTATUS(result);
		if (result)
			error("%s terminated with status %d", argv[0], result);
		while (argc-- > 0)
			free(argv[argc]);
		free(argv);
		break;
	}
	return exit_now;
}

static char *
find_file(const char *file, struct strlist *path, int mode)
{
	struct string *s;
	char *f;
	size_t lf, lp;
	int need_sep;

	lf = strlen(file);
	STRLIST_FOREACH(s, path) {
		lp = strlen(s->value);
		need_sep = (lp && s->value[lp - 1] != '/') ? 1 : 0;
		f = xmalloc(lp + lf + need_sep + 1);
		memcpy(f, s->value, lp);
		if (need_sep)
			f[lp] = '/';
		memcpy(f + lp + need_sep, file, lf + 1);
		if (access(f, mode) == 0)
			return f;
		free(f);
	}
	return xstrdup(file);
}

static char *
output_name(const char *file, const char *new_suffix, int counter, int last)
{
	const char *old_suffix;
	char *name;
	size_t lf, ls, len;
	int counter_len;

	if (last && final_output)
		return xstrdup(final_output);

	old_suffix = strrchr(file, '.');
	if (old_suffix != NULL && strchr(old_suffix, '/') != NULL)
		old_suffix = NULL;
	if (old_suffix == NULL)
		old_suffix = file + strlen(file);

	ls = strlen(new_suffix);
	if (save_temps || last) {
		lf = old_suffix - file;
		name = xmalloc(lf + ls + 1);
		memcpy(name, file, lf);
		memcpy(name + lf, new_suffix, ls + 1);
		return name;
	}
	if (temp_directory == NULL) {
		const char *template;
		char *path;
		size_t template_len;
		int need_sep;

		template = getenv("TMPDIR");
		if (template == NULL)
			template = "/tmp";
		template_len = strlen(template);
		if (template_len && template[template_len - 1] == '/')
			need_sep = 0;
		else
			need_sep = 1;
		path = xmalloc(template_len + need_sep + 6 + 1);
		memcpy(path, template, template_len);
		if (need_sep)
			path[template_len] = '/';
		memcpy(path + template_len + need_sep, "pcc-XXXXXX", 11);
		if (mkdtemp(path) == NULL)
			error("mkdtemp failed: %s", strerror(errno));
		temp_directory = path;
	}
	lf = strlen(temp_directory);
	counter_len = snprintf(NULL, 0, "%d", counter);
	if (counter_len < 1)
		error("snprintf failure");
	len = lf + 1 + (size_t)counter_len + ls + 1;
	name = xmalloc(len);
	snprintf(name, len, "%s/%d%s", temp_directory, counter, new_suffix);
	strlist_append(&temp_outputs, name);
	return name;
}

static int
preprocess_input(const char *file, char *input, char **output,
    const char *suffix, int counter)
{
	struct strlist args;
	struct string *s;
	char *out;
	int retval;

	strlist_init(&args);
	strlist_append_list(&args, &preprocessor_flags);
	STRLIST_FOREACH(s, &includes) {
		strlist_append(&args, "-i");
		strlist_append(&args, s->value);
	}
	STRLIST_FOREACH(s, &incdirs) {
		strlist_append(&args, "-I");
		strlist_append(&args, s->value);
	}
	STRLIST_FOREACH(s, &user_sysincdirs) {
		strlist_append(&args, "-S");
		strlist_append(&args, s->value);
	}
	if (!nostdinc) {
		STRLIST_FOREACH(s, &sysincdirs) {
			strlist_append(&args, "-S");
			strlist_append(&args, s->value);
		}
	}
	strlist_append(&args, input);
	if (last_phase == PREPROCESS && final_output == NULL)
		out = xstrdup("-");
	else
		out = output_name(file, suffix, counter,
		    last_phase == PREPROCESS);
	if (strcmp(out, "-"))
		strlist_append(&args, out);
	strlist_prepend(&args, find_file(preprocessor, &progdirs, X_OK));
	*output = out;
	retval = strlist_exec(&args);
	strlist_free(&args);
	return retval;
}

static int
compile_input(const char *file, char *input, char **output,
    const char *suffix, int counter)
{
	struct strlist args;
	char *out;
	int retval;

	strlist_init(&args);
	strlist_append_list(&args, &compiler_flags);
	if (debug_mode)
		strlist_append(&args, "-g");
	if (pic_mode)
		strlist_append(&args, "-k");
	if (profile_mode)
		warning("-pg is currently ignored");
	strlist_append(&args, input);
	out = output_name(file, suffix, counter, last_phase == ASSEMBLE);
	strlist_append(&args, out);
	strlist_prepend(&args, find_file(compiler, &progdirs, X_OK));
	*output = out;
	retval = strlist_exec(&args);
	strlist_free(&args);
	return retval;
}

static int
assemble_input(const char *file, char *input, char **output,
    const char *suffix, int counter)
{
	struct strlist args;
	char *out;
	int retval;

	strlist_init(&args);
	strlist_append_list(&args, &assembler_flags);
	strlist_append(&args, input);
	out = output_name(file, ".o", counter, last_phase == COMPILE);
	strlist_append(&args, "-o");
	strlist_append(&args, out);
	strlist_prepend(&args, find_file(assembler, &progdirs, X_OK));
	*output = out;
	retval = strlist_exec(&args);
	strlist_free(&args);
	return retval;
}

static int
handle_input(const char *file)
{
	static int counter;
	const char *suffix;
	char *src;
	int handled, retval;

	++counter;

	if (strcmp(file, "-") == 0) {
		/* XXX see -x option */
		suffix = ".c";
	} else {
		suffix = strrchr(file, '.');
		if (suffix != NULL && strchr(suffix, '/') != NULL)
			suffix = NULL;
		if (suffix == NULL)
			suffix = "";
	}

	src = xstrdup(file);
	if (strcmp(suffix, ".c") == 0) {
		suffix = ".i";
		retval = preprocess_input(file, src, &src, suffix, counter);
		if (retval)
			return retval;
		handled = 1;
	} else if (strcmp(suffix, ".S") == 0) {
		suffix = ".s";
		retval = preprocess_input(file, src, &src, suffix, counter);
		if (retval)
			return retval;
		handled = 1;
	}

	if (last_phase == PREPROCESS)
		goto done;

	if (strcmp(suffix, ".i") == 0) {
		suffix = ".s";
		retval = compile_input(file, src, &src, suffix, counter);
		if (retval)
			return retval;
		handled = 1;
	}
	if (last_phase == ASSEMBLE)
		goto done;

	if (strcmp(suffix, ".s") == 0) {
		suffix = ".o";
		retval = assemble_input(file, src, &src, suffix, counter);
		if (retval)
			return retval;
		handled = 1;
	}
	if (last_phase == COMPILE)
		goto done;
	if (strcmp(suffix, ".o") == 0)
		handled = 1;
	strlist_append(&middle_linker_flags, src);
done:
	if (handled)
		return 0;
	if (last_phase == LINK)
		warning("unknown suffix %s, passing file down to linker",
		    suffix);
	else
		warning("unknown suffix %s, skipped", suffix);
	free(src);
	return 0;
}

static int
run_linker(void)
{
	struct strlist linker_flags;
	struct strlist *early_csu, *late_csu;
	struct string *s;
	int retval;

	if (final_output) {
		strlist_prepend(&early_linker_flags, final_output);
		strlist_prepend(&early_linker_flags, "-o");
	}
	if (!nostdlib)
		strlist_append_list(&late_linker_flags, &stdlib_flags);
	if (!nostartfiles) {
		if (shared_mode) {
			early_csu = &early_dso_csu_files;
			late_csu = &late_dso_csu_files;
		} else {
			early_csu = &early_program_csu_files;
			late_csu = &late_program_csu_files;
		}
		STRLIST_FOREACH(s, early_csu)
			strlist_append_nocopy(&middle_linker_flags,
			    find_file(s->value, &crtdirs, R_OK));
		STRLIST_FOREACH(s, late_csu)
			strlist_append_nocopy(&late_linker_flags,
			    find_file(s->value, &crtdirs, R_OK));
	}
	strlist_init(&linker_flags);
	strlist_append_list(&linker_flags, &early_linker_flags);
	strlist_append_list(&linker_flags, &middle_linker_flags);
	strlist_append_list(&linker_flags, &late_linker_flags);
	strlist_prepend(&linker_flags, find_file(linker, &progdirs, X_OK));

	retval = strlist_exec(&linker_flags);

	strlist_free(&linker_flags);
	return retval;
}

static void
cleanup(void)
{
	struct string *file;

	STRLIST_FOREACH(file, &temp_outputs) {
		if (unlink(file->value) == -1)
			warning("removal of ``%s'' failed: %s", file->value,
			    strerror(errno));
	}
	if (temp_directory && rmdir(temp_directory) == -1)
		warning("removal of ``%s'' failed: %s", temp_directory,
		    strerror(errno));
}

int
main(int argc, char **argv)
{
	struct string *input;
	char *argp;
	int retval;

	strlist_init(&crtdirs);
	strlist_init(&user_sysincdirs);
	strlist_init(&sysincdirs);
	strlist_init(&incdirs);
	strlist_init(&includes);
	strlist_init(&libdirs);
	strlist_init(&progdirs);
	strlist_init(&inputs);
	strlist_init(&preprocessor_flags);
	strlist_init(&compiler_flags);
	strlist_init(&assembler_flags);
	strlist_init(&early_linker_flags);
	strlist_init(&middle_linker_flags);
	strlist_init(&late_linker_flags);
	strlist_init(&stdlib_flags);
	strlist_init(&early_program_csu_files);
	strlist_init(&late_program_csu_files);
	strlist_init(&early_dso_csu_files);
	strlist_init(&late_dso_csu_files);
	strlist_init(&temp_outputs);

	init_platform_specific(TARGOS, TARGMACH);

	while (--argc) {
		++argv;
		argp = *argv;

		if (*argp != '-' || strcmp(argp, "-") == 0) {
			strlist_append(&inputs, argp);
			continue;
		}
		switch (argp[1]) {
		case '-':
			if (strcmp(argp, "--param") == 0) {
				if (argc == 0)
					missing_argument(argp);
				--argc;
				++argv;
				/* Unused */
				continue;
			}
			if (strncmp(argp, "--sysroot=", 10) == 0) {
				sysroot = argp + 10;
				continue;
			}
			if (strcmp(argp, "--version") == 0) {
				printf("%s\n", versionstr);
				exit(0);
			}
			break;
		case 'B':
			strlist_append(&crtdirs, argp);
			strlist_append(&libdirs, argp);
			strlist_append(&progdirs, argp);
			continue;
		case 'C':
			if (argp[2] == '\0') {
				strlist_append(&preprocessor_flags, argp);
				continue;
			}
			break;
		case 'c':
			if (argp[2] == '\0') {
				set_last_phase(COMPILE);
				continue;
			}
			break;
		case 'D':
			strlist_append(&preprocessor_flags, argp);
			if (argp[2] == '\0') {
				if (argc == 0)
					missing_argument(argp);
				--argc;
				++argv;
				strlist_append(&preprocessor_flags, argp);
			}
			continue;
		case 'E':
			if (argp[2] == '\0') {
				set_last_phase(PREPROCESS);
				continue;
			}
			break;
		case 'f':
			if (strcmp(argp, "-fpic") == 0) {
				pic_mode = 1;
				continue;
			}
			if (strcmp(argp, "-fPIC") == 0) {
				pic_mode = 2;
				continue;
			}
			/* XXX GCC options */
			break;
		case 'g':
			if (argp[2] == '\0') {
				debug_mode = 1;
				continue;
			}
			/* XXX allow variants like -g1? */
			break;
		case 'I':
			if (argp[2] == '\0') {
				if (argc == 0)
					missing_argument(argp);
				--argc;
				++argv;
				strlist_append(&incdirs, argp);
				continue;
			}
			strlist_append(&incdirs, argp + 2);
			continue;
		case 'i':
			if (strcmp(argp, "-isystem") == 0) {
				if (argc == 0)
					missing_argument(argp);
				--argc;
				++argv;
				strlist_append(&user_sysincdirs, argp);
				continue;
			}
			if (strcmp(argp, "-include") == 0) {
				if (argc == 0)
					missing_argument(argp);
				--argc;
				++argv;
				strlist_append(&includes, argp);
				continue;
			}
			if (strcmp(argp, "-isysroot") == 0) {
				if (argc == 0)
					missing_argument(argp);
				--argc;
				++argv;
				isysroot = argp;
				continue;
			}
			/* XXX -idirafter */
			/* XXX -iquote */
			break;
		case 'k':
			if (argp[2] == '\0') {
				pic_mode = 1;
				continue;
			}
			break;
		case 'M':
			if (argp[2] == '\0') {
				strlist_append(&preprocessor_flags, argp);
				continue;
			}
			break;
		case 'm':
			/* XXX implement me */
			break;
		case 'n':
			if (strcmp(argp, "-nostdinc") == 0) {
				nostdinc = 1;
				continue;
			}
			if (strcmp(argp, "-nostdinc++") == 0)
				continue;
			if (strcmp(argp, "-nostdlib") == 0) {
				nostdlib = 1;
				nostartfiles = 1;
				continue;
			}
			if (strcmp(argp, "-nostartfiles") == 0) {
				nostartfiles = 1;
				continue;
			}
			break;
		case 'O':
			if (argp[2] != '\0' && argp[3] != '\0')
				break;
			switch(argp[2]) {
			case '2':
			case '1': case '\0':
				strlist_append(&compiler_flags, "-xtemps");
				strlist_append(&compiler_flags, "-xdeljumps");
				strlist_append(&compiler_flags, "-xinline");
			case '0':
				continue;
			}
			break;
		case 'o':
			if (argp[2] == '\0') {
				if (argc == 0)
					missing_argument(argp);
				--argc;
				++argv;
				if (final_output)
					error("Only one `-o' option allowed");
				final_output = *argv;
				continue;
			}
			break;
		case 'p':
			if (argp[2] == '\0' || strcmp(argp, "-pg") == 0) {
				profile_mode = 1;
				continue;
			}
			if (strcmp(argp, "-pedantic") == 0)
				continue;
			if (strcmp(argp, "-pipe") == 0)
				continue; /* XXX implement me */
			if (strcmp(argp, "-pthread") == 0) {
				use_pthread = 1;
				continue;
			}
			/* XXX -print-prog-name=XXX */
			/* XXX -print-multi-os-directory */
			break;
		case 'r':
			if (argp[2] == '\0') {
				strlist_append(&middle_linker_flags, argp);
				continue;
			}
			break;
		case 'S':
			if (argp[2] == '\0') {
				set_last_phase(ASSEMBLE);
				continue;
			}
			break;
		case 's':
			if (strcmp(argp, "-save-temps") == 0) {
				save_temps = 1;
				continue;
			}
			if (strcmp(argp, "-shared") == 0) {
				shared_mode = 1;
				continue;
			}
			if (strcmp(argp, "-static") == 0) {
				static_mode = 1;
				continue;
			}
			if (strncmp(argp, "-std=", 5) == 0)
				continue; /* XXX sanitize me */
			break;
		case 't':
			if (argp[2] == '\0') {
				strlist_append(&preprocessor_flags, argp);
				continue;
			}
		case 'U':
			strlist_append(&preprocessor_flags, argp);
			if (argp[2] == '\0') {
				if (argc == 0)
					missing_argument(argp);
				--argc;
				++argv;
				strlist_append(&preprocessor_flags, argp);
			}
			continue;
		case 'v':
			if (argp[2] == '\0') {
				verbose_mode = 1;
				continue;
			}
			break;
		case 'W':
			if (strncmp(argp, "-Wa,", 4) == 0) {
				split_and_append(&assembler_flags, argp + 4);
				continue;
			}
			if (strncmp(argp, "-Wl,", 4) == 0) {
				split_and_append(&middle_linker_flags, argp + 4);
				continue;
			}
			if (strncmp(argp, "-Wp,", 4) == 0) {
				split_and_append(&preprocessor_flags, argp + 4);
				continue;
			}
			/* XXX warning flags */
			break;
		case 'x':
			/* XXX -x c */
			/* XXX -c assembler-with-cpp */
			break;
		}
		error("unknown flag `%s'", argp);
	}

	if (last_phase == DEFAULT)
		last_phase = LINK;

	if (verbose_mode)
		printf("%s\n", versionstr);

	if (isysroot == NULL)
		isysroot = sysroot;
	expand_sysroot();

	if (last_phase != LINK && final_output && !STRLIST_EMPTY(&inputs) &&
	    !STRLIST_NEXT(STRLIST_FIRST(&inputs)))
		error("-o specified with more than one input");

	if (last_phase == PREPROCESS && final_output == NULL)
		final_output = "-";

	if (STRLIST_EMPTY(&inputs))
		error("No input specificed");

	retval = 0;

	signal(SIGTERM, sigterm_handler);

	STRLIST_FOREACH(input, &inputs) {
		if (handle_input(input->value))
			retval = 1;
	}
	if (!retval && last_phase == LINK) {
		if (run_linker())
			retval = 1;
	}

	if (exit_now)
		warning("Received signal, terminating");

	cleanup();

	strlist_free(&crtdirs);
	strlist_free(&user_sysincdirs);
	strlist_free(&sysincdirs);
	strlist_free(&incdirs);
	strlist_free(&includes);
	strlist_free(&libdirs);
	strlist_free(&progdirs);
	strlist_free(&inputs);
	strlist_free(&preprocessor_flags);
	strlist_free(&compiler_flags);
	strlist_free(&assembler_flags);
	strlist_free(&early_linker_flags);
	strlist_free(&middle_linker_flags);
	strlist_free(&late_linker_flags);
	strlist_free(&stdlib_flags);
	strlist_free(&early_program_csu_files);
	strlist_free(&late_program_csu_files);
	strlist_free(&early_dso_csu_files);
	strlist_free(&late_dso_csu_files);
	strlist_free(&temp_outputs);

	return retval;
}
