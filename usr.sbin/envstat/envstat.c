/* $NetBSD: envstat.c,v 1.88.2.3 2013/01/23 00:06:41 yamt Exp $ */

/*-
 * Copyright (c) 2007, 2008 Juan Romero Pardines.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: envstat.c,v 1.88.2.3 2013/01/23 00:06:41 yamt Exp $");
#endif /* not lint */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <paths.h>
#include <syslog.h>
#include <sys/envsys.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <prop/proplib.h>

#include "envstat.h"
#include "prog_ops.h"

#define ENVSYS_DFLAG	0x00000001	/* list registered devices */
#define ENVSYS_FFLAG	0x00000002	/* show temp in farenheit */
#define ENVSYS_LFLAG	0x00000004	/* list sensors */
#define ENVSYS_XFLAG	0x00000008	/* externalize dictionary */
#define ENVSYS_IFLAG 	0x00000010	/* skip invalid sensors */
#define ENVSYS_SFLAG	0x00000020	/* remove all properties set */
#define ENVSYS_TFLAG	0x00000040	/* make statistics */
#define ENVSYS_KFLAG	0x00000100	/* show temp in kelvin */

/* Sensors */
typedef struct envsys_sensor {
	SIMPLEQ_ENTRY(envsys_sensor) entries;
	int32_t	cur_value;
	int32_t	max_value;
	int32_t	min_value;
	int32_t	critmin_value;
	int32_t	critmax_value;
	int32_t	warnmin_value;
	int32_t	warnmax_value;
	char	desc[ENVSYS_DESCLEN];
	char	type[ENVSYS_DESCLEN];
	char	drvstate[ENVSYS_DESCLEN];
	char	battcap[ENVSYS_DESCLEN];
	char 	dvname[ENVSYS_DESCLEN];
	bool	invalid;
	bool	visible;
	bool	percentage;
} *sensor_t;

/* Sensor statistics */
typedef struct envsys_sensor_stats {
	SIMPLEQ_ENTRY(envsys_sensor_stats) entries;
	int32_t	max;
	int32_t	min;
	int32_t avg;
	char	desc[ENVSYS_DESCLEN];
} *sensor_stats_t;

/* Device properties */
typedef struct envsys_dvprops {
	uint64_t	refresh_timo;
	/* more members could be added in the future */
} *dvprops_t;

/* A simple queue to manage all sensors */
static SIMPLEQ_HEAD(, envsys_sensor) sensors_list =
    SIMPLEQ_HEAD_INITIALIZER(sensors_list);

/* A simple queue to manage statistics for all sensors */
static SIMPLEQ_HEAD(, envsys_sensor_stats) sensor_stats_list =
    SIMPLEQ_HEAD_INITIALIZER(sensor_stats_list);

static unsigned int 	interval, flags, width;
static char 		*mydevname, *sensors;
static bool 		statistics;
static u_int		header_passes;

static int 		parse_dictionary(int);
static int 		send_dictionary(FILE *);
static int 		find_sensors(prop_array_t, const char *, dvprops_t);
static void 		print_sensors(void);
static int 		check_sensors(const char *);
static int 		usage(void);

static int		sysmonfd; /* fd of /dev/sysmon */

