/* 	$NetBSD: config.c,v 1.3 2007/10/07 16:22:37 xtraeme Exp $	*/

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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: config.c,v 1.3 2007/10/07 16:22:37 xtraeme Exp $");
#endif /* not lint */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <sys/queue.h>
#include <prop/proplib.h>

#include "envstat.h"

/*
 * Singly linked list for dictionaries that store properties
 * in a sensor.
 */
static SLIST_HEAD(, sensor_block) sensor_block_list =
    SLIST_HEAD_INITIALIZER(&sensor_block_list);

/*
 * Singly linked list for devices that store a proplib array
 * device with a device name.
 */
static SLIST_HEAD(, device_block) device_block_list =
    SLIST_HEAD_INITIALIZER(&device_block_list);

enum {
	VALUE_ERR,
	PROP_ERR,
	SENSOR_ERR,
	DEV_ERR
};

static prop_dictionary_t cfdict, sensordict;
static void config_errmsg(int, const char *, const char *);

static void
config_errmsg(int lvl, const char *key, const char *key2)
{
	(void)printf("error: ");

	switch (lvl) {
	case VALUE_ERR:
		(void)printf("invalid value for '%s' in `%s'\n",
		    key, key2);
		break;
	case PROP_ERR:
		(void)printf("the '%s' property is not allowed "
		    "in `%s'\n", key, key2);
		break;
	case SENSOR_ERR:
		(void)printf("'%s' is not a valid sensor in the "
		   "`%s' device\n", key, key2);
		break;
	case DEV_ERR:
		(void)printf("device `%s' doesn't exist\n", key);
		break;
	}

	(void)printf("please fix the configuration file!\n");
	exit(EXIT_FAILURE);
}

/*
 * Adds a property into the global dictionary 'cfdict'.
 */
void
config_dict_add_prop(const char *key, char *value)
{
	if (!key || !value)
		return;

	if (!sensordict) {
		sensordict = prop_dictionary_create();
		if (!sensordict)
			err(EXIT_FAILURE, "cfdict");
	}

	if (!prop_dictionary_set_cstring(sensordict, key, value))
		err(EXIT_FAILURE, "prop_dict_set_cstring");
}

/*
 * Adds the last property into the dictionary and puts it into
 * the singly linked list for future use.
 */
void
config_dict_mark(const char *key)
{
	struct sensor_block *sb;

	if (!key)
		err(EXIT_FAILURE, "!key");

	sb = calloc(1, sizeof(*sb));
	if (!sb)
		err(EXIT_FAILURE, "!sb");

	sb->dict = prop_dictionary_create();
	if (!sb->dict)
		err(EXIT_FAILURE, "!sb->dict");

	sb->dict = prop_dictionary_copy(sensordict);
	SLIST_INSERT_HEAD(&sensor_block_list, sb, sb_head);
	config_dict_destroy(sensordict);
}

/*
 * Only used for debugging purposses.
 */
void
config_dict_dump(prop_dictionary_t d)
{
	char *buf;

	buf = prop_dictionary_externalize(d);
	(void)printf("%s", buf);
	free(buf);
}

/*
 * Returns the global dictionary.
 */
prop_dictionary_t
config_dict_parsed(void)
{
	return cfdict;
}

/*
 * Destroys all objects from a dictionary.
 */
void
config_dict_destroy(prop_dictionary_t d)
{
	prop_object_iterator_t iter;
	prop_object_t obj;

	iter = prop_dictionary_iterator(d);
	if (!iter)
		err(EXIT_FAILURE, "!iter");

	 while ((obj = prop_object_iterator_next(iter)) != NULL) {
		 prop_dictionary_remove(d,
		     prop_dictionary_keysym_cstring_nocopy(obj));
		 prop_object_iterator_reset(iter);
	 }

	 prop_object_iterator_release(iter);
}

/*
 * Parses all properties on the device and adds the device
 * into the singly linked list for devices and the global dictionary.
 */
