/* $NetBSD: envstat.c,v 1.105 2025/04/01 11:39:19 brad Exp $ */

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
__RCSID("$NetBSD: envstat.c,v 1.105 2025/04/01 11:39:19 brad Exp $");
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
#include <mj.h>
#include <paths.h>
#include <syslog.h>
#include <time.h>
#include <sys/envsys.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <prop/proplib.h>

#include "envstat.h"
#include "prog_ops.h"

#define ENVSYS_DFLAG	0x00000001	/* list registered devices */
#define ENVSYS_FFLAG	0x00000002	/* show temp in fahrenheit */
#define ENVSYS_LFLAG	0x00000004	/* list sensors */
#define ENVSYS_XFLAG	0x00000008	/* externalize dictionary */
#define ENVSYS_IFLAG 	0x00000010	/* skip invalid sensors */
#define ENVSYS_SFLAG	0x00000020	/* remove all properties set */
#define ENVSYS_TFLAG	0x00000040	/* make statistics */
#define ENVSYS_NFLAG	0x00000080	/* print value only */
#define ENVSYS_KFLAG	0x00000100	/* show temp in kelvin */
#define ENVSYS_JFLAG	0x00000200	/* print the output in JSON */
#define ENVSYS_tFLAG	0x00000400	/* print a timestamp */

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
	char	index[ENVSYS_DESCLEN];
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
static time_t		timestamp;