int main(int argc, char **argv)
{
	prop_dictionary_t dict;
	int c, rval = 0;
	char *endptr, *configfile = NULL;
	FILE *cf;

	if (prog_init && prog_init() == -1)
		err(1, "init failed");

	setprogname(argv[0]);

	while ((c = getopt(argc, argv, "c:Dd:fIi:klrSs:Tw:Wx")) != -1) {
		switch (c) {
		case 'c':	/* configuration file */
			configfile = optarg;
			break;
		case 'D':	/* list registered devices */
			flags |= ENVSYS_DFLAG;
			break;
		case 'd':	/* show sensors of a specific device */
			mydevname = optarg;
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
		case 'k':	/* display temperature in Kelvin */
			flags |= ENVSYS_KFLAG;
			break;
		case 'l':	/* list sensors */
			flags |= ENVSYS_LFLAG;
			break;
		case 'r':
			/*
			 * This flag is noop.. it's only here for
			 * compatibility with the old implementation.
			 */
			break;
		case 'S':
			flags |= ENVSYS_SFLAG;
			break;
		case 's':	/* only show specified sensors */
			sensors = optarg;
			break;
		case 'T':	/* make statistics */
			flags |= ENVSYS_TFLAG;
			break;
		case 'w':	/* width value for the lines */
			width = (unsigned int)strtoul(optarg, &endptr, 10);
			if (*endptr != '\0')
				errx(EXIT_FAILURE, "bad width '%s'", optarg);
			break;
		case 'x':	/* print the dictionary in raw format */
			flags |= ENVSYS_XFLAG;
			break;
		case 'W':	/* No longer used, retained for compatibility */
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

	/* Check if we want to make statistics */
	if (flags & ENVSYS_TFLAG) {
		if (!interval)
			errx(EXIT_FAILURE,
		    	    "-T cannot be used without an interval (-i)");
		else
			statistics = true;
	}

	if (mydevname && sensors)
		errx(EXIT_FAILURE, "-d flag cannot be used with -s");

	/* Open the device in ro mode */
	if ((sysmonfd = prog_open(_PATH_SYSMON, O_RDONLY)) == -1)
		err(EXIT_FAILURE, "%s", _PATH_SYSMON);

	/* Print dictionary in raw mode */
	if (flags & ENVSYS_XFLAG) {
		rval = prop_dictionary_recv_ioctl(sysmonfd,
						  ENVSYS_GETDICTIONARY,
						  &dict);
		if (rval)
			errx(EXIT_FAILURE, "%s", strerror(rval));

		config_dict_dump(dict);

	/* Remove all properties set in dictionary */
	} else if (flags & ENVSYS_SFLAG) {
		/* Close the ro descriptor */
		(void)prog_close(sysmonfd);

		/* open the fd in rw mode */
		if ((sysmonfd = prog_open(_PATH_SYSMON, O_RDWR)) == -1)
			err(EXIT_FAILURE, "%s", _PATH_SYSMON);

		dict = prop_dictionary_create();
		if (!dict)
			err(EXIT_FAILURE, "prop_dictionary_create");

		rval = prop_dictionary_set_bool(dict,
						"envsys-remove-props",
					        true);
		if (!rval)
			err(EXIT_FAILURE, "prop_dict_set_bool");

		/* send the dictionary to the kernel now */
		rval = prop_dictionary_send_ioctl(dict, sysmonfd,
		    ENVSYS_REMOVEPROPS);
		if (rval)
			warnx("%s", strerror(rval));

	/* Set properties in dictionary */
	} else if (configfile) {
		/*
		 * Parse the configuration file.
		 */
		if ((cf = fopen(configfile, "r")) == NULL) {
			syslog(LOG_ERR, "fopen failed: %s", strerror(errno));
			errx(EXIT_FAILURE, "%s", strerror(errno));
		}

		rval = send_dictionary(cf);
		(void)fclose(cf);

	/* Show sensors with interval */
	} else if (interval) {
		for (;;) {
			rval = parse_dictionary(sysmonfd);
			if (rval)
				break;

			(void)fflush(stdout);
			(void)sleep(interval);
		}
	/* Show sensors without interval */
	} else {
		rval = parse_dictionary(sysmonfd);
	}

	(void)prog_close(sysmonfd);

	return rval ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int
send_dictionary(FILE *cf)
{
	prop_dictionary_t kdict, udict;
	int error = 0;

	/* Retrieve dictionary from kernel */
	error = prop_dictionary_recv_ioctl(sysmonfd,
	    ENVSYS_GETDICTIONARY, &kdict);
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
	(void)prog_close(sysmonfd);
	if ((sysmonfd = prog_open(_PATH_SYSMON, O_RDWR)) == -1) {
		error = errno;
		warn("%s", _PATH_SYSMON);
		return error;
	}

	/*
	 * Send our sensor properties dictionary to the kernel then.
	 */
	error = prop_dictionary_send_ioctl(udict,
	    sysmonfd, ENVSYS_SETDICTIONARY);
	if (error)
		warnx("%s", strerror(error));

	prop_object_release(udict);
	return error;
}

static sensor_stats_t
find_stats_sensor(const char *desc)
{
	sensor_stats_t stats;

	/*
	 * If we matched a sensor by its description return it, otherwise
	 * allocate a new one.
	 */
	SIMPLEQ_FOREACH(stats, &sensor_stats_list, entries)
		if (strcmp(stats->desc, desc) == 0)
			return stats;

	stats = calloc(1, sizeof(*stats));
	if (stats == NULL)
		return NULL;

	(void)strlcpy(stats->desc, desc, sizeof(stats->desc));
	stats->min = INT32_MAX;
	stats->max = INT32_MIN;
	SIMPLEQ_INSERT_TAIL(&sensor_stats_list, stats, entries);

	return stats;
}

static int
parse_dictionary(int fd)
{
	sensor_t sensor = NULL;
	dvprops_t edp = NULL;
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

	/* No drivers registered? */
	if (prop_dictionary_count(dict) == 0) {
		warnx("no drivers registered");
		goto out;
	}

	if (mydevname) {
		/* -d flag specified, print sensors only for this device */
		obj = prop_dictionary_get(dict, mydevname);
		if (prop_object_type(obj) != PROP_TYPE_ARRAY) {
			warnx("unknown device `%s'", mydevname);
			rval = EINVAL;
			goto out;
		}

		rval = find_sensors(obj, mydevname, NULL);
		if (rval)
			goto out;

	} else {
		/* print sensors for all devices registered */
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

			edp = calloc(1, sizeof(*edp));
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
			}

			free(edp);
			edp = NULL;
		}
		prop_object_iterator_release(iter);
	}

	/* print sensors now */
	if (sensors)
		rval = check_sensors(sensors);
	if ((flags & ENVSYS_LFLAG) == 0 && (flags & ENVSYS_DFLAG) == 0)
		print_sensors();
	if (interval)
		(void)printf("\n");

out:
	while ((sensor = SIMPLEQ_FIRST(&sensors_list))) {
		SIMPLEQ_REMOVE_HEAD(&sensors_list, entries);
		free(sensor);
	}
	if (edp)
		free(edp);
	prop_object_release(dict);
	return rval;
}

static int
find_sensors(prop_array_t array, const char *dvname, dvprops_t edp)
{
	prop_object_iterator_t iter;
	prop_object_t obj, obj1, obj2;
	prop_string_t state, desc = NULL;
	sensor_t sensor = NULL;
	sensor_stats_t stats = NULL;

	iter = prop_array_iterator(array);
	if (!iter)
		return ENOMEM;

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

		/* new sensor coming */
		sensor = calloc(1, sizeof(*sensor));
		if (sensor == NULL) {
			prop_object_iterator_release(iter);
			return ENOMEM;
		}

		/* copy device name */
		(void)strlcpy(sensor->dvname, dvname, sizeof(sensor->dvname));

		/* description string */
		desc = prop_dictionary_get(obj, "description");
		if (desc) {
			/* copy description */
			(void)strlcpy(sensor->desc,
			    prop_string_cstring_nocopy(desc),
		    	    sizeof(sensor->desc));
		} else {
			free(sensor);
			continue;
		}

		/* type string */
		obj1  = prop_dictionary_get(obj, "type");
		if (obj1) {
			/* copy type */
			(void)strlcpy(sensor->type,
		    	    prop_string_cstring_nocopy(obj1),
		    	    sizeof(sensor->type));
		} else {
			free(sensor);
			continue;
		}

		/* check sensor's state */
		state = prop_dictionary_get(obj, "state");

		/* mark sensors with invalid/unknown state */
		if ((prop_string_equals_cstring(state, "invalid") ||
		     prop_string_equals_cstring(state, "unknown")))
			sensor->invalid = true;

		/* get current drive state string */
		obj1 = prop_dictionary_get(obj, "drive-state");
		if (obj1) {
			(void)strlcpy(sensor->drvstate,
			    prop_string_cstring_nocopy(obj1),
			    sizeof(sensor->drvstate));
		}

		/* get current battery capacity string */
		obj1 = prop_dictionary_get(obj, "battery-capacity");
		if (obj1) {
			(void)strlcpy(sensor->battcap,
			    prop_string_cstring_nocopy(obj1),
			    sizeof(sensor->battcap));
		}

		/* get current value */
		obj1 = prop_dictionary_get(obj, "cur-value");
		if (obj1)
			sensor->cur_value = prop_number_integer_value(obj1);

		/* get max value */
		obj1 = prop_dictionary_get(obj, "max-value");
		if (obj1)
			sensor->max_value = prop_number_integer_value(obj1);

		/* get min value */
		obj1 = prop_dictionary_get(obj, "min-value");
		if (obj1)
			sensor->min_value = prop_number_integer_value(obj1);

		/* get percentage flag */
		obj1 = prop_dictionary_get(obj, "want-percentage");
		if (obj1)
			sensor->percentage = prop_bool_true(obj1);

		/* get critical max value if available */
		obj1 = prop_dictionary_get(obj, "critical-max");
		if (obj1)
			sensor->critmax_value = prop_number_integer_value(obj1);

		/* get maximum capacity value if available */
		obj1 = prop_dictionary_get(obj, "maximum-capacity");
		if (obj1)
			sensor->critmax_value = prop_number_integer_value(obj1);

		/* get critical min value if available */
		obj1 = prop_dictionary_get(obj, "critical-min");
		if (obj1)
			sensor->critmin_value = prop_number_integer_value(obj1);

		/* get critical capacity value if available */
		obj1 = prop_dictionary_get(obj, "critical-capacity");
		if (obj1)
			sensor->critmin_value = prop_number_integer_value(obj1);

		/* get warning max value if available */
		obj1 = prop_dictionary_get(obj, "warning-max");
		if (obj1)
			sensor->warnmax_value = prop_number_integer_value(obj1);

		/* get high capacity value if available */
		obj1 = prop_dictionary_get(obj, "high-capacity");
		if (obj1)
			sensor->warnmax_value = prop_number_integer_value(obj1);

		/* get warning min value if available */
		obj1 = prop_dictionary_get(obj, "warning-min");
		if (obj1)
			sensor->warnmin_value = prop_number_integer_value(obj1);

		/* get warning capacity value if available */
		obj1 = prop_dictionary_get(obj, "warning-capacity");
		if (obj1)
			sensor->warnmin_value = prop_number_integer_value(obj1);

		/* print sensor names if -l was given */
		if (flags & ENVSYS_LFLAG) {
			if (width)
				(void)printf("%*s\n", width,
				    prop_string_cstring_nocopy(desc));
			else
				(void)printf("%s\n",
				    prop_string_cstring_nocopy(desc));
		}

		/* Add the sensor into the list */
		SIMPLEQ_INSERT_TAIL(&sensors_list, sensor, entries);

		/* Collect statistics if flag enabled */
		if (statistics) {
			/* ignore sensors not relevant for statistics */
			if ((strcmp(sensor->type, "Indicator") == 0) ||
			    (strcmp(sensor->type, "Battery charge") == 0) ||
			    (strcmp(sensor->type, "Drive") == 0))
				continue;

			/* ignore invalid data */
			if (sensor->invalid)
				continue;

			/* find or allocate a new statistics sensor */
			stats = find_stats_sensor(sensor->desc);
			if (stats == NULL) {
				free(sensor);
				prop_object_iterator_release(iter);
				return ENOMEM;
			}

			/* update data */
			if (sensor->cur_value > stats->max)
				stats->max = sensor->cur_value;

			if (sensor->cur_value < stats->min)
				stats->min = sensor->cur_value;

			/* compute avg value */
			stats->avg =
			    (sensor->cur_value + stats->max + stats->min) / 3;
		}
	}

	/* free memory */
	prop_object_iterator_release(iter);
	return 0;
}