void
config_devblock_add(const char *key, prop_dictionary_t kdict)
{
	struct device_block *db;
	struct sensor_block *sb;
	prop_array_t array;
	prop_object_iterator_t iter;
	prop_dictionary_t sdict;
	prop_object_t obj;
	prop_string_t lindex;
	const char *sensor;
	bool sensor_found = false;

	if (!key)
		err(EXIT_FAILURE, "devblock !key");

	array = prop_dictionary_get(kdict, key);
	if (!array)
		config_errmsg(DEV_ERR, key, NULL);

	SLIST_FOREACH(sb, &sensor_block_list, sb_head) {
		/* get the index object value from configuration */
		lindex = prop_dictionary_get(sb->dict, "index");
		sensor = prop_string_cstring_nocopy(lindex);

		iter = prop_array_iterator(array);
		if (!iter)
			err(EXIT_FAILURE, "prop_array_iterator devblock");

		/*
		 * Get the correct sensor's dictionary from kernel's
		 * dictionary.
		 */
		while ((sdict = prop_object_iterator_next(iter)) != NULL) {
			obj = prop_dictionary_get(sdict, "index");
			if (prop_string_equals(lindex, obj)) {
				sensor_found = true;
				break;
			}
		}

		if (!sensor_found) {
			prop_object_iterator_release(iter);
			config_errmsg(SENSOR_ERR, sensor, key);
		}

		config_devblock_check_sensorprops(sdict, sb->dict, sensor);
		prop_object_iterator_release(iter);
	}

	db = calloc(1, sizeof(*db));
	if (!db)
		err(EXIT_FAILURE, "calloc db");

	db->array = prop_array_create();
	if (!db->array)
		err(EXIT_FAILURE, "prop_array_create devblock");

	/*
	 * Add all dictionaries into the array.
	 */
	SLIST_FOREACH(sb, &sensor_block_list, sb_head)
		if (!prop_array_add(db->array, sb->dict))
			err(EXIT_FAILURE, "prop_array_add");

	/*
	 * Add this device block into our list.
	 */
	db->dev_key = strdup(key);
	SLIST_INSERT_HEAD(&device_block_list, db, db_head);

	/*
	 * Remove all items in the list, but just decrement
	 * the refcnt in the dictionaries... they are in use.
	 */
	while (!SLIST_EMPTY(&sensor_block_list)) {
		sb = SLIST_FIRST(&sensor_block_list);
		SLIST_REMOVE_HEAD(&sensor_block_list, sb_head);
		prop_object_release(sb->dict);
		free(sb);
	}

	/*
	 * Now the properties on the array has been parsed,
	 * add it into the global dict.
	 */
	if (!cfdict) {
		cfdict = prop_dictionary_create();
		if (!cfdict)
			err(EXIT_FAILURE, "prop_dictionary_create cfdict");
	}

	if (!prop_dictionary_set(cfdict, key, db->array))
		err(EXIT_FAILURE, "prop_dictionary_set db->array");

}

/*
 * Returns the dictionary that has 'sensor_key' in the 'dvname'
 * array.
 */
prop_dictionary_t
config_devblock_getdict(const char *dvname, const char *sensor_key)
{
	struct device_block *db;
	prop_object_iterator_t iter;
	prop_object_t obj, obj2;

	if (!dvname || !sensor_key)
		return NULL;

	SLIST_FOREACH(db, &device_block_list, db_head)
		if (strcmp(db->dev_key, dvname) == 0)
			break;

	if (!db)
		return NULL;

	iter = prop_array_iterator(db->array);
	if (!iter)
		return NULL;

	while ((obj = prop_object_iterator_next(iter)) != NULL) {
		obj2 = prop_dictionary_get(obj, "index");
		if (prop_string_equals_cstring(obj2, sensor_key))
			break;
	}

	prop_object_iterator_release(iter);
	return obj;
}

/*
 * Checks that all properties specified in the configuration file
 * are valid and updates the objects with proper values.
 */