static int 		parse_dictionary(int);
static int		add_sensors(prop_dictionary_t, prop_dictionary_t, const char *, const char *);
static int 		send_dictionary(FILE *);
static int 		find_sensors(prop_array_t, const char *, dvprops_t);
static void 		print_sensors(void);
static void 		print_sensors_json(void);
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

	while ((c = getopt(argc, argv, "c:Dd:fIi:jklnrSs:Ttw:Wx")) != -1) {
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
		case 'f':	/* display temperature in Fahrenheit */
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
		case 'j':
			flags |= ENVSYS_JFLAG;
			break;
		case 'k':	/* display temperature in Kelvin */
			flags |= ENVSYS_KFLAG;
			break;
		case 'l':	/* list sensors */
			flags |= ENVSYS_LFLAG;
			break;
		case 'n':	/* print value only */
			flags |= ENVSYS_NFLAG;
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
		case 't':	/* produce a timestamp */
			flags |= ENVSYS_tFLAG;
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

	if (argc > 0 && (flags & ENVSYS_XFLAG) == 0)
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

		if (mydevname || sensors) {
			prop_dictionary_t ndict;

			ndict = prop_dictionary_create();
			if (ndict == NULL)
				err(EXIT_FAILURE, "prop_dictionary_create");

			if (mydevname) {
				if (add_sensors(ndict, dict, mydevname, NULL))
					err(EXIT_FAILURE, "add_sensors");
			}
			if (sensors) {
				char *sstring, *p, *last, *s;
				char *dvstring = NULL; /* XXXGCC */
				unsigned count = 0;

				s = strdup(sensors);
				if (s == NULL)
					err(EXIT_FAILURE, "strdup");

				for ((p = strtok_r(s, ",", &last)); p;
				     (p = strtok_r(NULL, ",", &last))) {
					/* get device name */
					dvstring = strtok(p, ":");
					if (dvstring == NULL)
						errx(EXIT_FAILURE, "missing device name");

					/* get sensor description */
					sstring = strtok(NULL, ":");
					if (sstring == NULL)
						errx(EXIT_FAILURE, "missing sensor description");

					if (add_sensors(ndict, dict, dvstring, sstring))
						err(EXIT_FAILURE, "add_sensors");

					++count;
				}
				free(s);

				/* in case we asked for a single sensor
				 * show only the sensor dictionary
				 */
				if (count == 1) {
					prop_object_t obj, obj2;

					obj = prop_dictionary_get(ndict, dvstring);
					obj2 = prop_array_get(obj, 0);
					prop_object_release(ndict);
					ndict = obj2;
				}
			}

			prop_object_release(dict);
			dict = ndict;
		}

		if (argc > 0) {
			for (; argc > 0; ++argv, --argc)
				config_dict_extract(dict, *argv, true);
		} else
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
			timestamp = time(NULL);
			rval = parse_dictionary(sysmonfd);
			if (rval)
				break;

			(void)fflush(stdout);
			(void)sleep(interval);
		}
	/* Show sensors without interval */
	} else {
		timestamp = time(NULL);
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
add_sensors(prop_dictionary_t ndict, prop_dictionary_t dict, const char *dev, const char *sensor)
{
	prop_object_iterator_t iter, iter2;
	prop_object_t obj, obj2, desc;
	prop_array_t array, narray;
	prop_dictionary_t sdict;
	const char *dnp;
	unsigned int capacity = 1;
	uint64_t dummy;
	bool found = false;

	if (prop_dictionary_count(dict) == 0)
		return 0;

	narray = prop_dictionary_get(ndict, dev);
	if (narray)
		found = true;
	else {
		narray = prop_array_create_with_capacity(capacity);
		if (!narray)
			return -1;
		if (!prop_dictionary_set(ndict, dev, narray)) {
			prop_object_release(narray);
			return -1;
		}
	}

	iter = prop_dictionary_iterator(dict);
	if (iter == NULL)
		goto fail;
	while ((obj = prop_object_iterator_next(iter)) != NULL) {
		array = prop_dictionary_get_keysym(dict, obj);
		if (prop_object_type(array) != PROP_TYPE_ARRAY)
			break;

		dnp = prop_dictionary_keysym_value(obj);
		if (strcmp(dev, dnp))
			continue;
		found = true;

		iter2 = prop_array_iterator(array);
		while ((obj = prop_object_iterator_next(iter2)) != NULL) {
			obj2 = prop_dictionary_get(obj, "device-properties");
			if (obj2) {
				if (!prop_dictionary_get_uint64(obj2,
				    "refresh-timeout", &dummy))
					continue;
			}

			if (sensor) {
				desc = prop_dictionary_get(obj, "description");
				if (desc == NULL)
					continue;

				if (!prop_string_equals_string(desc, sensor))
					continue;
			}

			if (!prop_array_ensure_capacity(narray, capacity))
				goto fail;

			sdict = prop_dictionary_copy(obj);
			if (sdict == NULL)
				goto fail;
			prop_array_add(narray, sdict);
			++capacity;
		}
		prop_object_iterator_release(iter2);
	}
	prop_object_iterator_release(iter);

	/* drop key and array when device wasn't found */
	if (!found) {
		prop_dictionary_remove(ndict, dev);
		prop_object_release(narray);
	}

	return 0;

fail:
	prop_dictionary_remove(ndict, dev);
	prop_object_release(narray);
	return -1;
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

			dnp = prop_dictionary_keysym_value(obj);
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
	if ((flags & ENVSYS_LFLAG) == 0 &&
	    (flags & ENVSYS_DFLAG) == 0 &&
	    (flags & ENVSYS_JFLAG) == 0)
		print_sensors();
	if ((flags & ENVSYS_LFLAG) == 0 &&
	    (flags & ENVSYS_DFLAG) == 0 &&
	    (flags & ENVSYS_JFLAG))
		print_sensors_json();
	if (interval && ((flags & ENVSYS_NFLAG) == 0 || sensors == NULL)) {
		(void)printf("\n");
		if (flags & ENVSYS_JFLAG)
			(void)printf("#-------\n");
	}

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
			    prop_string_value(desc),
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
		    	    prop_string_value(obj1),
		    	    sizeof(sensor->type));
		} else {
			free(sensor);
			continue;
		}

		/* index string */
		obj1  = prop_dictionary_get(obj, "index");
		if (obj1) {
			/* copy type */
			(void)strlcpy(sensor->index,
		    	    prop_string_value(obj1),
		    	    sizeof(sensor->index));
		} else {
			free(sensor);
			continue;
		}

		/* check sensor's state */
		state = prop_dictionary_get(obj, "state");

		/* mark sensors with invalid/unknown state */
		if ((prop_string_equals_string(state, "invalid") ||
		     prop_string_equals_string(state, "unknown")))
			sensor->invalid = true;

		/* get current drive state string */
		obj1 = prop_dictionary_get(obj, "drive-state");
		if (obj1) {
			(void)strlcpy(sensor->drvstate,
			    prop_string_value(obj1),
			    sizeof(sensor->drvstate));
		}

		/* get current battery capacity string */
		obj1 = prop_dictionary_get(obj, "battery-capacity");
		if (obj1) {
			(void)strlcpy(sensor->battcap,
			    prop_string_value(obj1),
			    sizeof(sensor->battcap));
		}

		/* get current value */
		obj1 = prop_dictionary_get(obj, "cur-value");
		if (obj1)
			sensor->cur_value = prop_number_signed_value(obj1);

		/* get max value */
		obj1 = prop_dictionary_get(obj, "max-value");
		if (obj1)
			sensor->max_value = prop_number_signed_value(obj1);

		/* get min value */
		obj1 = prop_dictionary_get(obj, "min-value");
		if (obj1)
			sensor->min_value = prop_number_signed_value(obj1);

		/* get percentage flag */
		obj1 = prop_dictionary_get(obj, "want-percentage");
		if (obj1)
			sensor->percentage = prop_bool_true(obj1);

		/* get critical max value if available */
		obj1 = prop_dictionary_get(obj, "critical-max");
		if (obj1)
			sensor->critmax_value = prop_number_signed_value(obj1);

		/* get maximum capacity value if available */
		obj1 = prop_dictionary_get(obj, "maximum-capacity");
		if (obj1)
			sensor->critmax_value = prop_number_signed_value(obj1);

		/* get critical min value if available */
		obj1 = prop_dictionary_get(obj, "critical-min");
		if (obj1)
			sensor->critmin_value = prop_number_signed_value(obj1);

		/* get critical capacity value if available */
		obj1 = prop_dictionary_get(obj, "critical-capacity");
		if (obj1)
			sensor->critmin_value = prop_number_signed_value(obj1);

		/* get warning max value if available */
		obj1 = prop_dictionary_get(obj, "warning-max");
		if (obj1)
			sensor->warnmax_value = prop_number_signed_value(obj1);

		/* get high capacity value if available */
		obj1 = prop_dictionary_get(obj, "high-capacity");
		if (obj1)
			sensor->warnmax_value = prop_number_signed_value(obj1);

		/* get warning min value if available */
		obj1 = prop_dictionary_get(obj, "warning-min");
		if (obj1)
			sensor->warnmin_value = prop_number_signed_value(obj1);

		/* get warning capacity value if available */
		obj1 = prop_dictionary_get(obj, "warning-capacity");
		if (obj1)
			sensor->warnmin_value = prop_number_signed_value(obj1);

		/* print sensor names if -l was given */
		if (flags & ENVSYS_LFLAG) {
			if (width)
				(void)printf("%*s\n", width,
				    prop_string_value(desc));
			else
				(void)printf("%s\n",
				    prop_string_value(desc));
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

/* When adding a new sensor type, be sure to address both
 * print_sensors() and print_sensors_json()
 */

static void
print_sensors(void)
{
	sensor_t sensor;
	sensor_stats_t stats = NULL;
	size_t maxlen = 0, ilen;
	double temp = 0;
	const char *invalid = "N/A", *degrees, *tmpstr, *stype;
	const char *a, *b, *c, *d, *e, *units;
	const char *sep;
	int flen;
	bool nflag = (flags & ENVSYS_NFLAG) != 0;
	char tbuf[26];
	bool tflag = (flags & ENVSYS_tFLAG) != 0;

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

	if (tflag) {
		if (ctime_r(&timestamp, tbuf) != NULL)
			(void)printf("%s\n",tbuf);
	}

	if (!nflag) {
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

		sep = ":";
		flen = 10;
	} else {
		sep = "";
		flen = 1;
	}

	/* print the sensors */
	SIMPLEQ_FOREACH(sensor, &sensors_list, entries) {
		/* skip sensors that were not marked as visible */
		if (sensors && !sensor->visible)
			continue;

		/* skip invalid sensors if -I is set */
		if ((flags & ENVSYS_IFLAG) && sensor->invalid)
			continue;

		/* print device name */
		if (!nflag && !mydevname) {
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

		if (!nflag) {
			/* print sensor description */
			(void)printf("%s%*.*s", mydevname ? "" : "  ",
			    (int)maxlen,
			    (int)maxlen, sensor->desc);
		}

		/* print invalid string */
		if (sensor->invalid) {
			(void)printf("%s%*s\n", sep, flen, invalid);
			continue;
		}

		/*
		 * Indicator and Battery charge sensors.
		 */
		if ((strcmp(sensor->type, "Indicator") == 0) ||
		    (strcmp(sensor->type, "Battery charge") == 0)) {

			(void)printf("%s%*s", sep, flen,
			     sensor->cur_value ? "TRUE" : "FALSE");

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
		(void)printf("%*.3f", (int)ilen, temp);	\
		ilen = 9;					\
	} else							\
		ilen += 9;					\
} while (0)

		/* temperatures */
		} else if (strcmp(sensor->type, "Temperature") == 0) {

			ilen = nflag ? 1 : 10;
			degrees = "";
			(void)printf("%s",sep);
			PRINTTEMP(sensor->cur_value);
			stype = degrees;

			if (statistics) {
				/* show statistics if flag set */
				PRINTTEMP(stats->max);
				PRINTTEMP(stats->min);
				PRINTTEMP(stats->avg);
				ilen += 2;
			} else if (!nflag) {
				PRINTTEMP(sensor->critmax_value);
				PRINTTEMP(sensor->warnmax_value);
				PRINTTEMP(sensor->warnmin_value);
				PRINTTEMP(sensor->critmin_value);
			}
			if (!nflag)
				(void)printf("%*s", (int)ilen - 3, stype);
#undef PRINTTEMP

		/* fans */
		} else if (strcmp(sensor->type, "Fan") == 0) {
			stype = "RPM";

			(void)printf("%s%*u", sep, flen, sensor->cur_value);

			ilen = 8;
			if (statistics) {
				/* show statistics if flag set */
				(void)printf(" %8u %8u %8u",
				    stats->max, stats->min, stats->avg);
				ilen += 2;
			} else if (!nflag) {
				if (sensor->critmax_value) {
					(void)printf(" %*u", (int)ilen,
					    sensor->critmax_value);
					ilen = 8;
				} else
					ilen += 9;

				if (sensor->warnmax_value) {
					(void)printf(" %*u", (int)ilen,
					    sensor->warnmax_value);
					ilen = 8;
				} else
					ilen += 9;

				if (sensor->warnmin_value) {
					(void)printf(" %*u", (int)ilen,
					    sensor->warnmin_value);
					ilen = 8;
				} else
					ilen += 9;

				if (sensor->critmin_value) {
					(void)printf( " %*u", (int)ilen,
					    sensor->critmin_value);
					ilen = 8;
				} else
					ilen += 9;

			}

			if (!nflag)
				(void)printf(" %*s", (int)ilen - 3, stype);

		/* integers */
		} else if (strcmp(sensor->type, "Integer") == 0) {

			stype = "none";

			(void)printf("%s%*d", sep, flen, sensor->cur_value);

			ilen = 8;

/* Print percentage of max_value */
#define PRINTPCT(a)							\
do {									\
	if (sensor->max_value) {					\
		(void)printf(" %*.3f%%", (int)ilen,			\
			((a) * 100.0) / sensor->max_value);		\
		ilen = 8;						\
	} else								\
		ilen += 9;						\
} while ( /* CONSTCOND*/ 0 )

/* Print an integer sensor value */
#define PRINTINT(a)							\
do {									\
	(void)printf(" %*u", (int)ilen, (a));				\
	ilen = 8;							\
} while ( /* CONSTCOND*/ 0 )

			if (statistics) {
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
			} else if (!nflag) {
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
			}

			if (!nflag)
				(void)printf("%*s", (int)ilen - 3, stype);

#undef PRINTINT
#undef PRINTPCT

		/* drives  */
		} else if (strcmp(sensor->type, "Drive") == 0) {

			(void)printf("%s%*s", sep, flen, sensor->drvstate);

		/* Battery capacity */
		} else if (strcmp(sensor->type, "Battery capacity") == 0) {

			(void)printf("%s%*s", sep, flen, sensor->battcap);

		/* Illuminance */
		} else if (strcmp(sensor->type, "Illuminance") == 0) {

			stype = "lux";

			(void)printf("%s%*u", sep, flen, sensor->cur_value);

			ilen = 8;
			if (statistics) {
				/* show statistics if flag set */
				(void)printf(" %8u %8u %8u",
				    stats->max, stats->min, stats->avg);
				ilen += 2;
			} else if (!nflag) {
				if (sensor->critmax_value) {
					(void)printf(" %*u", (int)ilen,
					    sensor->critmax_value);
					ilen = 8;
				} else
					ilen += 9;

				if (sensor->warnmax_value) {
					(void)printf(" %*u", (int)ilen,
					    sensor->warnmax_value);
					ilen = 8;
				} else
					ilen += 9;

				if (sensor->warnmin_value) {
					(void)printf(" %*u", (int)ilen,
					    sensor->warnmin_value);
					ilen = 8;
				} else
					ilen += 9;

				if (sensor->critmin_value) {
					(void)printf( " %*u", (int)ilen,
					    sensor->critmin_value);
					ilen = 8;
				} else
					ilen += 9;

			}

			if (!nflag)
				(void)printf(" %*s", (int)ilen - 3, stype);

		/* Pressure */
		} else if (strcmp(sensor->type, "pressure") == 0) {
			stype = "hPa";

			(void)printf("%s%*.3f", sep, flen,
			    sensor->cur_value / 10000.0);

			ilen = 8;
			if (statistics) {
				/* show statistics if flag set */
				(void)printf("  %.3f  %.3f  %.3f",
				    stats->max / 10000.0, stats->min / 10000.0, stats->avg / 10000.0);
				ilen += 2;
			} else if (!nflag) {
				if (sensor->critmax_value) {
					(void)printf(" %*u", (int)ilen,
					    sensor->critmax_value);
					ilen = 8;
				} else
					ilen += 9;

				if (sensor->warnmax_value) {
					(void)printf(" %*u", (int)ilen,
					    sensor->warnmax_value);
					ilen = 8;
				} else
					ilen += 9;

				if (sensor->warnmin_value) {
					(void)printf(" %*u", (int)ilen,
					    sensor->warnmin_value);
					ilen = 8;
				} else
					ilen += 9;

				if (sensor->critmin_value) {
					(void)printf( " %*u", (int)ilen,
					    sensor->critmin_value);
					ilen = 8;
				} else
					ilen += 9;

			}

			if (!nflag)
				(void)printf(" %*s", (int)ilen - 3, stype);

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
			else if (strcmp(sensor->type, "relative Humidity") == 0)
				stype = "%rH";
			else
				stype = "?";

			(void)printf("%s%*.3f", sep, flen,
			    sensor->cur_value / 1000000.0);

			ilen = 9;

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
		(void)printf("%*.3f", (int)ilen, (a) / 1000000.0);	\
		ilen = 9;						\
	} else								\
		ilen += 9;						\
} while ( /* CONSTCOND*/ 0 )

			if (statistics) {
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
			} else if (!nflag) {
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
			}
#undef PRINTPCT
#undef PRINTVAL

			if (!nflag) {
				(void)printf(" %*s", (int)ilen - 4, stype);
				if (sensor->percentage && sensor->max_value) {
					(void)printf(" (%5.2f%%)",
					    (sensor->cur_value * 100.0) /
					    sensor->max_value);
				}
			}
		}
		(void)printf("\n");
	}
}

