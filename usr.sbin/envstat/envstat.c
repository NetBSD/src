/* $NetBSD: envstat.c,v 1.33 2007/07/12 22:52:54 xtraeme Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juan Romero Pardines.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Juan Romero Pardines
 *      for the NetBSD Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * TODO:
 *
 * 	o Some checks should be added to ensure that the user does not
 * 	  set unwanted values for the critical limits.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <prop/proplib.h>
#include <sys/envsys.h>

#define _PATH_DEV_SYSMON	"/dev/sysmon"

#define ENVSYS_DFLAG	0x00000001	/* list registered devices */
#define ENVSYS_FFLAG	0x00000002	/* show temp in farenheit */
#define ENVSYS_LFLAG	0x00000004	/* list sensors */
#define ENVSYS_XFLAG	0x00000008	/* externalize dictionary */

/*
 * Operation flags for -m.
 */
#define USERF_SCRITICAL 0x00000001	/* set a critical limit */
#define USERF_RCRITICAL 0x00000002	/* remove a critical limit */
#define USERF_SCRITMAX	0x00000004	/* set a critical max limit */
#define USERF_RCRITMAX	0x00000008	/* remove a critical max limit */
#define USERF_SCRITMIN	0x00000010	/* set a critical min limit */
#define USERF_RCRITMIN	0x00000020	/* remove a critical min limit */
#define USERF_SRFACT	0x00000040	/* set a new rfact */
#define USERF_SDESCR	0x00000080	/* set a new description */

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
};

static int interval, flags, width;
static char *mydevname, *sensors, *userreq;
static struct envsys_sensor *gesen;
static size_t gnelems, newsize;

static int parse_dictionary(int);
static int send_dictionary(int);
static int find_sensors(prop_array_t);
static void print_sensors(struct envsys_sensor *, size_t);
static int check_sensors(struct envsys_sensor *, char *, size_t);
static int usage(void);