static int
check_sensors(const char *str)
{
	sensor_t sensor = NULL;
	char *dvstring, *sstring, *p, *last, *s;
	bool sensor_found = false;

	if ((s = strdup(str)) == NULL)
		return errno;

	/*
	 * Parse device name and sensor description and find out
	 * if the sensor is valid.
	 */
	for ((p = strtok_r(s, ",", &last)); p;
	     (p = strtok_r(NULL, ",", &last))) {
		/* get device name */
		dvstring = strtok(p, ":");
		if (dvstring == NULL) {
			warnx("missing device name");
			goto out;
		}

		/* get sensor description */
		sstring = strtok(NULL, ":");
		if (sstring == NULL) {
			warnx("missing sensor description");
			goto out;
		}

		SIMPLEQ_FOREACH(sensor, &sensors_list, entries) {
			/* skip until we match device */
			if (strcmp(dvstring, sensor->dvname))
				continue;
			if (strcmp(sstring, sensor->desc) == 0) {
				sensor->visible = true;
				sensor_found = true;
				break;
			}
		}
		if (sensor_found == false) {
			warnx("unknown sensor `%s' for device `%s'",
		       	    sstring, dvstring);
			goto out;
		}
		sensor_found = false;
	}

	/* check if all sensors were ok, and error out if not */
	SIMPLEQ_FOREACH(sensor, &sensors_list, entries)
		if (sensor->visible) {
			free(s);
			return 0;
		}

	warnx("no sensors selected to display");
out:
	free(s);
	return EINVAL;
}

