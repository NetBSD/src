/*
 * WPA Supplicant / Configuration blobs
 * Copyright (c) 2003-2007, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#ifndef CONFIG_BLOB_H
#define CONFIG_BLOB_H

/**
 * struct wpa_config_blob - Named configuration blob
 *
 * This data structure is used to provide storage for binary objects to store
 * abstract information like certificates and private keys inlined with the
 * configuration data.
 */
struct wpa_config_blob {
	/**
	 * name - Blob name
	 */
	char *name;

	/**
	 * data - Pointer to binary data
	 */
	u8 *data;

	/**
	 * len - Length of binary data
	 */
	size_t len;

	/**
	 * next - Pointer to next blob in the configuration
	 */
	struct wpa_config_blob *next;
};

struct wpa_config;

const struct wpa_config_blob * wpa_config_get_blob(struct wpa_config *config,
						   const char *name);
void wpa_config_set_blob(struct wpa_config *config,
			 struct wpa_config_blob *blob);
void wpa_config_free_blob(struct wpa_config_blob *blob);
int wpa_config_remove_blob(struct wpa_config *config, const char *name);

#endif /* CONFIG_BLOB_H */