static void
print_sensors_json(void)
{
	sensor_t sensor;
	sensor_stats_t stats = NULL;
	double temp = 0;

	const char *degrees, *tmpstr, *stype;
	const char *d, *e;
	mj_t envstatj;
	mj_t devices;
	mj_t sensors_per_dev;
	mj_t this_sensor;
	mj_t a_float;
	bool tflag = (flags & ENVSYS_tFLAG) != 0;
	char tbuf[26];
	char *output_s = NULL;
	size_t output_size = 0;

	tmpstr = stype = d = e = NULL;

	memset(&envstatj, 0x0, sizeof(envstatj));
	mj_create(&envstatj, "object");
	memset(&devices, 0x0, sizeof(devices));
	mj_create(&devices, "object");

	/* print the sensors */
	SIMPLEQ_FOREACH(sensor, &sensors_list, entries) {
		/* completely skip sensors that were not marked as visible */
		if (sensors && !sensor->visible)
			continue;

		/* completely skip invalid sensors if -I is set */
		if ((flags & ENVSYS_IFLAG) && sensor->invalid)
			continue;

		/* find out the statistics sensor */
		if (statistics) {
			stats = find_stats_sensor(sensor->desc);
			if (stats == NULL) {
				/* No statistics for this sensor, completely skip */
				continue;
			}
		}

		if (tmpstr == NULL || strcmp(tmpstr, sensor->dvname)) {
			if (tmpstr != NULL) {
				mj_append_field(&devices, tmpstr, "array", &sensors_per_dev);
				mj_delete(&sensors_per_dev);
			}

			memset(&sensors_per_dev, 0x0, sizeof(sensors_per_dev));
			mj_create(&sensors_per_dev, "array");

			tmpstr = sensor->dvname;
		}

		/* Any condition that that causes the sensor to be
		 * completely skipped should be checked before we are
		 * at this point */

		memset(&this_sensor, 0x0, sizeof(this_sensor));
		mj_create(&this_sensor, "object");

		mj_append_field(&this_sensor, "index", "string",
		    sensor->index, strlen(sensor->index));
		mj_append_field(&this_sensor, "description", "string",
		    sensor->desc, strlen(sensor->desc));
		mj_append_field(&this_sensor, "type", "string",
		    sensor->type, strlen(sensor->type));
		mj_append_field(&this_sensor, "invalid", "integer",
		    (int64_t)sensor->invalid);

		/* The sensor is invalid, semi-skip it */
		if (sensor->invalid) {
			goto around;
		}

		/*
		 * Indicator and Battery charge sensors.
		 */
		if ((strcmp(sensor->type, "Indicator") == 0) ||
		    (strcmp(sensor->type, "Battery charge") == 0)) {

			mj_append_field(&this_sensor, "cur-value", "integer",
			    (int64_t)sensor->cur_value);

/* convert and print a temp value in degC, degF, or Kelvin */
#define PRINTTEMP(a,b)						\
do {								\
    if (a) {							\
	temp = ((a) / 1000000.0);				\
	if (flags & ENVSYS_FFLAG) {				\
		temp = temp * (9.0 / 5.0) - 459.67;		\
		degrees = "degF";				\
	} else if (flags & ENVSYS_KFLAG) {			\
		degrees = "K";					\
	} else {						\
		temp = temp - 273.15;				\
		degrees = "degC";				\
	}							\
	memset(&a_float, 0x0, sizeof(a_float));			\
	mj_create(&a_float, "number", temp);			\
	mj_append_field(&this_sensor, b, "object", &a_float);	\
	mj_delete(&a_float);					\
    }								\
} while (0)

		/* temperatures */
		} else if (strcmp(sensor->type, "Temperature") == 0) {

			degrees = "";
			PRINTTEMP(sensor->cur_value, "cur-value");
			stype = degrees;

			if (statistics) {
				/* show statistics if flag set */
				PRINTTEMP(stats->max, "max");
				PRINTTEMP(stats->min, "min");
				PRINTTEMP(stats->avg, "avg");
			}
			
			PRINTTEMP(sensor->critmax_value, "critical-max");
			PRINTTEMP(sensor->warnmax_value, "warning-max");
			PRINTTEMP(sensor->warnmin_value, "warning-min");
			PRINTTEMP(sensor->critmin_value, "critical-min");

			mj_append_field(&this_sensor, "units", "string",
			    stype, strlen(stype));
#undef PRINTTEMP

		/* fans */
		} else if (strcmp(sensor->type, "Fan") == 0) {
			stype = "RPM";

			mj_append_field(&this_sensor, "cur-value", "integer",
			    (int64_t)sensor->cur_value);

			if (statistics) {
				/* show statistics if flag set */
				mj_append_field(&this_sensor, "max", "integer",
				    (int64_t)stats->max);
				mj_append_field(&this_sensor, "min", "integer",
				    (int64_t)stats->min);
				mj_append_field(&this_sensor, "avg", "integer",
				    (int64_t)stats->avg);
			}
			mj_append_field(&this_sensor, "critical-max", "integer",
			    (int64_t)sensor->critmax_value);
			mj_append_field(&this_sensor, "warning-max", "integer",
			    (int64_t)sensor->warnmax_value);
			mj_append_field(&this_sensor, "warning-min", "integer",
			    (int64_t)sensor->warnmin_value);
			mj_append_field(&this_sensor, "critical-min", "integer",
			    (int64_t)sensor->critmin_value);

			mj_append_field(&this_sensor, "units", "string",
			    stype, strlen(stype));

		/* integers */
		} else if (strcmp(sensor->type, "Integer") == 0) {

			stype = "none";

			mj_append_field(&this_sensor, "cur-value", "integer",
			    (int64_t)sensor->cur_value);

/* Print percentage of max_value */
#define PRINTPCT(a,b)							\
do {									\
	if ((a) && sensor->max_value) {					\
		mj_append_field(&this_sensor, b, "integer",		\
		    (int64_t)((a) * 100.0) / sensor->max_value);	\
	}								\
} while ( /* CONSTCOND*/ 0 )
			if (statistics) {
				if (sensor->percentage) {
					PRINTPCT(stats->max, "max");
					PRINTPCT(stats->min, "min");
					PRINTPCT(stats->avg, "avg");
				} else {
					mj_append_field(&this_sensor, "max", "integer",
					    (int64_t)stats->max);
					mj_append_field(&this_sensor, "min", "integer",
					    (int64_t)stats->min);
					mj_append_field(&this_sensor, "avg", "integer",
					    (int64_t)stats->avg);
				}
			}
			if (sensor->percentage) {
				PRINTPCT(sensor->critmax_value, "critical-max");
				PRINTPCT(sensor->warnmax_value, "warning-max");
				PRINTPCT(sensor->warnmin_value, "warning-min");
				PRINTPCT(sensor->critmin_value, "critical-min");
			} else {
				mj_append_field(&this_sensor, "critmax_value", "integer",
				    (int64_t)sensor->critmax_value);
				mj_append_field(&this_sensor, "warning-max", "integer",
				    (int64_t)sensor->warnmax_value);
				mj_append_field(&this_sensor, "warning-min", "integer",
				    (int64_t)sensor->warnmin_value);
				mj_append_field(&this_sensor, "critical-min", "integer",
				    (int64_t)sensor->critmin_value);
			}

			mj_append_field(&this_sensor, "units", "string",
			    stype, strlen(stype));
		/* drives  */
		} else if (strcmp(sensor->type, "Drive") == 0) {
			mj_append_field(&this_sensor, "drvstate", "string",
			    sensor->drvstate, strlen(sensor->drvstate));
		/* Battery capacity */
		} else if (strcmp(sensor->type, "Battery capacity") == 0) {
			mj_append_field(&this_sensor, "battcap", "string",
			    sensor->battcap, strlen(sensor->battcap));
		/* Illuminance */
		} else if (strcmp(sensor->type, "Illuminance") == 0) {

			stype = "lux";

			mj_append_field(&this_sensor, "cur-value", "integer",
			    (int64_t)sensor->cur_value);

			if (statistics) {
				/* show statistics if flag set */
				mj_append_field(&this_sensor, "max", "integer",
				    (int64_t)stats->max);
				mj_append_field(&this_sensor, "min", "integer",
				    (int64_t)stats->min);
				mj_append_field(&this_sensor, "avg", "integer",
				    (int64_t)stats->avg);
			}

			mj_append_field(&this_sensor, "critmax_value", "integer",
			    (int64_t)sensor->critmax_value);
			mj_append_field(&this_sensor, "warning-max", "integer",
			    (int64_t)sensor->warnmax_value);
			mj_append_field(&this_sensor, "warning-min", "integer",
			    (int64_t)sensor->warnmin_value);
			mj_append_field(&this_sensor, "critical-min", "integer",
			    (int64_t)sensor->critmin_value);

			mj_append_field(&this_sensor, "units", "string",
			    stype, strlen(stype));
		/* Pressure */
		} else if (strcmp(sensor->type, "pressure") == 0) {
#define PRINTFLOAT(a,b)						\
do {								\
    if (a) {							\
	memset(&a_float, 0x0, sizeof(a_float));			\
	mj_create(&a_float, "number", a);			\
	mj_append_field(&this_sensor, b, "object", &a_float);	\
	mj_delete(&a_float);					\
    }								\
} while (0)
			stype = "hPa";

			PRINTFLOAT(sensor->cur_value / 10000.0, "cur-value");

			if (statistics) {
				/* show statistics if flag set */
				PRINTFLOAT(stats->max / 10000.0, "max");
				PRINTFLOAT(stats->min / 10000.0, "min");
				PRINTFLOAT(stats->avg / 10000.0, "avg");
			}

			PRINTFLOAT(sensor->critmax_value, "critmax_value");
			PRINTFLOAT(sensor->warnmax_value, "warning-max");
			PRINTFLOAT(sensor->warnmin_value, "warning-min");
			PRINTFLOAT(sensor->critmin_value, "critical-min");

			mj_append_field(&this_sensor, "units", "string",
			    stype, strlen(stype));
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
			else if (strcmp(sensor->type, "relative Humidity") == 0)
				stype = "%rH";
			else
				stype = "?";

			PRINTFLOAT(sensor->cur_value / 1000000.0, "cur-value");

			mj_append_field(&this_sensor, "units", "string",
			    stype, strlen(stype));
#undef PRINTFLOAT

/* Print a generic sensor value */
#define PRINTVAL(a,b)							\
do {									\
	if ((a)) {							\
		memset(&a_float, 0x0, sizeof(a_float));			\
		mj_create(&a_float, "number", a / 1000000.0);		\
		mj_append_field(&this_sensor, b, "object", &a_float);	\
		mj_delete(&a_float);					\
	}								\
} while ( /* CONSTCOND*/ 0 )

			if (statistics) {
				if (sensor->percentage) {
					PRINTPCT(stats->max, "max");
					PRINTPCT(stats->min, "min");
					PRINTPCT(stats->avg, "avg");
				} else {
					PRINTVAL(stats->max, "max");
					PRINTVAL(stats->min, "min");
					PRINTVAL(stats->avg, "avg");
				}
			}
			if (sensor->percentage) {
				PRINTPCT(sensor->critmax_value, "critmax_value");
				PRINTPCT(sensor->warnmax_value, "warning-max");
				PRINTPCT(sensor->warnmin_value, "warning-min");
				PRINTPCT(sensor->critmin_value, "critical-min");
			} else {

				PRINTVAL(sensor->critmax_value, "critmax_value");
				PRINTVAL(sensor->warnmax_value, "warning-max");
				PRINTVAL(sensor->warnmin_value, "warning-min");
				PRINTVAL(sensor->critmin_value, "critical-min");
			}
#undef PRINTPCT
#undef PRINTVAL

			mj_append_field(&this_sensor, "units", "string",
			    stype, strlen(stype));
			if (sensor->percentage && sensor->max_value) {
				memset(&a_float, 0x0, sizeof(a_float));
				mj_create(&a_float, "number",
				    (sensor->cur_value * 100.0) / sensor->max_value);
				mj_append_field(&this_sensor, "pct", "object",
				    &a_float);
				mj_delete(&a_float);
			}
		}
around:
		mj_append(&sensors_per_dev, "object", &this_sensor);
		mj_delete(&this_sensor);
	}

	if (tmpstr != NULL) {
		mj_append_field(&devices, tmpstr, "array", &sensors_per_dev);
		mj_delete(&sensors_per_dev);
	
		mj_append_field(&envstatj, "devices", "object", &devices);
		if (tflag) {
			mj_append_field(&envstatj, "timestamp", "integer", (int64_t)timestamp);
			if (ctime_r(&timestamp, tbuf) != NULL)
				/* Pull off the newline */
				mj_append_field(&envstatj, "human_timestamp", "string", tbuf, strlen(tbuf) - 1);
		}

#ifdef __NOT
		mj_asprint(&output_s, &envstatj, MJ_JSON_ENCODE);
		printf("%s", output_s);
		if (output_s != NULL)
			free(output_s);
#endif
		/* Work around lib/59230 for now.  Way over allocate
		 * the output buffer.  Summary of the problem:
		 * mj_string_size does not appear to allocate enough
		 * space if there are arrays present in the serialized
		 * superatom.  It appears that for every array that is
		 * placed inside of another object (or probably array)
		 * the resulting buffer is a single character too
		 * small.  So just do what mj_asprint() does, except
		 * make the size much larger than is needed.
		 */

		output_size = mj_string_size(&envstatj) * 3;
		output_s = calloc(1, output_size);
		if (output_s != NULL) {
			mj_snprint(output_s, output_size, &envstatj, MJ_JSON_ENCODE);
			printf("%s", output_s);
			free(output_s);
		}
	}

	mj_delete(&devices);
	mj_delete(&envstatj);
}

static int
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-DfIjklnrSTt] ", getprogname());
	(void)fprintf(stderr, "[-c file] [-d device] [-i interval] ");
	(void)fprintf(stderr, "[-s device:sensor,...] [-w width]\n");
	(void)fprintf(stderr, "       %s ", getprogname());
	(void)fprintf(stderr, "[-d device] ");
	(void)fprintf(stderr, "[-s device:sensor,...] ");
	(void)fprintf(stderr, "-x [property]\n");
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}
