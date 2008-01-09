/* $NetBSD: envstat.c,v 1.42.2.2 2008/01/09 02:01:59 matt Exp $ */

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

/*
 * TODO
 *
 *  o Some checks should be added to ensure that the user does not
 *    set unwanted values for the critical limits.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: envstat.c,v 1.42.2.2 2008/01/09 02:01:59 matt Exp $");
#endif /* not lint */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <syslog.h>
#include <prop/proplib.h>
#include <sys/envsys.h>
#include <sys/types.h>

#include "envstat.h"

#define _PATH_DEV_SYSMON	"/dev/sysmon"

#define ENVSYS_DFLAG	0x00000001	/* list registered devices */
#define ENVSYS_FFLAG	0x00000002	/* show temp in farenheit */
#define ENVSYS_LFLAG	0x00000004	/* list sensors */
#define ENVSYS_XFLAG	0x00000008	/* externalize dictionary */
#define ENVSYS_IFLAG 	0x00000010	/* skips invalid sensors */
#define ENVSYS_SFLAG	0x00000020	/* removes all properties set */

struct envsys_sensor {
	bool	invalid;
	bool	visible;
	bool	percentage;
	int32_t	cur_value;
	int32_t	max_value;
	int32_t	min_value;
	int32_t	avg_value;
	int32_t critcap_value;
	int32_t	critmin_value;
	int32_t	critmax_value;
	char	desc[ENVSYS_DESCLEN];
	char	type[ENVSYS_DESCLEN];
	char	drvstate[ENVSYS_DESCLEN];
	char	battcap[ENVSYS_DESCLEN];
	char 	dvname[ENVSYS_DESCLEN];
};

struct envsys_dvprops {
	uint64_t	refresh_timo;
	char 		refresh_units[ENVSYS_DESCLEN];
	/* more values could be added in the future */
};

static unsigned int interval, flags, width;
static char *mydevname, *sensors;
static struct envsys_sensor *gesen;
static size_t gnelems, newsize;

static int parse_dictionary(int);
static int send_dictionary(FILE *, int);
static int find_sensors(prop_array_t, const char *, struct envsys_dvprops *);
static void print_sensors(struct envsys_sensor *, size_t, const char *);
static int check_sensors(struct envsys_sensor *, char *, size_t);
static int usage(void);


