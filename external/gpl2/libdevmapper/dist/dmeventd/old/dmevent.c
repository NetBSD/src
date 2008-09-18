/*
 * Copyright (C) 2005 Red Hat, Inc. All rights reserved.
 *
 * This file is part of the device-mapper userspace tools.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "libdevmapper.h"
#include "libdm-event.h"
#include "libmultilog.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>

static enum event_type events = ALL_ERRORS; /* All until we can distinguish. */
static char default_dso_name[] = "noop";  /* default DSO is noop */
static int default_reg = 1;		 /* default action is register */
static uint32_t timeout;

struct event_ops {
int (*dm_register_for_event)(char *dso_name, char *device,
			     enum event_type event_types);
int (*dm_unregister_for_event)(char *dso_name, char *device,
			       enum event_type event_types);
int (*dm_get_registered_device)(char **dso_name, char **device,
				enum event_type *event_types, int next);
int (*dm_set_event_timeout)(char *device, uint32_t time);
int (*dm_get_event_timeout)(char *device, uint32_t *time);
};

/* Display help. */
static void print_usage(char *name)
{
	char *cmd = strrchr(name, '/');

	cmd = cmd ? cmd + 1 : name;
	printf("Usage::\n"
	       "%s [options] <device>\n"
	       "\n"
	       "Options:\n"
	       "  -d <dso>           Specify the DSO to use.\n"
	       "  -h                 Print this usage.\n"
	       "  -l                 List registered devices.\n"
	       "  -r                 Register for event (default).\n"
	       "  -t <timeout>       (un)register for timeout event.\n"
	       "  -u                 Unregister for event.\n"
	       "\n", cmd);
}

/* Parse command line arguments. */
static int parse_argv(int argc, char **argv, char **dso_name_arg,
		      char **device_arg, int *reg, int *list)
{
	int c;
	const char *options = "d:hlrt:u";

	while ((c = getopt(argc, argv, options)) != -1) {
		switch (c) {
		case 'd':
			*dso_name_arg = optarg;
			break;
		case 'h':
			print_usage(argv[0]);
			exit(EXIT_SUCCESS);
		case 'l':
			*list = 1;
			break;
		case 'r':
			*reg = 1;
			break;
		case 't':
			events = TIMEOUT;
			if (sscanf(optarg, "%"SCNu32, &timeout) != 1){
				fprintf(stderr, "invalid timeout '%s'\n",
					optarg);
				timeout = 0;
			}
			break;
		case 'u':
			*reg = 0;
			break;
		default:
			fprintf(stderr, "Unknown option '%c'.\n"
				"Try '-h' for help.\n", c);

			return 0;
		}
	}

	if (optind >= argc) {
		if (!*list) {
			fprintf(stderr, "You need to specify a device.\n");
			return 0;
		}
	} else 
		*device_arg = argv[optind];

	return 1;
}


static int lookup_symbol(void *dl, void **symbol, const char *name)
{
	if ((*symbol = dlsym(dl, name)))
		return 1;

	fprintf(stderr, "error looking up %s symbol: %s\n", name, dlerror());

	return 0;
}

static int lookup_symbols(void *dl, struct event_ops *e)
{
	return lookup_symbol(dl, (void *) &e->dm_register_for_event,
			     "dm_register_for_event") &&
	       lookup_symbol(dl, (void *) &e->dm_unregister_for_event,
			     "dm_unregister_for_event") &&
	       lookup_symbol(dl, (void *) &e->dm_get_registered_device,
			     "dm_get_registered_device") &&
	       lookup_symbol(dl, (void *) &e->dm_set_event_timeout,
			     "dm_set_event_timeout") &&
	       lookup_symbol(dl, (void *) &e->dm_get_event_timeout,
			     "dm_get_event_timeout");
}

int main(int argc, char **argv)
{
	void *dl;
	struct event_ops e;
	int list = 0, next = 0, ret, reg = default_reg;
	char *device, *device_arg = NULL, *dso_name, *dso_name_arg = NULL;

	if (!parse_argv(argc, argv, &dso_name_arg, &device_arg, &reg, &list))
		exit(EXIT_FAILURE);

	if (device_arg) {
		if (!(device = strdup(device_arg)))
			exit(EXIT_FAILURE);
	} else
		device = NULL;

	if (dso_name_arg) {
		if (!(dso_name = strdup(dso_name_arg)))
			exit(EXIT_FAILURE);
	} else {
		if (!(dso_name = strdup(default_dso_name)))
			exit(EXIT_FAILURE);
	}

	/* FIXME: use -v/-q options to set this */
	multilog_add_type(standard, NULL);
	multilog_init_verbose(standard, _LOG_DEBUG);

	if (!(dl = dlopen("libdmevent.so", RTLD_NOW))){
		fprintf(stderr, "Cannot dlopen libdmevent.so: %s\n", dlerror());
		goto out;
	}
	if (!(lookup_symbols(dl, &e)))
		goto out;
	if (list) {
		while (1) {
			if ((ret= e.dm_get_registered_device(&dso_name,
							     &device,
							     &events, next)))
				break;
			printf("%s %s 0x%x", dso_name, device, events);
			if (events & TIMEOUT){
				if ((ret = e.dm_get_event_timeout(device,
							  	  &timeout))) {
					ret = EXIT_FAILURE;
					goto out;
				}
				printf(" %"PRIu32"\n", timeout);
			} else
				printf("\n");
			if (device_arg)
				break;

			next = 1;
		}

		ret = (ret && device_arg) ? EXIT_FAILURE : EXIT_SUCCESS;
		goto out;
	}

	if ((ret = reg ? e.dm_register_for_event(dso_name, device, events) :
			 e.dm_unregister_for_event(dso_name, device, events))) {
		fprintf(stderr, "Failed to %sregister %s: %s\n",
			reg ? "": "un", device, strerror(-ret));
		ret = EXIT_FAILURE;
	} else {
		if (reg && (events & TIMEOUT) &&
		    ((ret = e.dm_set_event_timeout(device, timeout)))){
			fprintf(stderr, "Failed to set timeout for %s: %s\n",
				device, strerror(-ret));
			ret = EXIT_FAILURE;
		} else {
			printf("%s %sregistered successfully.\n",
			       device, reg ? "" : "un");
			ret = EXIT_SUCCESS;
		}
	}

   out:
	multilog_del_type(standard);

	free(device);
	free(dso_name);

	exit(ret);
}

/*
 * Overrides for Emacs so that we follow Linus's tabbing style.
 * Emacs will notice this stuff at the end of the file and automatically
 * adjust the settings for this buffer only.  This must remain at the end
 * of the file.
 * ---------------------------------------------------------------------------
 * Local variables:
 * c-file-style: "linux"
 * End:
 */