void
config_devblock_check_sensorprops(prop_dictionary_t ksdict,
				  prop_dictionary_t csdict,
				  const char *sensor)
{
	prop_object_t obj, obj2, obj3;
	char *strval, *endptr;
	double val;

	/*
	 * rfact property set?
	 */
	obj = prop_dictionary_get(csdict, "rfact");
	if (obj) {
		obj2 = prop_dictionary_get(ksdict, "allow-rfact");
		if (prop_bool_true(obj2)) {
			strval = prop_string_cstring(obj);
			val = strtod(strval, &endptr);
			if (*endptr != '\0')
				config_errmsg(VALUE_ERR, "rfact", sensor);

			if (!prop_dictionary_set_uint32(csdict, "rfact", val))
				err(EXIT_FAILURE, "dict_set rfact");
		} else
			config_errmsg(PROP_ERR, "rfact", sensor);
	}

	/*
	 * critical-capacity property set?
	 */
	obj = prop_dictionary_get(csdict, "critical-capacity");
	if (obj) {
		obj2 = prop_dictionary_get(ksdict, "want-percentage");
		obj3 = prop_dictionary_get(ksdict, "monitoring-supported");
		if (prop_bool_true(obj2) && prop_bool_true(obj3)) {
			strval = prop_string_cstring(obj);
			val = strtod(strval, &endptr);
			if ((*endptr != '\0') || (val < 0 || val > 100))
				config_errmsg(VALUE_ERR,
					      "critical-capacity",
					      sensor);
			/*
			 * Convert the value to a valid percentage.
			 */
			obj = prop_dictionary_get(ksdict, "max-value");
			val = (val / 100) * prop_number_integer_value(obj);

			if (!prop_dictionary_set_uint8(csdict,
						       "critical-capacity",
						       val))
				err(EXIT_FAILURE, "dict_set critcap");
		} else
			config_errmsg(PROP_ERR,
				      "critical-capacity", sensor);
	}

	/*
	 * critical-max property set?
	 */
	obj = prop_dictionary_get(csdict, "critical-max");
	if (obj) {
		obj2 = prop_dictionary_get(ksdict, "monitoring-supported");
		if (!prop_bool_true(obj2))
			config_errmsg(PROP_ERR, "critical-max", sensor);

		strval = prop_string_cstring(obj);
		obj = convert_val_to_pnumber(ksdict, "critical-max",
					     sensor, strval);
		if (!prop_dictionary_set(csdict, "critical-max", obj))
			err(EXIT_FAILURE, "prop_dict_set cmax");
	}

	/*
	 * critical-min property set?
	 */
	obj = prop_dictionary_get(csdict, "critical-min");
	if (obj) {
		obj2 = prop_dictionary_get(ksdict, "monitoring-supported");
		if (!prop_bool_true(obj2))
			config_errmsg(PROP_ERR, "critical-min", sensor);

		strval = prop_string_cstring(obj);
		obj = convert_val_to_pnumber(ksdict, "critical-min",
					     sensor, strval);
		if (!prop_dictionary_set(csdict, "critical-min", obj))
			err(EXIT_FAILURE, "prop_dict_set cmin");
	}
}

/*
 * Conversions for critical-max and critical-min properties.
 */
prop_number_t
convert_val_to_pnumber(prop_dictionary_t kdict, const char *prop,
		       const char *sensor, char *value)
{
	prop_object_t obj;
	prop_number_t num;
	double val;
	char *strval, *tmp, *endptr;
	bool celsius;
	size_t len;

	/*
	 * Make the conversion for sensor's type.
	 */
	obj = prop_dictionary_get(kdict, "type");
	if (prop_string_equals_cstring(obj, "Temperature")) {
		tmp = strchr(value, 'C');
		if (tmp)
			celsius = true;
		else {
			tmp = strchr(value, 'F');
			if (!tmp)
				config_errmsg(VALUE_ERR, prop, sensor);

			celsius = false;
		}

		len = strlen(value);
		strval = calloc(len, sizeof(*value));
		if (!strval)
			err(EXIT_FAILURE, "calloc");
	
		(void)strlcpy(strval, value, len);
		val = strtod(strval, &endptr);
		if (*endptr != '\0')
			config_errmsg(VALUE_ERR, prop, sensor);

		/* convert to fahrenheit */
		if (!celsius)
			val = (val - 32.0) * (5.0 / 9.0);

		/* convert to microKelvin */
		val = val * 1000000 + 273150000;
		num = prop_number_create_unsigned_integer(val);

	} else if (prop_string_equals_cstring(obj, "Fan")) {
		/* no conversion */
		val = strtod(value, &endptr);
		if (*endptr != '\0')
			config_errmsg(VALUE_ERR, prop, sensor);

		num = prop_number_create_unsigned_integer(val);

	} else {
		val = strtod(value, &endptr);
		if (*endptr != '\0')
			config_errmsg(VALUE_ERR, prop, sensor);

		/* convert to m[V,W,Ohms] again */
		val *= 1000000.0;
		num = prop_number_create_integer(val);
	}

	return num;
}