int main(int argc, char **argv)
{
	prop_dictionary_t dict;
	int c, fd, rval;
	char *endptr, *configfile = NULL;
	FILE *cf;

	rval = flags = interval = width = 0;
	newsize = gnelems = 0;
	gesen = NULL;

	setprogname(argv[0]);

	while ((c = getopt(argc, argv, "c:Dd:fIi:lrSs:w:x")) != -1) {
		switch (c) {
		case 'c':	/* configuration file */
			configfile = strdup(optarg);
			if (configfile == NULL)
				err(EXIT_FAILURE, "strdup");
			break;
		case 'D':	/* list registered devices */
			flags |= ENVSYS_DFLAG;
			break;
		case 'd':	/* show sensors of a specific device */
			mydevname = strdup(optarg);
			if (mydevname == NULL)
				err(EXIT_FAILURE, "strdup");
			break;
		case 'f':	/* display temperature in Farenheit */
			flags |= ENVSYS_FFLAG;
			break;
		case 'I':	/* Skips invalid sensors */
			flags |= ENVSYS_IFLAG;
			break;
		case 'i':	/* wait time between intervals */
			interval = (unsigned int)strtoul(optarg, &endptr, 10);
			if (*endptr != '\0')
				errx(EXIT_FAILURE, "bad interval '%s'", optarg);
			break;
		case 'l':	/* list sensors */
			flags |= ENVSYS_LFLAG;
			break;
		case 'r':
			/* 
			 * This flag doesn't do anything... it's only here for
			 * compatibility with the old implementation.
			 */
			break;
		case 'S':
			flags |= ENVSYS_SFLAG;
			break;
		case 's':	/* only show specified sensors */
			sensors = strdup(optarg);
			if (sensors == NULL)
				err(EXIT_FAILURE, "strdup");
			break;
		case 'w':	/* width value for the lines */
			width = strtoul(optarg, &endptr, 10);
			if (*endptr != '\0')
				errx(EXIT_FAILURE, "bad width '%s'", optarg);
			break;
		case 'x':	/* print the dictionary in raw format */
			flags |= ENVSYS_XFLAG;
			break;
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	}

	argc -= optind;
	argv += optind;

	if (argc > 0)
		usage();

	if ((fd = open(_PATH_DEV_SYSMON, O_RDONLY)) == -1)
		err(EXIT_FAILURE, "%s", _PATH_DEV_SYSMON);

	if (flags & ENVSYS_XFLAG) {
		rval = prop_dictionary_recv_ioctl(fd,
						  ENVSYS_GETDICTIONARY,
						  &dict);
		if (rval)
			errx(EXIT_FAILURE, "%s", strerror(rval));

		config_dict_dump(dict);

	} else if (flags & ENVSYS_SFLAG) {
		(void)close(fd);

		if ((fd = open(_PATH_DEV_SYSMON, O_RDWR)) == -1)
			err(EXIT_FAILURE, "%s", _PATH_DEV_SYSMON);

		dict = prop_dictionary_create();
		if (!dict)
			err(EXIT_FAILURE, "prop_dictionary_create");
		
		rval = prop_dictionary_set_bool(dict,
						"envsys-remove-props",
					        true);
		if (!rval)
			err(EXIT_FAILURE, "prop_dict_set_bool");

		rval = prop_dictionary_send_ioctl(dict, fd, ENVSYS_REMOVEPROPS);
		if (rval)
			warnx("%s", strerror(rval));

	} else if (configfile) {
		/*
		 * Parse the configuration file.
		 */
		if ((cf = fopen(configfile, "r")) == NULL) {
			syslog(LOG_ERR, "fopen failed: %s", strerror(errno));
			errx(EXIT_FAILURE, "%s", strerror(errno));
		}

		rval = send_dictionary(cf, fd);
		(void)fclose(cf);

#define MISSING_FLAG()					\
do {							\
	if (sensors && !mydevname)			\
		errx(EXIT_FAILURE, "-s requires -d");	\
} while (/* CONSTCOND */ 0)

	} else if (interval) {
		MISSING_FLAG();
		for (;;) {
			rval = parse_dictionary(fd);
			if (rval)
				break;

			(void)fflush(stdout);
			(void)sleep(interval);
		}
	} else {
		MISSING_FLAG();
		rval = parse_dictionary(fd);
	}

	if (sensors)
		free(sensors);
	if (mydevname)
		free(mydevname);
	(void)close(fd);

	return rval ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int
send_dictionary(FILE *cf, int fd)
{
	prop_dictionary_t kdict, udict;
	int error = 0;

	error = prop_dictionary_recv_ioctl(fd, ENVSYS_GETDICTIONARY, &kdict);
      	if (error)
		return error;

	config_parse(cf, kdict);

	/*
	 * Dictionary built by the parser from the configuration file.
	 */
	udict = config_dict_parsed();

	/*
	 * Close the read only descriptor and open a new one read write.
	 */
	(void)close(fd);
	if ((fd = open(_PATH_DEV_SYSMON, O_RDWR)) == -1) {
		error = errno;
		warn("%s", _PATH_DEV_SYSMON);
		return error;
	}

	/* 
	 * Send our sensor properties dictionary to the kernel then.
	 */
	error = prop_dictionary_send_ioctl(udict, fd, ENVSYS_SETDICTIONARY);
	if (error)
		warnx("%s", strerror(error));

	prop_object_release(udict);
	return error;
}

static int
parse_dictionary(int fd)
{
	struct envsys_dvprops *edp = NULL;
	prop_array_t array;
	prop_dictionary_t dict;
	prop_object_iterator_t iter;
	prop_object_t obj;
	const char *dnp = NULL;
	int rval = 0;

	/* receive dictionary from kernel */
	rval = prop_dictionary_recv_ioctl(fd, ENVSYS_GETDICTIONARY, &dict);
	if (rval)
		return rval;

	if (prop_dictionary_count(dict) == 0) {
		warnx("no drivers registered");
		goto out;
	}

	if (mydevname) {
		obj = prop_dictionary_get(dict, mydevname);
		if (prop_object_type(obj) != PROP_TYPE_ARRAY) {
			warnx("unknown device `%s'", mydevname);
			rval = EINVAL;
			goto out;
		}

		rval = find_sensors(obj, mydevname, NULL);
		if (rval)
			goto out;

		if ((flags & ENVSYS_LFLAG) == 0)
			print_sensors(gesen, gnelems, mydevname);
		if (interval)
			(void)printf("\n");
	} else {
		iter = prop_dictionary_iterator(dict);
		if (iter == NULL) {
			rval = EINVAL;
			goto out;
		}

		/* iterate over the dictionary returned by the kernel */
		while ((obj = prop_object_iterator_next(iter)) != NULL) {

			array = prop_dictionary_get_keysym(dict, obj);
			if (prop_object_type(array) != PROP_TYPE_ARRAY) {
				warnx("no sensors found");
				rval = EINVAL;
				goto out;
			}

			edp = (struct envsys_dvprops *)malloc(sizeof(*edp));
			if (!edp) {
				rval = ENOMEM;
				goto out;
			}

			dnp = prop_dictionary_keysym_cstring_nocopy(obj);
			rval = find_sensors(array, dnp, edp);
			if (rval)
				goto out;

			if (((flags & ENVSYS_LFLAG) == 0) &&
			    (flags & ENVSYS_DFLAG)) {
				(void)printf("%s (checking events every ",
				    dnp);
				if (edp->refresh_timo == 1)
					(void)printf("second)\n");
				else
					(void)printf("%d seconds)\n",
					    (int)edp->refresh_timo);
				continue;
			}
			
			if ((flags & ENVSYS_LFLAG) == 0) {
				(void)printf("[%s]\n", dnp);
				print_sensors(gesen, gnelems, dnp);
			}

			if (interval)
				(void)printf("\n");

			free(edp);
			edp = NULL;
		}

		prop_object_iterator_release(iter);
	}

out:
	if (gesen) {
		free(gesen);
		gesen = NULL;
		gnelems = 0;
		newsize = 0;
	}
	if (edp)
		free(edp);
	prop_object_release(dict);
	return rval;
}

static int
find_sensors(prop_array_t array, const char *dvname, struct envsys_dvprops *edp)
{
	prop_object_iterator_t iter;
	prop_object_t obj, obj1, obj2;
	prop_string_t state, desc = NULL;
	struct envsys_sensor *esen = NULL;
	int rval = 0;
	char *str = NULL;

	newsize += prop_array_count(array) * sizeof(*gesen);
	esen = realloc(gesen, newsize);
	if (esen == NULL) {
		if (gesen)
			free(gesen);
		gesen = NULL;
		return ENOMEM;
	}
	gesen = esen;

	iter = prop_array_iterator(array);
	if (!iter)
		return EINVAL;

	/* iterate over the array of dictionaries */
	while ((obj = prop_object_iterator_next(iter)) != NULL) {

		/* get the refresh-timeout property */
		obj2 = prop_dictionary_get(obj, "device-properties");
		if (obj2) {
			if (!edp)
				continue;
			if (!prop_dictionary_get_uint64(obj2,
							"refresh-timeout",
							&edp->refresh_timo))
				continue;
		}

		/* copy device name */
		(void)strlcpy(gesen[gnelems].dvname, dvname,
		    sizeof(gesen[gnelems].dvname));

		gesen[gnelems].visible = false;

		/* check sensor's state */
		state = prop_dictionary_get(obj, "state");

		/* mark sensors with invalid/unknown state */
		if ((prop_string_equals_cstring(state, "invalid") ||
		     prop_string_equals_cstring(state, "unknown")))
			gesen[gnelems].invalid = true;
		else
			gesen[gnelems].invalid = false;

		/* description string */
		desc = prop_dictionary_get(obj, "description");
		if (desc) {
			/* copy description */
			(void)strlcpy(gesen[gnelems].desc,
			    prop_string_cstring_nocopy(desc),
		    	    sizeof(gesen[gnelems].desc));
		} else
			continue;

		/* type string */
		obj1  = prop_dictionary_get(obj, "type");
		/* copy type */
		(void)strlcpy(gesen[gnelems].type,
		    prop_string_cstring_nocopy(obj1),
		    sizeof(gesen[gnelems].type));

		/* get current drive state string */
		obj1 = prop_dictionary_get(obj, "drive-state");
		if (obj1)
			(void)strlcpy(gesen[gnelems].drvstate,
			    prop_string_cstring_nocopy(obj1),
			    sizeof(gesen[gnelems].drvstate));

		/* get current battery capacity string */
		obj1 = prop_dictionary_get(obj, "battery-capacity");
		if (obj1)
			(void)strlcpy(gesen[gnelems].battcap,
			    prop_string_cstring_nocopy(obj1),
			    sizeof(gesen[gnelems].battcap));

		/* get current value */
		obj1 = prop_dictionary_get(obj, "cur-value");
		gesen[gnelems].cur_value = prop_number_integer_value(obj1);

		/* get max value */
		obj1 = prop_dictionary_get(obj, "max-value");
		if (obj1)
			gesen[gnelems].max_value =
			    prop_number_integer_value(obj1);
		else
			gesen[gnelems].max_value = 0;

		/* get min value */
		obj1 = prop_dictionary_get(obj, "min-value");
		if (obj1)
			gesen[gnelems].min_value =
			    prop_number_integer_value(obj1);
		else
			gesen[gnelems].min_value = 0;

		/* get avg value */
		obj1 = prop_dictionary_get(obj, "avg-value");
		if (obj1)
			gesen[gnelems].avg_value =
			    prop_number_integer_value(obj1);
		else
			gesen[gnelems].avg_value = 0;

		/* get percentage flag */
		obj1 = prop_dictionary_get(obj, "want-percentage");
		if (obj1)
			gesen[gnelems].percentage = prop_bool_true(obj1);

		/* get critical max value if available */
		obj1 = prop_dictionary_get(obj, "critical-max");
		if (obj1) {
			gesen[gnelems].critmax_value =
			    prop_number_integer_value(obj1);
		} else
			gesen[gnelems].critmax_value = 0;

		/* get critical min value if available */
		obj1 = prop_dictionary_get(obj, "critical-min");
		if (obj1) {
			gesen[gnelems].critmin_value =
			    prop_number_integer_value(obj1);
		} else
			gesen[gnelems].critmin_value = 0;

		/* get critical capacity value if available */
		obj1 = prop_dictionary_get(obj, "critical-capacity");
		if (obj1) {
			gesen[gnelems].critcap_value =
			    prop_number_integer_value(obj1);
		} else
			gesen[gnelems].critcap_value = 0;

		/* pass to the next struct and increase the counter */
		gnelems++;

		/* print sensor names if -l was given */
		if (flags & ENVSYS_LFLAG) {
			if (width)
				(void)printf("%*s\n", width,
				    prop_string_cstring_nocopy(desc));
			else
				(void)printf("%s\n",
				    prop_string_cstring_nocopy(desc));
		}
	}

	/* free memory */
	prop_object_iterator_release(iter);

	/* 
	 * if -s was specified, we need a way to mark if a sensor
	 * was found.
	 */
	if (sensors) {
		str = strdup(sensors);
		if (!str)
			return ENOMEM;

		rval = check_sensors(gesen, str, gnelems);
		free(str);
	}

	return rval;
}

static int
check_sensors(struct envsys_sensor *es, char *str, size_t nelems)
{
	int i;
	char *sname;

	sname = strtok(str, ",");
	while (sname) {
		for (i = 0; i < nelems; i++) {
			if (strcmp(sname, es[i].desc) == 0) {
				es[i].visible = true;
				break;
			}
		}
		if (i >= nelems) {
			if (mydevname) {
				warnx("unknown sensor `%s' for device `%s'",
				    sname, mydevname);
				return EINVAL;
			}
		}
		sname = strtok(NULL, ",");
	}

	/* check if all sensors were ok, and error out if not */
	for (i = 0; i < nelems; i++) {
		if (es[i].visible)
			return 0;
	}

	warnx("no sensors selected to display");
	return EINVAL;
}

static void
print_sensors(struct envsys_sensor *es, size_t nelems, const char *dvname)
{
	size_t maxlen = 0;
	double temp = 0;
	const char *invalid = "N/A";
	const char *degrees = NULL;
	int i;

	/* find the longest description */
	for (i = 0; i < nelems; i++) {
		if (strlen(es[i].desc) > maxlen)
			maxlen = strlen(es[i].desc);
	}

	if (width)
		maxlen = width;

	/* print the sensors */
	for (i = 0; i < nelems; i++) {
		/* skip sensors that don't belong to device 'dvname' */
		if (strcmp(es[i].dvname, dvname))
			continue;

		/* skip sensors that were not marked as visible */
		if (sensors && !es[i].visible)
			continue;

		/* Do not print invalid sensors if -I is set */
		if ((flags & ENVSYS_IFLAG) && es[i].invalid)
			continue;

		(void)printf("%s%*.*s", mydevname ? "" : "  ", (int)maxlen,
		    (int)maxlen, es[i].desc);

		if (es[i].invalid) {
			(void)printf(": %10s\n", invalid);
			continue;
		}

		/*
		 * Indicator and Battery charge sensors.
		 */
		if ((strcmp(es[i].type, "Indicator") == 0) ||
		    (strcmp(es[i].type, "Battery charge") == 0)) {

			(void)printf(": %10s", es[i].cur_value ? "ON" : "OFF");

/* converts the value to degC or degF */
#define CONVERTTEMP(a, b, c)					\
do {								\
	if (b) 							\
		(a) = ((b) / 1000000.0) - 273.15;		\
	if (flags & ENVSYS_FFLAG) {				\
		if (b)						\
			(a) = (9.0 / 5.0) * (a) + 32.0;		\
		(c) = "degF";					\
	} else							\
		(c) = "degC";					\
} while (/* CONSTCOND */ 0)


		/* temperatures */
		} else if (strcmp(es[i].type, "Temperature") == 0) {

			CONVERTTEMP(temp, es[i].cur_value, degrees);
			(void)printf(": %10.3f %s", temp, degrees);
			
			if (es[i].critmax_value || es[i].critmin_value)
				(void)printf("  ");

			if (es[i].critmax_value) {
				CONVERTTEMP(temp, es[i].critmax_value, degrees);
				(void)printf("max: %8.3f %s  ", temp, degrees);
			}

			if (es[i].critmin_value) {
				CONVERTTEMP(temp, es[i].critmin_value, degrees);
				(void)printf("min: %8.3f %s", temp, degrees);
			}
#undef CONVERTTEMP

		/* fans */
		} else if (strcmp(es[i].type, "Fan") == 0) {

			(void)printf(": %10u RPM", es[i].cur_value);

			if (es[i].critmax_value || es[i].critmin_value)
				(void)printf("   ");
			if (es[i].critmax_value)
				(void)printf("max: %8u RPM   ",
				    es[i].critmax_value);
			if (es[i].critmin_value)
				(void)printf("min: %8u RPM",
				    es[i].critmin_value);

		/* integers */
		} else if (strcmp(es[i].type, "Integer") == 0) {

			(void)printf(": %10d", es[i].cur_value);

		/* drives  */
		} else if (strcmp(es[i].type, "Drive") == 0) {

			(void)printf(": %10s", es[i].drvstate);

		/* Battery capacity */
		} else if (strcmp(es[i].type, "Battery capacity") == 0) {

			(void)printf(": %10s", es[i].battcap);

		/* everything else */
		} else {
			const char *type;

			if (strcmp(es[i].type, "Voltage DC") == 0)
				type = "V";
			else if (strcmp(es[i].type, "Voltage AC") == 0)
				type = "VAC";
			else if (strcmp(es[i].type, "Ampere") == 0)
				type = "A";
			else if (strcmp(es[i].type, "Watts") == 0)
				type = "W";
			else if (strcmp(es[i].type, "Ohms") == 0)
				type = "Ohms";
			else if (strcmp(es[i].type, "Watt hour") == 0)
				type = "Wh";
			else if (strcmp(es[i].type, "Ampere hour") == 0)
				type = "Ah";
			else
				type = NULL;

			(void)printf(": %10.3f %s",
			    es[i].cur_value / 1000000.0, type);

			if (es[i].percentage && es[i].max_value) {
				(void)printf(" (%5.2f%%)",
				    (es[i].cur_value * 100.0) /
				    es[i].max_value);
			}

			if (es[i].critcap_value) {
				(void)printf(" critical (%5.2f%%)",
				    (es[i].critcap_value * 100.0) /
				    es[i].max_value);
			}

			if (es[i].critmax_value || es[i].critmin_value)
				(void)printf("     ");
			if (es[i].critmax_value)
				(void)printf("max: %8.3f %s     ",
				    es[i].critmax_value / 1000000.0,
				    type);
			if (es[i].critmin_value)
				(void)printf("min: %8.3f %s",
				    es[i].critmin_value / 1000000.0,
				    type);

		}

		(void)printf("\n");
	}
}

static int
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-DfIlrSx] ", getprogname());
	(void)fprintf(stderr, "[-c file] [-d device] [-i interval] ");
	(void)fprintf(stderr, "[-s sensor,...] [-w width]\n");
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}