int main(int argc, char **argv)
{
	prop_dictionary_t dict;
	int c, fd, rval;
	char *endptr;

	rval = flags = interval = width = 0;
	newsize = gnelems = 0;
	gesen = NULL;

	setprogname(argv[0]);

	while ((c = getopt(argc, argv, "Dd:fi:lm:s:w:x")) != -1) {
		switch (c) {
		case 'd':	/* show sensors of a specific device */
			mydevname = strdup(optarg);
			if (mydevname == NULL)
				err(ENOMEM, "out of memory");
			break;
		case 'i':	/* wait time between intervals */
			interval = strtoul(optarg, &endptr, 10);
			if (*endptr != '\0')
				errx(1, "interval must be an integer");
			break;
		case 'D':	/* list registered devices */
			flags |= ENVSYS_DFLAG;
			break;
		case 'f':	/* display temperature in Farenheit */
			flags |= ENVSYS_FFLAG;
			break;
		case 'l':	/* list sensors */
			flags |= ENVSYS_LFLAG;
			break;
		case 'w':	/* width value for the lines */
			width = strtoul(optarg, &endptr, 10);
			if (*endptr != '\0')
				errx(1, "width must be an integer");
			break;
		case 'x':	/* print the dictionary in raw format */
			flags |= ENVSYS_XFLAG;
			break;
		case 's':	/* only show specified sensors */
			sensors = strdup(optarg);
			if (sensors == NULL)
				err(ENOMEM, "out of memory");
			break;
		case 'm':
			userreq = strdup(optarg);
			if (userreq == NULL)
				err(ENOMEM, "out of memory");
			break;
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	}

	if ((fd = open(_PATH_DEV_SYSMON, O_RDONLY)) == -1)
		err(EXIT_FAILURE, "open");

	if (!interval && (flags & ENVSYS_XFLAG)) {
		if (prop_dictionary_recv_ioctl(fd,
		    			       ENVSYS_GETDICTIONARY,
					       &dict)) {
			(void)close(fd);
			err(EINVAL, "recv_ioctl");
		}
	}

	if (argc == 1) {
		rval = parse_dictionary(fd);

	} else if (userreq) {
		if (!sensors || !mydevname) {
			(void)fprintf(stderr, "%s: -m cannot be used without "
			    "-s and -d\n", getprogname());
			return EINVAL;
		}
			
		rval = send_dictionary(fd);
		goto out;

	} else if (interval) {
		for (;;) {
			rval = parse_dictionary(fd);
			if (rval)
				goto out;
			(void)fflush(stdout);
			(void)sleep(interval);
		}

	} else if (!interval) {
		if (flags & ENVSYS_XFLAG)
			(void)printf("%s", prop_dictionary_externalize(dict));
		else
			rval = parse_dictionary(fd);

	} else
		usage();

out:
	if (sensors)
		free(sensors);
	if (userreq)
		free(userreq);
	if (mydevname)
		free(mydevname);
	if (gesen)
		free(gesen);
	(void)close(fd);
	return rval;
}

static int
send_dictionary(int fd)
{
	prop_dictionary_t dict, udict;
	prop_object_t obj;
	char *buf, *target, *endptr;
	int error, i, uflag;
	double val;

	error = uflag = val = 0;

	/* 
	 * part 1: kernel dictionary.
	 *
	 * This parts consists in parsing the kernel dictionary
	 * to check for unknown device or sensor and we must
	 * know what type of sensor are we trying to set
	 * a critical condition.
	 */
	if (prop_dictionary_recv_ioctl(fd, ENVSYS_GETDICTIONARY, &dict))
		return EINVAL;

	if (mydevname) {
		obj = prop_dictionary_get(dict, mydevname);
		if (prop_object_type(obj) != PROP_TYPE_ARRAY) {
			warnx("unknown device `%s'", mydevname);
			prop_object_release(dict);
			return EINVAL;
		}

		if (find_sensors(obj)) {
			prop_object_release(dict);
			return EINVAL;
		}
	}

	/* find the type for selected sensor */
	for (i = 0; i < gnelems; i++)
		if (strcmp(sensors, gesen[i].desc) == 0)
			break;

	/* we know the type of the sensor now, release kernel dict */
	prop_object_release(dict);

	/* 
	 * part 2: userland dictionary.
	 *
	 * This parts consists in setting the values provided
	 * by the user and convert when necesssary to send
	 * them to the kernel again.
	 */
	udict = prop_dictionary_create();

	/* create the driver-name object */
	obj = prop_string_create_cstring_nocopy(mydevname);
	if (obj == NULL || !prop_dictionary_set(udict, "driver-name", obj)) {
		error = EINVAL;
		goto out;
	}

	prop_object_release(obj);

	/* create the sensor-name object */
	obj = prop_string_create_cstring_nocopy(sensors);
	if (obj == NULL || !prop_dictionary_set(udict, "sensor-name", obj)) {
		error = EINVAL;
		goto out;
	}

	prop_object_release(obj);
	/* 
	 * parse the -m argument; we understand the following ways:
	 *
	 * 	-m critical/crit{max,min}=value
	 * 	-m critical/crit{max,min}=remove
	 * 	-m desc="BLAH"
	 * 	-m rfact=value
	 */
	if (userreq) {
		buf = strtok(userreq, "=");
		target = strdup(buf);
		if (target == NULL) {
			error = ENOMEM;
			goto out;
		}

		while (buf != NULL) {
			/* 
			 * skip current string if it's the same
			 * than target requested.
			 */
			if (strcmp(target, buf) == 0)
				buf = strtok(NULL, "=");

			/* check what target was requested */
			if (strcmp(target, "desc") == 0) {
				uflag |= USERF_SDESCR;
				obj = prop_string_create_cstring_nocopy(buf);
				break;
#define SETNCHECKVAL(a, b)						\
do {									\
	if (strcmp(buf, "remove") == 0)					\
		uflag |= (a);						\
	else {								\
		uflag |= (b);						\
		val = strtod(buf, &endptr);				\
		if (*endptr != '\0') {					\
			(void)printf("%s: invalid value\n",		\
			    getprogname());				\
			error = EINVAL;					\
			goto out;					\
		}							\
	}								\
} while (/* CONSTCOND */ 0)

			} else if (strcmp(target, "critical") == 0) {
				SETNCHECKVAL(USERF_RCRITICAL, USERF_SCRITICAL);
				break;
			} else if (strcmp(target, "critmax") == 0) {
				SETNCHECKVAL(USERF_RCRITMAX, USERF_SCRITMAX);
				break;
			} else if (strcmp(target, "critmin") == 0) {
				SETNCHECKVAL(USERF_RCRITMIN, USERF_SCRITMIN);
				break;
			} else if (strcmp(target, "rfact") == 0) {
				uflag |= USERF_SRFACT;
				val = strtod(buf, &endptr);
				if (*endptr != '\0') {
					(void)printf("%s: invalid value\n",
					    getprogname());
					error = EINVAL;
					goto out;
				}
				break;
			} else {
				(void)printf("%s: invalid target\n",
				    getprogname());
				error =  EINVAL;
				goto out;
			}
		}
		free(target);
	}

	/* critical capacity for percentage sensors */
	if (uflag & USERF_SCRITICAL) {
		/* sanity check */
		if (val < 0 || val > 100) {
			(void)printf("%s: invalid value (0><100)\n",
			    getprogname());
			error = EINVAL;
			goto out;
		}

		/* ok... convert the value */
		val = (val / 100) * gesen[i].max_value;
		obj = prop_number_create_unsigned_integer(val);
	}

	/*
	 * conversions required to send a proper value to the kernel.
	 */
	if ((uflag & USERF_SCRITMAX) || (uflag & USERF_SCRITMIN)) {
		/* temperatures */
		if (strcmp(gesen[i].type, "Temperature") == 0) {
			/* convert from farenheit to celsius */
			if (flags & ENVSYS_FFLAG)
				val = (val - 32.0) * (5.0 / 9.0);

			/* convert to microKelvin */
			val = val * 1000000 + 273150000;
			/* printf("val=%d\n", (int)val); */
			obj = prop_number_create_unsigned_integer(val);
		/* fans */
		} else if (strcmp(gesen[i].type, "Fan") == 0) {
			if (val < 0 || val > 10000) {
				error = EINVAL;
				goto out;
			}
			/* printf("val=%d\n", (int)val); */
			obj = prop_number_create_unsigned_integer(val);

		/* volts, watts, ohms, etc */
		} else {
			/* convert to m[V,W,Ohms] again */
			val *= 1000000.0;
			/* printf("val=%5.0f\n", val); */
			obj = prop_number_create_integer(val);
		}
	}

	/* user wanted to set a new description */	
	if (uflag & USERF_SDESCR) {
		if (!prop_dictionary_set(udict, "new-description", obj)) {
			error = EINVAL;
			goto out;
		}

	/* user wanted to set a new critical capacity */
	} else if (uflag & USERF_SCRITICAL) {
		if (!prop_dictionary_set(udict, "critical-capacity", obj)) {
			error = EINVAL;
			goto out;
		}

	} else if (uflag & USERF_RCRITICAL) {
		obj = prop_bool_create(1);
		if (!prop_dictionary_set(udict, "remove-critical-cap", obj)) {
			error = EINVAL;
			goto out;
		}

	/* user wanted to remove a critical min limit */
	} else if (uflag & USERF_RCRITMIN) {
		obj = prop_bool_create(1);
		if (!prop_dictionary_set(udict, "remove-cmin-limit", obj)) {
			error = EINVAL;
			goto out;
		}

	/* user wanted to remove a critical max limit */
	} else if (uflag & USERF_RCRITMAX) {
		obj = prop_bool_create(1);
		if (!prop_dictionary_set(udict, "remove-cmax-limit", obj)) {
			error = EINVAL;
			goto out;
		}

	/* user wanted to set a new critical min value */
	} else if (uflag & USERF_SCRITMIN) {
		if (!prop_dictionary_set(udict, "critical-min-limit", obj)) {
			error = EINVAL;
			goto out;
		}

	/* user wanted to set a new critical max value */
	} else if (uflag & USERF_SCRITMAX) {
		if (!prop_dictionary_set(udict, "critical-max-limit", obj)) {
			error = EINVAL;
			goto out;
		}

	/* user wanted to set a new rfact */
	} else if (uflag & USERF_SRFACT) {
		obj = prop_number_create_integer(val);
		if (!prop_dictionary_set(udict, "new-rfact", obj)) {
			error = EINVAL;
			goto out;
		}

	} else {
		(void)printf("%s: unknown operation\n", getprogname());
		error = EINVAL;
		goto out;
	}

	prop_object_release(obj);

#if 0
	printf("%s", prop_dictionary_externalize(udict));
	return error;
#endif

	/* all done? send our dictionary now */
	error = prop_dictionary_send_ioctl(udict, fd, ENVSYS_SETDICTIONARY);

	if (error)
		(void)printf("%s: %s\n", getprogname(), strerror(error));
out:
	prop_object_release(udict);
	return error;
}

static int
parse_dictionary(int fd)
{
	prop_array_t array;
	prop_dictionary_t dict;
	prop_object_iterator_t iter;
	prop_object_t obj;
	const char *dnp = NULL;
	int rval = 0;

	/* receive dictionary from kernel */
	if (prop_dictionary_recv_ioctl(fd, ENVSYS_GETDICTIONARY, &dict))
		return EINVAL;

	if (mydevname) {
		obj = prop_dictionary_get(dict, mydevname);
		if (prop_object_type(obj) != PROP_TYPE_ARRAY) {
			warnx("unknown device `%s'", mydevname);
			rval = EINVAL;
			goto out;
		}

		rval = find_sensors(obj);
		if (rval)
			goto out;
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

			dnp = prop_dictionary_keysym_cstring_nocopy(obj);

			if (flags & ENVSYS_DFLAG) {
				(void)printf("%s\n", dnp);
			} else {
				rval = find_sensors(array);
				if (rval)
					goto out;
			}
		}

		prop_object_iterator_release(iter);
	}

	if (userreq == NULL)
		if ((flags & ENVSYS_LFLAG) == 0)
			print_sensors(gesen, gnelems);

	if (interval)
		(void)printf("\n");

out:
	if (gesen) {
		free(gesen);
		gesen = NULL;
		gnelems = 0;
		newsize = 0;
	}
	prop_object_release(dict);
	return rval;
}