static void
print_sensors(void)
{
	sensor_t sensor;
	sensor_stats_t stats = NULL;
	size_t maxlen = 0, ilen;
	double temp = 0;
	const char *invalid = "N/A", *degrees, *tmpstr, *stype;
	const char *a, *b, *c, *d, *e, *units;

	tmpstr = stype = d = e = NULL;

	/* find the longest description */
	SIMPLEQ_FOREACH(sensor, &sensors_list, entries)
		if (strlen(sensor->desc) > maxlen)
			maxlen = strlen(sensor->desc);

	if (width)
		maxlen = width;

	/*
	 * Print a header at the bottom only once showing different
	 * members if the statistics flag is set or not.
	 *
	 * As bonus if -s is set, only print this header every 10 iterations
	 * to avoid redundancy... like vmstat(1).
	 */

	a = "Current";
	units = "Unit";
	if (statistics) {
		b = "Max";
		c = "Min";
		d = "Avg";
	} else {
		b = "CritMax";
		c = "WarnMax";
		d = "WarnMin";
		e = "CritMin";
	}

	if (!sensors || (!header_passes && sensors) ||
	    (header_passes == 10 && sensors)) {
		if (statistics)
			(void)printf("%s%*s  %9s %8s %8s %8s %6s\n",
			    mydevname ? "" : "  ", (int)maxlen,
			    "", a, b, c, d, units);
		else
			(void)printf("%s%*s  %9s %8s %8s %8s %8s %5s\n",
			    mydevname ? "" : "  ", (int)maxlen,
			    "", a, b, c, d, e, units);
		if (sensors && header_passes == 10)
			header_passes = 0;
	}
	if (sensors)
		header_passes++;

	/* print the sensors */
	SIMPLEQ_FOREACH(sensor, &sensors_list, entries) {
		/* skip sensors that were not marked as visible */
		if (sensors && !sensor->visible)
			continue;

		/* skip invalid sensors if -I is set */
		if ((flags & ENVSYS_IFLAG) && sensor->invalid)
			continue;

		/* print device name */
		if (!mydevname) {
			if (tmpstr == NULL || strcmp(tmpstr, sensor->dvname))
				printf("[%s]\n", sensor->dvname);

			tmpstr = sensor->dvname;
		}

		/* find out the statistics sensor */
		if (statistics) {
			stats = find_stats_sensor(sensor->desc);
			if (stats == NULL) {
				/* No statistics for this sensor */
				continue;
			}
		}

		/* print sensor description */
		(void)printf("%s%*.*s", mydevname ? "" : "  ", (int)maxlen,
		    (int)maxlen, sensor->desc);

		/* print invalid string */
		if (sensor->invalid) {
			(void)printf(": %9s\n", invalid);
			continue;
		}

		/*
		 * Indicator and Battery charge sensors.
		 */
		if ((strcmp(sensor->type, "Indicator") == 0) ||
		    (strcmp(sensor->type, "Battery charge") == 0)) {

			(void)printf(":%10s", sensor->cur_value ? "TRUE" : "FALSE");

/* convert and print a temp value in degC, degF, or Kelvin */
#define PRINTTEMP(a)						\
do {								\
	if (a) {						\
		temp = ((a) / 1000000.0);			\
		if (flags & ENVSYS_FFLAG) {			\
			temp = temp * (9.0 / 5.0) - 459.67;	\
			degrees = "degF";			\
		} else if (flags & ENVSYS_KFLAG) {		\
			degrees = "K";				\
		} else {					\
			temp = temp - 273.15;			\
			degrees = "degC";			\
		}						\
		(void)printf("%*.3f ", (int)ilen, temp);	\
		ilen = 8;					\
	} else							\
		ilen += 9;					\
} while (/* CONSTCOND */ 0)

		/* temperatures */
		} else if (strcmp(sensor->type, "Temperature") == 0) {

			ilen = 10;
			degrees = "";
			(void)printf(":");
			PRINTTEMP(sensor->cur_value);
			stype = degrees;

			if (statistics) {
				/* show statistics if flag set */
				PRINTTEMP(stats->max);
				PRINTTEMP(stats->min);
				PRINTTEMP(stats->avg);
				ilen += 2;
			} else {
				PRINTTEMP(sensor->critmax_value);
				PRINTTEMP(sensor->warnmax_value);
				PRINTTEMP(sensor->warnmin_value);
				PRINTTEMP(sensor->critmin_value);
			}
			(void)printf("%*s", (int)ilen - 3, stype);
#undef PRINTTEMP

		/* fans */
		} else if (strcmp(sensor->type, "Fan") == 0) {
			stype = "RPM";

			(void)printf(":%10u ", sensor->cur_value);

			ilen = 8;
			if (statistics) {
				/* show statistics if flag set */
				(void)printf("%8u %8u %8u ",
				    stats->max, stats->min, stats->avg);
				ilen += 2;
			} else {
				if (sensor->critmax_value) {
					(void)printf("%*u ", (int)ilen,
					    sensor->critmax_value);
					ilen = 8;
				} else
					ilen += 9;

				if (sensor->warnmax_value) {
					(void)printf("%*u ", (int)ilen,
					    sensor->warnmax_value);
					ilen = 8;
				} else
					ilen += 9;

				if (sensor->warnmin_value) {
					(void)printf("%*u ", (int)ilen,
					    sensor->warnmin_value);
					ilen = 8;
				} else
					ilen += 9;

				if (sensor->critmin_value) {
					(void)printf( "%*u ", (int)ilen,
					    sensor->critmin_value);
					ilen = 8;
				} else
					ilen += 9;

			}

			(void)printf("%*s", (int)ilen - 3, stype);

		/* integers */
		} else if (strcmp(sensor->type, "Integer") == 0) {

			stype = "none";

			(void)printf(":%10d ", sensor->cur_value);

			ilen = 8;

/* Print percentage of max_value */
#define PRINTPCT(a)							\
do {									\
	if (sensor->max_value) {					\
		(void)printf("%*.3f%%", (int)ilen,			\
			((a) * 100.0) / sensor->max_value);		\
		ilen = 8;						\
	} else								\
		ilen += 9;						\
} while ( /* CONSTCOND*/ 0 )

/* Print an integer sensor value */
#define PRINTINT(a)							\
do {									\
	(void)printf("%*u ", (int)ilen, (a));				\
	ilen = 8;							\
} while ( /* CONSTCOND*/ 0 )

			if (!statistics) {
				if (sensor->percentage) {
					PRINTPCT(sensor->critmax_value);
					PRINTPCT(sensor->warnmax_value);
					PRINTPCT(sensor->warnmin_value);
					PRINTPCT(sensor->critmin_value);
				} else {
					PRINTINT(sensor->critmax_value);
					PRINTINT(sensor->warnmax_value);
					PRINTINT(sensor->warnmin_value);
					PRINTINT(sensor->critmin_value);
				}
			} else {
				if (sensor->percentage) {
					PRINTPCT(stats->max);
					PRINTPCT(stats->min);
					PRINTPCT(stats->avg);
				} else {
					PRINTINT(stats->max);
					PRINTINT(stats->min);
					PRINTINT(stats->avg);
				}
				ilen += 2;
			}

			(void)printf("%*s", (int)ilen - 3, stype);

#undef PRINTINT
#undef PRINTPCT

		/* drives  */
		} else if (strcmp(sensor->type, "Drive") == 0) {

			(void)printf(":%10s", sensor->drvstate);

		/* Battery capacity */
		} else if (strcmp(sensor->type, "Battery capacity") == 0) {

			(void)printf(":%10s", sensor->battcap);

		/* everything else */
		} else {
			if (strcmp(sensor->type, "Voltage DC") == 0)
				stype = "V";
			else if (strcmp(sensor->type, "Voltage AC") == 0)
				stype = "VAC";
			else if (strcmp(sensor->type, "Ampere") == 0)
				stype = "A";
			else if (strcmp(sensor->type, "Watts") == 0)
				stype = "W";
			else if (strcmp(sensor->type, "Ohms") == 0)
				stype = "Ohms";
			else if (strcmp(sensor->type, "Watt hour") == 0)
				stype = "Wh";
			else if (strcmp(sensor->type, "Ampere hour") == 0)
				stype = "Ah";
			else
				stype = "?";

			(void)printf(":%10.3f ",
			    sensor->cur_value / 1000000.0);

			ilen = 8;

/* Print percentage of max_value */
#define PRINTPCT(a)							\
do {									\
	if ((a) && sensor->max_value) {					\
		(void)printf("%*.3f%%", (int)ilen,			\
			((a) * 100.0) / sensor->max_value);		\
		ilen = 8;						\
	} else								\
		ilen += 9;						\
} while ( /* CONSTCOND*/ 0 )

/* Print a generic sensor value */
#define PRINTVAL(a)							\
do {									\
	if ((a)) {							\
		(void)printf("%*.3f ", (int)ilen, (a) / 1000000.0);	\
		ilen = 8;						\
	} else								\
		ilen += 9;						\
} while ( /* CONSTCOND*/ 0 )

			if (!statistics) {
				if (sensor->percentage) {
					PRINTPCT(sensor->critmax_value);
					PRINTPCT(sensor->warnmax_value);
					PRINTPCT(sensor->warnmin_value);
					PRINTPCT(sensor->critmin_value);
				} else {

					PRINTVAL(sensor->critmax_value);
					PRINTVAL(sensor->warnmax_value);
					PRINTVAL(sensor->warnmin_value);
					PRINTVAL(sensor->critmin_value);
				}
			} else {
				if (sensor->percentage) {
					PRINTPCT(stats->max);
					PRINTPCT(stats->min);
					PRINTPCT(stats->avg);
				} else {
					PRINTVAL(stats->max);
					PRINTVAL(stats->min);
					PRINTVAL(stats->avg);
				}
				ilen += 2;
			}
#undef PRINTPCT
#undef PRINTVAL

			(void)printf("%*s", (int)ilen - 3, stype);

			if (sensor->percentage && sensor->max_value) {
				(void)printf(" (%5.2f%%)",
				    (sensor->cur_value * 100.0) /
				    sensor->max_value);
			}
		}
		(void)printf("\n");
	}
}

static int
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-DfIklrSTx] ", getprogname());
	(void)fprintf(stderr, "[-c file] [-d device] [-i interval] ");
	(void)fprintf(stderr, "[-s device:sensor,...] [-w width]\n");
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}