static int
find_sensors(prop_array_t array)
{
	prop_object_iterator_t iter;
	prop_object_t obj, obj1;
	prop_string_t state, desc = NULL;
	struct envsys_sensor *esen = NULL;
	int rval = 0;
	size_t oldsize;
	char *str = NULL;

	oldsize = newsize;
	newsize += prop_array_count(array) * sizeof(*gesen);
	esen = realloc(gesen, newsize);
	if (esen == NULL) {
		if (gesen)
			free(gesen);
		gesen = NULL;
		rval = ENOMEM;
		goto out;
	}
	gesen = esen;

	iter = prop_array_iterator(array);
	if (iter == NULL) {
		rval = EINVAL;
		goto out;
	}

	/* iterate over the array of dictionaries */
	while ((obj = prop_object_iterator_next(iter)) != NULL) {
		
		gesen[gnelems].visible = false;

		/* check sensor's state */
		state = prop_dictionary_get(obj, "state");

		/* mark invalid sensors */
		if (prop_string_equals_cstring(state, "invalid"))
			gesen[gnelems].invalid = true;
		else
			gesen[gnelems].invalid = false;

		/* description string */
		desc = prop_dictionary_get(obj, "description");
		/* copy description */
		(void)strlcpy(gesen[gnelems].desc,
		    prop_string_cstring_nocopy(desc),
		    sizeof(gesen[gnelems].desc));

		/* type string */
		obj1  = prop_dictionary_get(obj, "type");
		/* copy type */
		(void)strlcpy(gesen[gnelems].type,
		    prop_string_cstring_nocopy(obj1),
		    sizeof(gesen[gnelems].type));

		/* get current drive state string */
		obj1 = prop_dictionary_get(obj, "drive-state");
		if (obj1 != NULL)
			(void)strlcpy(gesen[gnelems].drvstate,
			    prop_string_cstring_nocopy(obj1),
			    sizeof(gesen[gnelems].drvstate));

		/* get current value */
		obj1 = prop_dictionary_get(obj, "cur-value");
		gesen[gnelems].cur_value = prop_number_integer_value(obj1);

		/* get max value */
		obj1 = prop_dictionary_get(obj, "max-value");
		if (obj1 != NULL)
			gesen[gnelems].max_value =
			    prop_number_integer_value(obj1);
		else
			gesen[gnelems].max_value = 0;

		/* get min value */
		obj1 = prop_dictionary_get(obj, "min-value");
		if (obj1 != NULL)
			gesen[gnelems].min_value =
			    prop_number_integer_value(obj1);
		else
			gesen[gnelems].min_value = 0;

		/* get avg value */
		obj1 = prop_dictionary_get(obj, "avg-value");
		if (obj1 != NULL)
			gesen[gnelems].avg_value =
			    prop_number_integer_value(obj1);
		else
			gesen[gnelems].avg_value = 0;

		/* get percentage flag */
		obj1 = prop_dictionary_get(obj, "want-percentage");
		if (obj1 != NULL)
			gesen[gnelems].percentage = prop_bool_true(obj1);

		/* get critical max value if available */
		obj1 = prop_dictionary_get(obj, "critical-max-limit");
		if (obj1 != NULL) {
			gesen[gnelems].critmax_value =
			    prop_number_integer_value(obj1);
		} else
			gesen[gnelems].critmax_value = 0;

		/* get critical min value if available */
		obj1 = prop_dictionary_get(obj, "critical-min-limit");
		if (obj1 != NULL) {
			gesen[gnelems].critmin_value =
			    prop_number_integer_value(obj1);
		} else
			gesen[gnelems].critmin_value = 0;

		/* get critical capacity value if available */
		obj1 = prop_dictionary_get(obj, "critical-capacity");
		if (obj1 != NULL) {
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
		if (str == NULL)
			return ENOMEM;

		rval = check_sensors(gesen, str, gnelems);
		if (rval)
			goto out;
	}

out:
	if (str)
		free(str);
	return rval;
}

static int
check_sensors(struct envsys_sensor *es, char *str, size_t nelems)
{
	int i;
	char *sname;

	sname = strtok(str, ",");
	while (sname != NULL) {
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
			} else {
				warnx("unknown sensor `%s'", sname);
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
print_sensors(struct envsys_sensor *es, size_t nelems)
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

		/* skip sensors that were not marked as visible */
		if (sensors && !es[i].visible)
			continue;

		(void)printf("%*.*s", (int)maxlen, (int)maxlen, es[i].desc);

		if (es[i].invalid) {
			(void)printf(": %10s\n", invalid);
			continue;
		}

		if (strcmp(es[i].type, "Indicator") == 0) {

			(void)printf(": %10s", es[i].cur_value ? "ON" : "OFF");

/* converts the value to degC or degF */
#define CONVERTTEMP(a, b, c)					\
do {								\
	(a) = ((b) / 1000000.0) - 273.15;			\
	if (flags & ENVSYS_FFLAG) {				\
		(a) = (9.0 / 5.0) * (a) + 32.0;			\
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

		/* drives */
		} else if (strcmp(es[i].type, "Drive") == 0) {

			(void)printf(": %10s", es[i].drvstate);

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
	(void)fprintf(stderr, "Usage: %s [-Dflx] ", getprogname());
	(void)fprintf(stderr, "[-m ...] [-s s1,s2 ] [-w num] ");
	(void)fprintf(stderr, "[-i num] [-d ...]\n");
	exit(EXIT_FAILURE);
	/* NOTREACHED */
}
