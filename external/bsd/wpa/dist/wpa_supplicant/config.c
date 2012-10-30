/*
 * WPA Supplicant / Configuration parser and common functions
 * Copyright (c) 2003-2008, Jouni Malinen <j@w1.fi>
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

#include "includes.h"

#include "common.h"
#include "utils/uuid.h"
#include "crypto/sha1.h"
#include "rsn_supp/wpa.h"
#include "eap_peer/eap.h"
#include "p2p/p2p.h"
#include "config.h"


#if !defined(CONFIG_CTRL_IFACE) && defined(CONFIG_NO_CONFIG_WRITE)
#define NO_CONFIG_WRITE
#endif

/*
 * Structure for network configuration parsing. This data is used to implement
 * a generic parser for each network block variable. The table of configuration
 * variables is defined below in this file (ssid_fields[]).
 */
struct parse_data {
	/* Configuration variable name */
	char *name;

	/* Parser function for this variable */
	int (*parser)(const struct parse_data *data, struct wpa_ssid *ssid,
		      int line, const char *value);

#ifndef NO_CONFIG_WRITE
	/* Writer function (i.e., to get the variable in text format from
	 * internal presentation). */
	char * (*writer)(const struct parse_data *data, struct wpa_ssid *ssid);
#endif /* NO_CONFIG_WRITE */

	/* Variable specific parameters for the parser. */
	void *param1, *param2, *param3, *param4;

	/* 0 = this variable can be included in debug output and ctrl_iface
	 * 1 = this variable contains key/private data and it must not be
	 *     included in debug output unless explicitly requested. In
	 *     addition, this variable will not be readable through the
	 *     ctrl_iface.
	 */
	int key_data;
};


static char * wpa_config_parse_string(const char *value, size_t *len)
{
	if (*value == '"') {
		const char *pos;
		char *str;
		value++;
		pos = os_strrchr(value, '"');
		if (pos == NULL || pos[1] != '\0')
			return NULL;
		*len = pos - value;
		str = os_malloc(*len + 1);
		if (str == NULL)
			return NULL;
		os_memcpy(str, value, *len);
		str[*len] = '\0';
		return str;
	} else {
		u8 *str;
		size_t tlen, hlen = os_strlen(value);
		if (hlen & 1)
			return NULL;
		tlen = hlen / 2;
		str = os_malloc(tlen + 1);
		if (str == NULL)
			return NULL;
		if (hexstr2bin(value, str, tlen)) {
			os_free(str);
			return NULL;
		}
		str[tlen] = '\0';
		*len = tlen;
		return (char *) str;
	}
}


static int wpa_config_parse_str(const struct parse_data *data,
				struct wpa_ssid *ssid,
				int line, const char *value)
{
	size_t res_len, *dst_len;
	char **dst, *tmp;

	if (os_strcmp(value, "NULL") == 0) {
		wpa_printf(MSG_DEBUG, "Unset configuration string '%s'",
			   data->name);
		tmp = NULL;
		res_len = 0;
		goto set;
	}

	tmp = wpa_config_parse_string(value, &res_len);
	if (tmp == NULL) {
		wpa_printf(MSG_ERROR, "Line %d: failed to parse %s '%s'.",
			   line, data->name,
			   data->key_data ? "[KEY DATA REMOVED]" : value);
		return -1;
	}

	if (data->key_data) {
		wpa_hexdump_ascii_key(MSG_MSGDUMP, data->name,
				      (u8 *) tmp, res_len);
	} else {
		wpa_hexdump_ascii(MSG_MSGDUMP, data->name,
				  (u8 *) tmp, res_len);
	}

	if (data->param3 && res_len < (size_t) data->param3) {
		wpa_printf(MSG_ERROR, "Line %d: too short %s (len=%lu "
			   "min_len=%ld)", line, data->name,
			   (unsigned long) res_len, (long) data->param3);
		os_free(tmp);
		return -1;
	}

	if (data->param4 && res_len > (size_t) data->param4) {
		wpa_printf(MSG_ERROR, "Line %d: too long %s (len=%lu "
			   "max_len=%ld)", line, data->name,
			   (unsigned long) res_len, (long) data->param4);
		os_free(tmp);
		return -1;
	}

set:
	dst = (char **) (((u8 *) ssid) + (long) data->param1);
	dst_len = (size_t *) (((u8 *) ssid) + (long) data->param2);
	os_free(*dst);
	*dst = tmp;
	if (data->param2)
		*dst_len = res_len;

	return 0;
}


#ifndef NO_CONFIG_WRITE
static int is_hex(const u8 *data, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		if (data[i] < 32 || data[i] >= 127)
			return 1;
	}
	return 0;
}


static char * wpa_config_write_string_ascii(const u8 *value, size_t len)
{
	char *buf;

	buf = os_malloc(len + 3);
	if (buf == NULL)
		return NULL;
	buf[0] = '"';
	os_memcpy(buf + 1, value, len);
	buf[len + 1] = '"';
	buf[len + 2] = '\0';

	return buf;
}


static char * wpa_config_write_string_hex(const u8 *value, size_t len)
{
	char *buf;

	buf = os_zalloc(2 * len + 1);
	if (buf == NULL)
		return NULL;
	wpa_snprintf_hex(buf, 2 * len + 1, value, len);

	return buf;
}


static char * wpa_config_write_string(const u8 *value, size_t len)
{
	if (value == NULL)
		return NULL;

	if (is_hex(value, len))
		return wpa_config_write_string_hex(value, len);
	else
		return wpa_config_write_string_ascii(value, len);
}


static char * wpa_config_write_str(const struct parse_data *data,
				   struct wpa_ssid *ssid)
{
	size_t len;
	char **src;

	src = (char **) (((u8 *) ssid) + (long) data->param1);
	if (*src == NULL)
		return NULL;

	if (data->param2)
		len = *((size_t *) (((u8 *) ssid) + (long) data->param2));
	else
		len = os_strlen(*src);

	return wpa_config_write_string((const u8 *) *src, len);
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_int(const struct parse_data *data,
				struct wpa_ssid *ssid,
				int line, const char *value)
{
	int *dst;

	dst = (int *) (((u8 *) ssid) + (long) data->param1);
	*dst = atoi(value);
	wpa_printf(MSG_MSGDUMP, "%s=%d (0x%x)", data->name, *dst, *dst);

	if (data->param3 && *dst < (long) data->param3) {
		wpa_printf(MSG_ERROR, "Line %d: too small %s (value=%d "
			   "min_value=%ld)", line, data->name, *dst,
			   (long) data->param3);
		*dst = (long) data->param3;
		return -1;
	}

	if (data->param4 && *dst > (long) data->param4) {
		wpa_printf(MSG_ERROR, "Line %d: too large %s (value=%d "
			   "max_value=%ld)", line, data->name, *dst,
			   (long) data->param4);
		*dst = (long) data->param4;
		return -1;
	}

	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_int(const struct parse_data *data,
				   struct wpa_ssid *ssid)
{
	int *src, res;
	char *value;

	src = (int *) (((u8 *) ssid) + (long) data->param1);

	value = os_malloc(20);
	if (value == NULL)
		return NULL;
	res = os_snprintf(value, 20, "%d", *src);
	if (res < 0 || res >= 20) {
		os_free(value);
		return NULL;
	}
	value[20 - 1] = '\0';
	return value;
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_bssid(const struct parse_data *data,
				  struct wpa_ssid *ssid, int line,
				  const char *value)
{
	if (value[0] == '\0' || os_strcmp(value, "\"\"") == 0 ||
	    os_strcmp(value, "any") == 0) {
		ssid->bssid_set = 0;
		wpa_printf(MSG_MSGDUMP, "BSSID any");
		return 0;
	}
	if (hwaddr_aton(value, ssid->bssid)) {
		wpa_printf(MSG_ERROR, "Line %d: Invalid BSSID '%s'.",
			   line, value);
		return -1;
	}
	ssid->bssid_set = 1;
	wpa_hexdump(MSG_MSGDUMP, "BSSID", ssid->bssid, ETH_ALEN);
	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_bssid(const struct parse_data *data,
				     struct wpa_ssid *ssid)
{
	char *value;
	int res;

	if (!ssid->bssid_set)
		return NULL;

	value = os_malloc(20);
	if (value == NULL)
		return NULL;
	res = os_snprintf(value, 20, MACSTR, MAC2STR(ssid->bssid));
	if (res < 0 || res >= 20) {
		os_free(value);
		return NULL;
	}
	value[20 - 1] = '\0';
	return value;
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_psk(const struct parse_data *data,
				struct wpa_ssid *ssid, int line,
				const char *value)
{
	if (*value == '"') {
#ifndef CONFIG_NO_PBKDF2
		const char *pos;
		size_t len;

		value++;
		pos = os_strrchr(value, '"');
		if (pos)
			len = pos - value;
		else
			len = os_strlen(value);
		if (len < 8 || len > 63) {
			wpa_printf(MSG_ERROR, "Line %d: Invalid passphrase "
				   "length %lu (expected: 8..63) '%s'.",
				   line, (unsigned long) len, value);
			return -1;
		}
		wpa_hexdump_ascii_key(MSG_MSGDUMP, "PSK (ASCII passphrase)",
				      (u8 *) value, len);
		if (ssid->passphrase && os_strlen(ssid->passphrase) == len &&
		    os_memcmp(ssid->passphrase, value, len) == 0)
			return 0;
		ssid->psk_set = 0;
		os_free(ssid->passphrase);
		ssid->passphrase = os_malloc(len + 1);
		if (ssid->passphrase == NULL)
			return -1;
		os_memcpy(ssid->passphrase, value, len);
		ssid->passphrase[len] = '\0';
		return 0;
#else /* CONFIG_NO_PBKDF2 */
		wpa_printf(MSG_ERROR, "Line %d: ASCII passphrase not "
			   "supported.", line);
		return -1;
#endif /* CONFIG_NO_PBKDF2 */
	}

	if (hexstr2bin(value, ssid->psk, PMK_LEN) ||
	    value[PMK_LEN * 2] != '\0') {
		wpa_printf(MSG_ERROR, "Line %d: Invalid PSK '%s'.",
			   line, value);
		return -1;
	}

	os_free(ssid->passphrase);
	ssid->passphrase = NULL;

	ssid->psk_set = 1;
	wpa_hexdump_key(MSG_MSGDUMP, "PSK", ssid->psk, PMK_LEN);
	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_psk(const struct parse_data *data,
				   struct wpa_ssid *ssid)
{
	if (ssid->passphrase)
		return wpa_config_write_string_ascii(
			(const u8 *) ssid->passphrase,
			os_strlen(ssid->passphrase));

	if (ssid->psk_set)
		return wpa_config_write_string_hex(ssid->psk, PMK_LEN);

	return NULL;
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_proto(const struct parse_data *data,
				  struct wpa_ssid *ssid, int line,
				  const char *value)
{
	int val = 0, last, errors = 0;
	char *start, *end, *buf;

	buf = os_strdup(value);
	if (buf == NULL)
		return -1;
	start = buf;

	while (*start != '\0') {
		while (*start == ' ' || *start == '\t')
			start++;
		if (*start == '\0')
			break;
		end = start;
		while (*end != ' ' && *end != '\t' && *end != '\0')
			end++;
		last = *end == '\0';
		*end = '\0';
		if (os_strcmp(start, "WPA") == 0)
			val |= WPA_PROTO_WPA;
		else if (os_strcmp(start, "RSN") == 0 ||
			 os_strcmp(start, "WPA2") == 0)
			val |= WPA_PROTO_RSN;
		else {
			wpa_printf(MSG_ERROR, "Line %d: invalid proto '%s'",
				   line, start);
			errors++;
		}

		if (last)
			break;
		start = end + 1;
	}
	os_free(buf);

	if (val == 0) {
		wpa_printf(MSG_ERROR,
			   "Line %d: no proto values configured.", line);
		errors++;
	}

	wpa_printf(MSG_MSGDUMP, "proto: 0x%x", val);
	ssid->proto = val;
	return errors ? -1 : 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_proto(const struct parse_data *data,
				     struct wpa_ssid *ssid)
{
	int first = 1, ret;
	char *buf, *pos, *end;

	pos = buf = os_zalloc(10);
	if (buf == NULL)
		return NULL;
	end = buf + 10;

	if (ssid->proto & WPA_PROTO_WPA) {
		ret = os_snprintf(pos, end - pos, "%sWPA", first ? "" : " ");
		if (ret < 0 || ret >= end - pos)
			return buf;
		pos += ret;
		first = 0;
	}

	if (ssid->proto & WPA_PROTO_RSN) {
		ret = os_snprintf(pos, end - pos, "%sRSN", first ? "" : " ");
		if (ret < 0 || ret >= end - pos)
			return buf;
		pos += ret;
		first = 0;
	}

	return buf;
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_key_mgmt(const struct parse_data *data,
				     struct wpa_ssid *ssid, int line,
				     const char *value)
{
	int val = 0, last, errors = 0;
	char *start, *end, *buf;

	buf = os_strdup(value);
	if (buf == NULL)
		return -1;
	start = buf;

	while (*start != '\0') {
		while (*start == ' ' || *start == '\t')
			start++;
		if (*start == '\0')
			break;
		end = start;
		while (*end != ' ' && *end != '\t' && *end != '\0')
			end++;
		last = *end == '\0';
		*end = '\0';
		if (os_strcmp(start, "WPA-PSK") == 0)
			val |= WPA_KEY_MGMT_PSK;
		else if (os_strcmp(start, "WPA-EAP") == 0)
			val |= WPA_KEY_MGMT_IEEE8021X;
		else if (os_strcmp(start, "IEEE8021X") == 0)
			val |= WPA_KEY_MGMT_IEEE8021X_NO_WPA;
		else if (os_strcmp(start, "NONE") == 0)
			val |= WPA_KEY_MGMT_NONE;
		else if (os_strcmp(start, "WPA-NONE") == 0)
			val |= WPA_KEY_MGMT_WPA_NONE;
#ifdef CONFIG_IEEE80211R
		else if (os_strcmp(start, "FT-PSK") == 0)
			val |= WPA_KEY_MGMT_FT_PSK;
		else if (os_strcmp(start, "FT-EAP") == 0)
			val |= WPA_KEY_MGMT_FT_IEEE8021X;
#endif /* CONFIG_IEEE80211R */
#ifdef CONFIG_IEEE80211W
		else if (os_strcmp(start, "WPA-PSK-SHA256") == 0)
			val |= WPA_KEY_MGMT_PSK_SHA256;
		else if (os_strcmp(start, "WPA-EAP-SHA256") == 0)
			val |= WPA_KEY_MGMT_IEEE8021X_SHA256;
#endif /* CONFIG_IEEE80211W */
#ifdef CONFIG_WPS
		else if (os_strcmp(start, "WPS") == 0)
			val |= WPA_KEY_MGMT_WPS;
#endif /* CONFIG_WPS */
		else {
			wpa_printf(MSG_ERROR, "Line %d: invalid key_mgmt '%s'",
				   line, start);
			errors++;
		}

		if (last)
			break;
		start = end + 1;
	}
	os_free(buf);

	if (val == 0) {
		wpa_printf(MSG_ERROR,
			   "Line %d: no key_mgmt values configured.", line);
		errors++;
	}

	wpa_printf(MSG_MSGDUMP, "key_mgmt: 0x%x", val);
	ssid->key_mgmt = val;
	return errors ? -1 : 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_key_mgmt(const struct parse_data *data,
					struct wpa_ssid *ssid)
{
	char *buf, *pos, *end;
	int ret;

	pos = buf = os_zalloc(50);
	if (buf == NULL)
		return NULL;
	end = buf + 50;

	if (ssid->key_mgmt & WPA_KEY_MGMT_PSK) {
		ret = os_snprintf(pos, end - pos, "%sWPA-PSK",
				  pos == buf ? "" : " ");
		if (ret < 0 || ret >= end - pos) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (ssid->key_mgmt & WPA_KEY_MGMT_IEEE8021X) {
		ret = os_snprintf(pos, end - pos, "%sWPA-EAP",
				  pos == buf ? "" : " ");
		if (ret < 0 || ret >= end - pos) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (ssid->key_mgmt & WPA_KEY_MGMT_IEEE8021X_NO_WPA) {
		ret = os_snprintf(pos, end - pos, "%sIEEE8021X",
				  pos == buf ? "" : " ");
		if (ret < 0 || ret >= end - pos) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (ssid->key_mgmt & WPA_KEY_MGMT_NONE) {
		ret = os_snprintf(pos, end - pos, "%sNONE",
				  pos == buf ? "" : " ");
		if (ret < 0 || ret >= end - pos) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (ssid->key_mgmt & WPA_KEY_MGMT_WPA_NONE) {
		ret = os_snprintf(pos, end - pos, "%sWPA-NONE",
				  pos == buf ? "" : " ");
		if (ret < 0 || ret >= end - pos) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

#ifdef CONFIG_IEEE80211R
	if (ssid->key_mgmt & WPA_KEY_MGMT_FT_PSK)
		pos += os_snprintf(pos, end - pos, "%sFT-PSK",
				   pos == buf ? "" : " ");

	if (ssid->key_mgmt & WPA_KEY_MGMT_FT_IEEE8021X)
		pos += os_snprintf(pos, end - pos, "%sFT-EAP",
				   pos == buf ? "" : " ");
#endif /* CONFIG_IEEE80211R */

#ifdef CONFIG_IEEE80211W
	if (ssid->key_mgmt & WPA_KEY_MGMT_PSK_SHA256)
		pos += os_snprintf(pos, end - pos, "%sWPA-PSK-SHA256",
				   pos == buf ? "" : " ");

	if (ssid->key_mgmt & WPA_KEY_MGMT_IEEE8021X_SHA256)
		pos += os_snprintf(pos, end - pos, "%sWPA-EAP-SHA256",
				   pos == buf ? "" : " ");
#endif /* CONFIG_IEEE80211W */

#ifdef CONFIG_WPS
	if (ssid->key_mgmt & WPA_KEY_MGMT_WPS)
		pos += os_snprintf(pos, end - pos, "%sWPS",
				   pos == buf ? "" : " ");
#endif /* CONFIG_WPS */

	return buf;
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_cipher(int line, const char *value)
{
	int val = 0, last;
	char *start, *end, *buf;

	buf = os_strdup(value);
	if (buf == NULL)
		return -1;
	start = buf;

	while (*start != '\0') {
		while (*start == ' ' || *start == '\t')
			start++;
		if (*start == '\0')
			break;
		end = start;
		while (*end != ' ' && *end != '\t' && *end != '\0')
			end++;
		last = *end == '\0';
		*end = '\0';
		if (os_strcmp(start, "CCMP") == 0)
			val |= WPA_CIPHER_CCMP;
		else if (os_strcmp(start, "TKIP") == 0)
			val |= WPA_CIPHER_TKIP;
		else if (os_strcmp(start, "WEP104") == 0)
			val |= WPA_CIPHER_WEP104;
		else if (os_strcmp(start, "WEP40") == 0)
			val |= WPA_CIPHER_WEP40;
		else if (os_strcmp(start, "NONE") == 0)
			val |= WPA_CIPHER_NONE;
		else {
			wpa_printf(MSG_ERROR, "Line %d: invalid cipher '%s'.",
				   line, start);
			os_free(buf);
			return -1;
		}

		if (last)
			break;
		start = end + 1;
	}
	os_free(buf);

	if (val == 0) {
		wpa_printf(MSG_ERROR, "Line %d: no cipher values configured.",
			   line);
		return -1;
	}
	return val;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_cipher(int cipher)
{
	char *buf, *pos, *end;
	int ret;

	pos = buf = os_zalloc(50);
	if (buf == NULL)
		return NULL;
	end = buf + 50;

	if (cipher & WPA_CIPHER_CCMP) {
		ret = os_snprintf(pos, end - pos, "%sCCMP",
				  pos == buf ? "" : " ");
		if (ret < 0 || ret >= end - pos) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (cipher & WPA_CIPHER_TKIP) {
		ret = os_snprintf(pos, end - pos, "%sTKIP",
				  pos == buf ? "" : " ");
		if (ret < 0 || ret >= end - pos) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (cipher & WPA_CIPHER_WEP104) {
		ret = os_snprintf(pos, end - pos, "%sWEP104",
				  pos == buf ? "" : " ");
		if (ret < 0 || ret >= end - pos) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (cipher & WPA_CIPHER_WEP40) {
		ret = os_snprintf(pos, end - pos, "%sWEP40",
				  pos == buf ? "" : " ");
		if (ret < 0 || ret >= end - pos) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (cipher & WPA_CIPHER_NONE) {
		ret = os_snprintf(pos, end - pos, "%sNONE",
				  pos == buf ? "" : " ");
		if (ret < 0 || ret >= end - pos) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	return buf;
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_pairwise(const struct parse_data *data,
				     struct wpa_ssid *ssid, int line,
				     const char *value)
{
	int val;
	val = wpa_config_parse_cipher(line, value);
	if (val == -1)
		return -1;
	if (val & ~(WPA_CIPHER_CCMP | WPA_CIPHER_TKIP | WPA_CIPHER_NONE)) {
		wpa_printf(MSG_ERROR, "Line %d: not allowed pairwise cipher "
			   "(0x%x).", line, val);
		return -1;
	}

	wpa_printf(MSG_MSGDUMP, "pairwise: 0x%x", val);
	ssid->pairwise_cipher = val;
	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_pairwise(const struct parse_data *data,
					struct wpa_ssid *ssid)
{
	return wpa_config_write_cipher(ssid->pairwise_cipher);
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_group(const struct parse_data *data,
				  struct wpa_ssid *ssid, int line,
				  const char *value)
{
	int val;
	val = wpa_config_parse_cipher(line, value);
	if (val == -1)
		return -1;
	if (val & ~(WPA_CIPHER_CCMP | WPA_CIPHER_TKIP | WPA_CIPHER_WEP104 |
		    WPA_CIPHER_WEP40)) {
		wpa_printf(MSG_ERROR, "Line %d: not allowed group cipher "
			   "(0x%x).", line, val);
		return -1;
	}

	wpa_printf(MSG_MSGDUMP, "group: 0x%x", val);
	ssid->group_cipher = val;
	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_group(const struct parse_data *data,
				     struct wpa_ssid *ssid)
{
	return wpa_config_write_cipher(ssid->group_cipher);
}
#endif /* NO_CONFIG_WRITE */


static int wpa_config_parse_auth_alg(const struct parse_data *data,
				     struct wpa_ssid *ssid, int line,
				     const char *value)
{
	int val = 0, last, errors = 0;
	char *start, *end, *buf;

	buf = os_strdup(value);
	if (buf == NULL)
		return -1;
	start = buf;

	while (*start != '\0') {
		while (*start == ' ' || *start == '\t')
			start++;
		if (*start == '\0')
			break;
		end = start;
		while (*end != ' ' && *end != '\t' && *end != '\0')
			end++;
		last = *end == '\0';
		*end = '\0';
		if (os_strcmp(start, "OPEN") == 0)
			val |= WPA_AUTH_ALG_OPEN;
		else if (os_strcmp(start, "SHARED") == 0)
			val |= WPA_AUTH_ALG_SHARED;
		else if (os_strcmp(start, "LEAP") == 0)
			val |= WPA_AUTH_ALG_LEAP;
		else {
			wpa_printf(MSG_ERROR, "Line %d: invalid auth_alg '%s'",
				   line, start);
			errors++;
		}

		if (last)
			break;
		start = end + 1;
	}
	os_free(buf);

	if (val == 0) {
		wpa_printf(MSG_ERROR,
			   "Line %d: no auth_alg values configured.", line);
		errors++;
	}

	wpa_printf(MSG_MSGDUMP, "auth_alg: 0x%x", val);
	ssid->auth_alg = val;
	return errors ? -1 : 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_auth_alg(const struct parse_data *data,
					struct wpa_ssid *ssid)
{
	char *buf, *pos, *end;
	int ret;

	pos = buf = os_zalloc(30);
	if (buf == NULL)
		return NULL;
	end = buf + 30;

	if (ssid->auth_alg & WPA_AUTH_ALG_OPEN) {
		ret = os_snprintf(pos, end - pos, "%sOPEN",
				  pos == buf ? "" : " ");
		if (ret < 0 || ret >= end - pos) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (ssid->auth_alg & WPA_AUTH_ALG_SHARED) {
		ret = os_snprintf(pos, end - pos, "%sSHARED",
				  pos == buf ? "" : " ");
		if (ret < 0 || ret >= end - pos) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	if (ssid->auth_alg & WPA_AUTH_ALG_LEAP) {
		ret = os_snprintf(pos, end - pos, "%sLEAP",
				  pos == buf ? "" : " ");
		if (ret < 0 || ret >= end - pos) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	return buf;
}
#endif /* NO_CONFIG_WRITE */


static int * wpa_config_parse_freqs(const struct parse_data *data,
				    struct wpa_ssid *ssid, int line,
				    const char *value)
{
	int *freqs;
	size_t used, len;
	const char *pos;

	used = 0;
	len = 10;
	freqs = os_zalloc((len + 1) * sizeof(int));
	if (freqs == NULL)
		return NULL;

	pos = value;
	while (pos) {
		while (*pos == ' ')
			pos++;
		if (used == len) {
			int *n;
			size_t i;
			n = os_realloc(freqs, (len * 2 + 1) * sizeof(int));
			if (n == NULL) {
				os_free(freqs);
				return NULL;
			}
			for (i = len; i <= len * 2; i++)
				n[i] = 0;
			freqs = n;
			len *= 2;
		}

		freqs[used] = atoi(pos);
		if (freqs[used] == 0)
			break;
		used++;
		pos = os_strchr(pos + 1, ' ');
	}

	return freqs;
}


static int wpa_config_parse_scan_freq(const struct parse_data *data,
				      struct wpa_ssid *ssid, int line,
				      const char *value)
{
	int *freqs;

	freqs = wpa_config_parse_freqs(data, ssid, line, value);
	if (freqs == NULL)
		return -1;
	os_free(ssid->scan_freq);
	ssid->scan_freq = freqs;

	return 0;
}


static int wpa_config_parse_freq_list(const struct parse_data *data,
				      struct wpa_ssid *ssid, int line,
				      const char *value)
{
	int *freqs;

	freqs = wpa_config_parse_freqs(data, ssid, line, value);
	if (freqs == NULL)
		return -1;
	os_free(ssid->freq_list);
	ssid->freq_list = freqs;

	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_freqs(const struct parse_data *data,
				     const int *freqs)
{
	char *buf, *pos, *end;
	int i, ret;
	size_t count;

	if (freqs == NULL)
		return NULL;

	count = 0;
	for (i = 0; freqs[i]; i++)
		count++;

	pos = buf = os_zalloc(10 * count + 1);
	if (buf == NULL)
		return NULL;
	end = buf + 10 * count + 1;

	for (i = 0; freqs[i]; i++) {
		ret = os_snprintf(pos, end - pos, "%s%u",
				  i == 0 ? "" : " ", freqs[i]);
		if (ret < 0 || ret >= end - pos) {
			end[-1] = '\0';
			return buf;
		}
		pos += ret;
	}

	return buf;
}


static char * wpa_config_write_scan_freq(const struct parse_data *data,
					 struct wpa_ssid *ssid)
{
	return wpa_config_write_freqs(data, ssid->scan_freq);
}


static char * wpa_config_write_freq_list(const struct parse_data *data,
					 struct wpa_ssid *ssid)
{
	return wpa_config_write_freqs(data, ssid->freq_list);
}
#endif /* NO_CONFIG_WRITE */


#ifdef IEEE8021X_EAPOL
static int wpa_config_parse_eap(const struct parse_data *data,
				struct wpa_ssid *ssid, int line,
				const char *value)
{
	int last, errors = 0;
	char *start, *end, *buf;
	struct eap_method_type *methods = NULL, *tmp;
	size_t num_methods = 0;

	buf = os_strdup(value);
	if (buf == NULL)
		return -1;
	start = buf;

	while (*start != '\0') {
		while (*start == ' ' || *start == '\t')
			start++;
		if (*start == '\0')
			break;
		end = start;
		while (*end != ' ' && *end != '\t' && *end != '\0')
			end++;
		last = *end == '\0';
		*end = '\0';
		tmp = methods;
		methods = os_realloc(methods,
				     (num_methods + 1) * sizeof(*methods));
		if (methods == NULL) {
			os_free(tmp);
			os_free(buf);
			return -1;
		}
		methods[num_methods].method = eap_peer_get_type(
			start, &methods[num_methods].vendor);
		if (methods[num_methods].vendor == EAP_VENDOR_IETF &&
		    methods[num_methods].method == EAP_TYPE_NONE) {
			wpa_printf(MSG_ERROR, "Line %d: unknown EAP method "
				   "'%s'", line, start);
			wpa_printf(MSG_ERROR, "You may need to add support for"
				   " this EAP method during wpa_supplicant\n"
				   "build time configuration.\n"
				   "See README for more information.");
			errors++;
		} else if (methods[num_methods].vendor == EAP_VENDOR_IETF &&
			   methods[num_methods].method == EAP_TYPE_LEAP)
			ssid->leap++;
		else
			ssid->non_leap++;
		num_methods++;
		if (last)
			break;
		start = end + 1;
	}
	os_free(buf);

	tmp = methods;
	methods = os_realloc(methods, (num_methods + 1) * sizeof(*methods));
	if (methods == NULL) {
		os_free(tmp);
		return -1;
	}
	methods[num_methods].vendor = EAP_VENDOR_IETF;
	methods[num_methods].method = EAP_TYPE_NONE;
	num_methods++;

	wpa_hexdump(MSG_MSGDUMP, "eap methods",
		    (u8 *) methods, num_methods * sizeof(*methods));
	os_free(ssid->eap.eap_methods);
	ssid->eap.eap_methods = methods;
	return errors ? -1 : 0;
}


static char * wpa_config_write_eap(const struct parse_data *data,
				   struct wpa_ssid *ssid)
{
	int i, ret;
	char *buf, *pos, *end;
	const struct eap_method_type *eap_methods = ssid->eap.eap_methods;
	const char *name;

	if (eap_methods == NULL)
		return NULL;

	pos = buf = os_zalloc(100);
	if (buf == NULL)
		return NULL;
	end = buf + 100;

	for (i = 0; eap_methods[i].vendor != EAP_VENDOR_IETF ||
		     eap_methods[i].method != EAP_TYPE_NONE; i++) {
		name = eap_get_name(eap_methods[i].vendor,
				    eap_methods[i].method);
		if (name) {
			ret = os_snprintf(pos, end - pos, "%s%s",
					  pos == buf ? "" : " ", name);
			if (ret < 0 || ret >= end - pos)
				break;
			pos += ret;
		}
	}

	end[-1] = '\0';

	return buf;
}


static int wpa_config_parse_password(const struct parse_data *data,
				     struct wpa_ssid *ssid, int line,
				     const char *value)
{
	u8 *hash;

	if (os_strcmp(value, "NULL") == 0) {
		wpa_printf(MSG_DEBUG, "Unset configuration string 'password'");
		os_free(ssid->eap.password);
		ssid->eap.password = NULL;
		ssid->eap.password_len = 0;
		return 0;
	}

	if (os_strncmp(value, "hash:", 5) != 0) {
		char *tmp;
		size_t res_len;

		tmp = wpa_config_parse_string(value, &res_len);
		if (tmp == NULL) {
			wpa_printf(MSG_ERROR, "Line %d: failed to parse "
				   "password.", line);
			return -1;
		}
		wpa_hexdump_ascii_key(MSG_MSGDUMP, data->name,
				      (u8 *) tmp, res_len);

		os_free(ssid->eap.password);
		ssid->eap.password = (u8 *) tmp;
		ssid->eap.password_len = res_len;
		ssid->eap.flags &= ~EAP_CONFIG_FLAGS_PASSWORD_NTHASH;

		return 0;
	}


	/* NtPasswordHash: hash:<32 hex digits> */
	if (os_strlen(value + 5) != 2 * 16) {
		wpa_printf(MSG_ERROR, "Line %d: Invalid password hash length "
			   "(expected 32 hex digits)", line);
		return -1;
	}

	hash = os_malloc(16);
	if (hash == NULL)
		return -1;

	if (hexstr2bin(value + 5, hash, 16)) {
		os_free(hash);
		wpa_printf(MSG_ERROR, "Line %d: Invalid password hash", line);
		return -1;
	}

	wpa_hexdump_key(MSG_MSGDUMP, data->name, hash, 16);

	os_free(ssid->eap.password);
	ssid->eap.password = hash;
	ssid->eap.password_len = 16;
	ssid->eap.flags |= EAP_CONFIG_FLAGS_PASSWORD_NTHASH;

	return 0;
}


static char * wpa_config_write_password(const struct parse_data *data,
					struct wpa_ssid *ssid)
{
	char *buf;

	if (ssid->eap.password == NULL)
		return NULL;

	if (!(ssid->eap.flags & EAP_CONFIG_FLAGS_PASSWORD_NTHASH)) {
		return wpa_config_write_string(
			ssid->eap.password, ssid->eap.password_len);
	}

	buf = os_malloc(5 + 32 + 1);
	if (buf == NULL)
		return NULL;

	os_memcpy(buf, "hash:", 5);
	wpa_snprintf_hex(buf + 5, 32 + 1, ssid->eap.password, 16);

	return buf;
}
#endif /* IEEE8021X_EAPOL */


static int wpa_config_parse_wep_key(u8 *key, size_t *len, int line,
				    const char *value, int idx)
{
	char *buf, title[20];
	int res;

	buf = wpa_config_parse_string(value, len);
	if (buf == NULL) {
		wpa_printf(MSG_ERROR, "Line %d: Invalid WEP key %d '%s'.",
			   line, idx, value);
		return -1;
	}
	if (*len > MAX_WEP_KEY_LEN) {
		wpa_printf(MSG_ERROR, "Line %d: Too long WEP key %d '%s'.",
			   line, idx, value);
		os_free(buf);
		return -1;
	}
	os_memcpy(key, buf, *len);
	os_free(buf);
	res = os_snprintf(title, sizeof(title), "wep_key%d", idx);
	if (res >= 0 && (size_t) res < sizeof(title))
		wpa_hexdump_key(MSG_MSGDUMP, title, key, *len);
	return 0;
}


static int wpa_config_parse_wep_key0(const struct parse_data *data,
				     struct wpa_ssid *ssid, int line,
				     const char *value)
{
	return wpa_config_parse_wep_key(ssid->wep_key[0],
					&ssid->wep_key_len[0], line,
					value, 0);
}


static int wpa_config_parse_wep_key1(const struct parse_data *data,
				     struct wpa_ssid *ssid, int line,
				     const char *value)
{
	return wpa_config_parse_wep_key(ssid->wep_key[1],
					&ssid->wep_key_len[1], line,
					value, 1);
}


static int wpa_config_parse_wep_key2(const struct parse_data *data,
				     struct wpa_ssid *ssid, int line,
				     const char *value)
{
	return wpa_config_parse_wep_key(ssid->wep_key[2],
					&ssid->wep_key_len[2], line,
					value, 2);
}


static int wpa_config_parse_wep_key3(const struct parse_data *data,
				     struct wpa_ssid *ssid, int line,
				     const char *value)
{
	return wpa_config_parse_wep_key(ssid->wep_key[3],
					&ssid->wep_key_len[3], line,
					value, 3);
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_wep_key(struct wpa_ssid *ssid, int idx)
{
	if (ssid->wep_key_len[idx] == 0)
		return NULL;
	return wpa_config_write_string(ssid->wep_key[idx],
				       ssid->wep_key_len[idx]);
}


static char * wpa_config_write_wep_key0(const struct parse_data *data,
					struct wpa_ssid *ssid)
{
	return wpa_config_write_wep_key(ssid, 0);
}


static char * wpa_config_write_wep_key1(const struct parse_data *data,
					struct wpa_ssid *ssid)
{
	return wpa_config_write_wep_key(ssid, 1);
}


static char * wpa_config_write_wep_key2(const struct parse_data *data,
					struct wpa_ssid *ssid)
{
	return wpa_config_write_wep_key(ssid, 2);
}


static char * wpa_config_write_wep_key3(const struct parse_data *data,
					struct wpa_ssid *ssid)
{
	return wpa_config_write_wep_key(ssid, 3);
}
#endif /* NO_CONFIG_WRITE */


#ifdef CONFIG_P2P

static int wpa_config_parse_p2p_client_list(const struct parse_data *data,
					    struct wpa_ssid *ssid, int line,
					    const char *value)
{
	const char *pos;
	u8 *buf, *n, addr[ETH_ALEN];
	size_t count;

	buf = NULL;
	count = 0;

	pos = value;
	while (pos && *pos) {
		while (*pos == ' ')
			pos++;

		if (hwaddr_aton(pos, addr)) {
			wpa_printf(MSG_ERROR, "Line %d: Invalid "
				   "p2p_client_list address '%s'.",
				   line, value);
			/* continue anyway */
		} else {
			n = os_realloc(buf, (count + 1) * ETH_ALEN);
			if (n == NULL) {
				os_free(buf);
				return -1;
			}
			buf = n;
			os_memcpy(buf + count * ETH_ALEN, addr, ETH_ALEN);
			count++;
			wpa_hexdump(MSG_MSGDUMP, "p2p_client_list",
				    addr, ETH_ALEN);
		}

		pos = os_strchr(pos, ' ');
	}

	os_free(ssid->p2p_client_list);
	ssid->p2p_client_list = buf;
	ssid->num_p2p_clients = count;

	return 0;
}


#ifndef NO_CONFIG_WRITE
static char * wpa_config_write_p2p_client_list(const struct parse_data *data,
					       struct wpa_ssid *ssid)
{
	char *value, *end, *pos;
	int res;
	size_t i;

	if (ssid->p2p_client_list == NULL || ssid->num_p2p_clients == 0)
		return NULL;

	value = os_malloc(20 * ssid->num_p2p_clients);
	if (value == NULL)
		return NULL;
	pos = value;
	end = value + 20 * ssid->num_p2p_clients;

	for (i = 0; i < ssid->num_p2p_clients; i++) {
		res = os_snprintf(pos, end - pos, MACSTR " ",
				  MAC2STR(ssid->p2p_client_list +
					  i * ETH_ALEN));
		if (res < 0 || res >= end - pos) {
			os_free(value);
			return NULL;
		}
		pos += res;
	}

	if (pos > value)
		pos[-1] = '\0';

	return value;
}
#endif /* NO_CONFIG_WRITE */

#endif /* CONFIG_P2P */

/* Helper macros for network block parser */

#ifdef OFFSET
#undef OFFSET
#endif /* OFFSET */
/* OFFSET: Get offset of a variable within the wpa_ssid structure */
#define OFFSET(v) ((void *) &((struct wpa_ssid *) 0)->v)

/* STR: Define a string variable for an ASCII string; f = field name */
#ifdef NO_CONFIG_WRITE
#define _STR(f) #f, wpa_config_parse_str, OFFSET(f)
#define _STRe(f) #f, wpa_config_parse_str, OFFSET(eap.f)
#else /* NO_CONFIG_WRITE */
#define _STR(f) #f, wpa_config_parse_str, wpa_config_write_str, OFFSET(f)
#define _STRe(f) #f, wpa_config_parse_str, wpa_config_write_str, OFFSET(eap.f)
#endif /* NO_CONFIG_WRITE */
#define STR(f) _STR(f), NULL, NULL, NULL, 0
#define STRe(f) _STRe(f), NULL, NULL, NULL, 0
#define STR_KEY(f) _STR(f), NULL, NULL, NULL, 1
#define STR_KEYe(f) _STRe(f), NULL, NULL, NULL, 1

/* STR_LEN: Define a string variable with a separate variable for storing the
 * data length. Unlike STR(), this can be used to store arbitrary binary data
 * (i.e., even nul termination character). */
#define _STR_LEN(f) _STR(f), OFFSET(f ## _len)
#define _STR_LENe(f) _STRe(f), OFFSET(eap.f ## _len)
#define STR_LEN(f) _STR_LEN(f), NULL, NULL, 0
#define STR_LENe(f) _STR_LENe(f), NULL, NULL, 0
#define STR_LEN_KEY(f) _STR_LEN(f), NULL, NULL, 1

/* STR_RANGE: Like STR_LEN(), but with minimum and maximum allowed length
 * explicitly specified. */
#define _STR_RANGE(f, min, max) _STR_LEN(f), (void *) (min), (void *) (max)
#define STR_RANGE(f, min, max) _STR_RANGE(f, min, max), 0
#define STR_RANGE_KEY(f, min, max) _STR_RANGE(f, min, max), 1

#ifdef NO_CONFIG_WRITE
#define _INT(f) #f, wpa_config_parse_int, OFFSET(f), (void *) 0
#define _INTe(f) #f, wpa_config_parse_int, OFFSET(eap.f), (void *) 0
#else /* NO_CONFIG_WRITE */
#define _INT(f) #f, wpa_config_parse_int, wpa_config_write_int, \
	OFFSET(f), (void *) 0
#define _INTe(f) #f, wpa_config_parse_int, wpa_config_write_int, \
	OFFSET(eap.f), (void *) 0
#endif /* NO_CONFIG_WRITE */

/* INT: Define an integer variable */
#define INT(f) _INT(f), NULL, NULL, 0
#define INTe(f) _INTe(f), NULL, NULL, 0

/* INT_RANGE: Define an integer variable with allowed value range */
#define INT_RANGE(f, min, max) _INT(f), (void *) (min), (void *) (max), 0

/* FUNC: Define a configuration variable that uses a custom function for
 * parsing and writing the value. */
#ifdef NO_CONFIG_WRITE
#define _FUNC(f) #f, wpa_config_parse_ ## f, NULL, NULL, NULL, NULL
#else /* NO_CONFIG_WRITE */
#define _FUNC(f) #f, wpa_config_parse_ ## f, wpa_config_write_ ## f, \
	NULL, NULL, NULL, NULL
#endif /* NO_CONFIG_WRITE */
#define FUNC(f) _FUNC(f), 0
#define FUNC_KEY(f) _FUNC(f), 1

/*
 * Table of network configuration variables. This table is used to parse each
 * network configuration variable, e.g., each line in wpa_supplicant.conf file
 * that is inside a network block.
 *
 * This table is generated using the helper macros defined above and with
 * generous help from the C pre-processor. The field name is stored as a string
 * into .name and for STR and INT types, the offset of the target buffer within
 * struct wpa_ssid is stored in .param1. .param2 (if not NULL) is similar
 * offset to the field containing the length of the configuration variable.
 * .param3 and .param4 can be used to mark the allowed range (length for STR
 * and value for INT).
 *
 * For each configuration line in wpa_supplicant.conf, the parser goes through
 * this table and select the entry that matches with the field name. The parser
 * function (.parser) is then called to parse the actual value of the field.
 *
 * This kind of mechanism makes it easy to add new configuration parameters,
 * since only one line needs to be added into this table and into the
 * struct wpa_ssid definition if the new variable is either a string or
 * integer. More complex types will need to use their own parser and writer
 * functions.
 */
static const struct parse_data ssid_fields[] = {
	{ STR_RANGE(ssid, 0, MAX_SSID_LEN) },
	{ INT_RANGE(scan_ssid, 0, 1) },
	{ FUNC(bssid) },
	{ FUNC_KEY(psk) },
	{ FUNC(proto) },
	{ FUNC(key_mgmt) },
	{ FUNC(pairwise) },
	{ FUNC(group) },
	{ FUNC(auth_alg) },
	{ FUNC(scan_freq) },
	{ FUNC(freq_list) },
#ifdef IEEE8021X_EAPOL
	{ FUNC(eap) },
	{ STR_LENe(identity) },
	{ STR_LENe(anonymous_identity) },
	{ FUNC_KEY(password) },
	{ STRe(ca_cert) },
	{ STRe(ca_path) },
	{ STRe(client_cert) },
	{ STRe(private_key) },
	{ STR_KEYe(private_key_passwd) },
	{ STRe(dh_file) },
	{ STRe(subject_match) },
	{ STRe(altsubject_match) },
	{ STRe(ca_cert2) },
	{ STRe(ca_path2) },
	{ STRe(client_cert2) },
	{ STRe(private_key2) },
	{ STR_KEYe(private_key2_passwd) },
	{ STRe(dh_file2) },
	{ STRe(subject_match2) },
	{ STRe(altsubject_match2) },
	{ STRe(phase1) },
	{ STRe(phase2) },
	{ STRe(pcsc) },
	{ STR_KEYe(pin) },
	{ STRe(engine_id) },
	{ STRe(key_id) },
	{ STRe(cert_id) },
	{ STRe(ca_cert_id) },
	{ STR_KEYe(pin2) },
	{ STRe(engine2_id) },
	{ STRe(key2_id) },
	{ STRe(cert2_id) },
	{ STRe(ca_cert2_id) },
	{ INTe(engine) },
	{ INTe(engine2) },
	{ INT(eapol_flags) },
#endif /* IEEE8021X_EAPOL */
	{ FUNC_KEY(wep_key0) },
	{ FUNC_KEY(wep_key1) },
	{ FUNC_KEY(wep_key2) },
	{ FUNC_KEY(wep_key3) },
	{ INT(wep_tx_keyidx) },
	{ INT(priority) },
#ifdef IEEE8021X_EAPOL
	{ INT(eap_workaround) },
	{ STRe(pac_file) },
	{ INTe(fragment_size) },
#endif /* IEEE8021X_EAPOL */
	{ INT_RANGE(mode, 0, 4) },
	{ INT_RANGE(proactive_key_caching, 0, 1) },
	{ INT_RANGE(disabled, 0, 2) },
	{ STR(id_str) },
#ifdef CONFIG_IEEE80211W
	{ INT_RANGE(ieee80211w, 0, 2) },
#endif /* CONFIG_IEEE80211W */
	{ INT_RANGE(peerkey, 0, 1) },
	{ INT_RANGE(mixed_cell, 0, 1) },
	{ INT_RANGE(frequency, 0, 10000) },
	{ INT(wpa_ptk_rekey) },
	{ STR(bgscan) },
#ifdef CONFIG_P2P
	{ FUNC(p2p_client_list) },
#endif /* CONFIG_P2P */
};

#undef OFFSET
#undef _STR
#undef STR
#undef STR_KEY
#undef _STR_LEN
#undef STR_LEN
#undef STR_LEN_KEY
#undef _STR_RANGE
#undef STR_RANGE
#undef STR_RANGE_KEY
#undef _INT
#undef INT
#undef INT_RANGE
#undef _FUNC
#undef FUNC
#undef FUNC_KEY
#define NUM_SSID_FIELDS (sizeof(ssid_fields) / sizeof(ssid_fields[0]))


/**
 * wpa_config_add_prio_network - Add a network to priority lists
 * @config: Configuration data from wpa_config_read()
 * @ssid: Pointer to the network configuration to be added to the list
 * Returns: 0 on success, -1 on failure
 *
 * This function is used to add a network block to the priority list of
 * networks. This must be called for each network when reading in the full
 * configuration. In addition, this can be used indirectly when updating
 * priorities by calling wpa_config_update_prio_list().
 */
int wpa_config_add_prio_network(struct wpa_config *config,
				struct wpa_ssid *ssid)
{
	int prio;
	struct wpa_ssid *prev, **nlist;

	/*
	 * Add to an existing priority list if one is available for the
	 * configured priority level for this network.
	 */
	for (prio = 0; prio < config->num_prio; prio++) {
		prev = config->pssid[prio];
		if (prev->priority == ssid->priority) {
			while (prev->pnext)
				prev = prev->pnext;
			prev->pnext = ssid;
			return 0;
		}
	}

	/* First network for this priority - add a new priority list */
	nlist = os_realloc(config->pssid,
			   (config->num_prio + 1) * sizeof(struct wpa_ssid *));
	if (nlist == NULL)
		return -1;

	for (prio = 0; prio < config->num_prio; prio++) {
		if (nlist[prio]->priority < ssid->priority)
			break;
	}

	os_memmove(&nlist[prio + 1], &nlist[prio],
		   (config->num_prio - prio) * sizeof(struct wpa_ssid *));

	nlist[prio] = ssid;
	config->num_prio++;
	config->pssid = nlist;

	return 0;
}


/**
 * wpa_config_update_prio_list - Update network priority list
 * @config: Configuration data from wpa_config_read()
 * Returns: 0 on success, -1 on failure
 *
 * This function is called to update the priority list of networks in the
 * configuration when a network is being added or removed. This is also called
 * if a priority for a network is changed.
 */
int wpa_config_update_prio_list(struct wpa_config *config)
{
	struct wpa_ssid *ssid;
	int ret = 0;

	os_free(config->pssid);
	config->pssid = NULL;
	config->num_prio = 0;

	ssid = config->ssid;
	while (ssid) {
		ssid->pnext = NULL;
		if (wpa_config_add_prio_network(config, ssid) < 0)
			ret = -1;
		ssid = ssid->next;
	}

	return ret;
}


#ifdef IEEE8021X_EAPOL
static void eap_peer_config_free(struct eap_peer_config *eap)
{
	os_free(eap->eap_methods);
	os_free(eap->identity);
	os_free(eap->anonymous_identity);
	os_free(eap->password);
	os_free(eap->ca_cert);
	os_free(eap->ca_path);
	os_free(eap->client_cert);
	os_free(eap->private_key);
	os_free(eap->private_key_passwd);
	os_free(eap->dh_file);
	os_free(eap->subject_match);
	os_free(eap->altsubject_match);
	os_free(eap->ca_cert2);
	os_free(eap->ca_path2);
	os_free(eap->client_cert2);
	os_free(eap->private_key2);
	os_free(eap->private_key2_passwd);
	os_free(eap->dh_file2);
	os_free(eap->subject_match2);
	os_free(eap->altsubject_match2);
	os_free(eap->phase1);
	os_free(eap->phase2);
	os_free(eap->pcsc);
	os_free(eap->pin);
	os_free(eap->engine_id);
	os_free(eap->key_id);
	os_free(eap->cert_id);
	os_free(eap->ca_cert_id);
	os_free(eap->key2_id);
	os_free(eap->cert2_id);
	os_free(eap->ca_cert2_id);
	os_free(eap->pin2);
	os_free(eap->engine2_id);
	os_free(eap->otp);
	os_free(eap->pending_req_otp);
	os_free(eap->pac_file);
	os_free(eap->new_password);
}
#endif /* IEEE8021X_EAPOL */


/**
 * wpa_config_free_ssid - Free network/ssid configuration data
 * @ssid: Configuration data for the network
 *
 * This function frees all resources allocated for the network configuration
 * data.
 */
void wpa_config_free_ssid(struct wpa_ssid *ssid)
{
	os_free(ssid->ssid);
	os_free(ssid->passphrase);
#ifdef IEEE8021X_EAPOL
	eap_peer_config_free(&ssid->eap);
#endif /* IEEE8021X_EAPOL */
	os_free(ssid->id_str);
	os_free(ssid->scan_freq);
	os_free(ssid->freq_list);
	os_free(ssid->bgscan);
	os_free(ssid->p2p_client_list);
	os_free(ssid);
}


/**
 * wpa_config_free - Free configuration data
 * @config: Configuration data from wpa_config_read()
 *
 * This function frees all resources allocated for the configuration data by
 * wpa_config_read().
 */
void wpa_config_free(struct wpa_config *config)
{
#ifndef CONFIG_NO_CONFIG_BLOBS
	struct wpa_config_blob *blob, *prevblob;
#endif /* CONFIG_NO_CONFIG_BLOBS */
	struct wpa_ssid *ssid, *prev = NULL;

	ssid = config->ssid;
	while (ssid) {
		prev = ssid;
		ssid = ssid->next;
		wpa_config_free_ssid(prev);
	}

#ifndef CONFIG_NO_CONFIG_BLOBS
	blob = config->blobs;
	prevblob = NULL;
	while (blob) {
		prevblob = blob;
		blob = blob->next;
		wpa_config_free_blob(prevblob);
	}
#endif /* CONFIG_NO_CONFIG_BLOBS */

	os_free(config->ctrl_interface);
	os_free(config->ctrl_interface_group);
	os_free(config->opensc_engine_path);
	os_free(config->pkcs11_engine_path);
	os_free(config->pkcs11_module_path);
	os_free(config->driver_param);
	os_free(config->device_name);
	os_free(config->manufacturer);
	os_free(config->model_name);
	os_free(config->model_number);
	os_free(config->serial_number);
	os_free(config->config_methods);
	os_free(config->p2p_ssid_postfix);
	os_free(config->pssid);
	os_free(config->home_realm);
	os_free(config->home_username);
	os_free(config->home_password);
	os_free(config->home_ca_cert);
	os_free(config->home_imsi);
	os_free(config->home_milenage);
	os_free(config->p2p_pref_chan);
	os_free(config);
}


/**
 * wpa_config_foreach_network - Iterate over each configured network
 * @config: Configuration data from wpa_config_read()
 * @func: Callback function to process each network
 * @arg: Opaque argument to pass to callback function
 *
 * Iterate over the set of configured networks calling the specified
 * function for each item. We guard against callbacks removing the
 * supplied network.
 */
void wpa_config_foreach_network(struct wpa_config *config,
				void (*func)(void *, struct wpa_ssid *),
				void *arg)
{
	struct wpa_ssid *ssid, *next;

	ssid = config->ssid;
	while (ssid) {
		next = ssid->next;
		func(arg, ssid);
		ssid = next;
	}
}


/**
 * wpa_config_get_network - Get configured network based on id
 * @config: Configuration data from wpa_config_read()
 * @id: Unique network id to search for
 * Returns: Network configuration or %NULL if not found
 */
struct wpa_ssid * wpa_config_get_network(struct wpa_config *config, int id)
{
	struct wpa_ssid *ssid;

	ssid = config->ssid;
	while (ssid) {
		if (id == ssid->id)
			break;
		ssid = ssid->next;
	}

	return ssid;
}


/**
 * wpa_config_add_network - Add a new network with empty configuration
 * @config: Configuration data from wpa_config_read()
 * Returns: The new network configuration or %NULL if operation failed
 */
struct wpa_ssid * wpa_config_add_network(struct wpa_config *config)
{
	int id;
	struct wpa_ssid *ssid, *last = NULL;

	id = -1;
	ssid = config->ssid;
	while (ssid) {
		if (ssid->id > id)
			id = ssid->id;
		last = ssid;
		ssid = ssid->next;
	}
	id++;

	ssid = os_zalloc(sizeof(*ssid));
	if (ssid == NULL)
		return NULL;
	ssid->id = id;
	if (last)
		last->next = ssid;
	else
		config->ssid = ssid;

	wpa_config_update_prio_list(config);

	return ssid;
}


/**
 * wpa_config_remove_network - Remove a configured network based on id
 * @config: Configuration data from wpa_config_read()
 * @id: Unique network id to search for
 * Returns: 0 on success, or -1 if the network was not found
 */
int wpa_config_remove_network(struct wpa_config *config, int id)
{
	struct wpa_ssid *ssid, *prev = NULL;

	ssid = config->ssid;
	while (ssid) {
		if (id == ssid->id)
			break;
		prev = ssid;
		ssid = ssid->next;
	}

	if (ssid == NULL)
		return -1;

	if (prev)
		prev->next = ssid->next;
	else
		config->ssid = ssid->next;

	wpa_config_update_prio_list(config);
	wpa_config_free_ssid(ssid);
	return 0;
}


/**
 * wpa_config_set_network_defaults - Set network default values
 * @ssid: Pointer to network configuration data
 */
void wpa_config_set_network_defaults(struct wpa_ssid *ssid)
{
	ssid->proto = DEFAULT_PROTO;
	ssid->pairwise_cipher = DEFAULT_PAIRWISE;
	ssid->group_cipher = DEFAULT_GROUP;
	ssid->key_mgmt = DEFAULT_KEY_MGMT;
#ifdef IEEE8021X_EAPOL
	ssid->eapol_flags = DEFAULT_EAPOL_FLAGS;
	ssid->eap_workaround = DEFAULT_EAP_WORKAROUND;
	ssid->eap.fragment_size = DEFAULT_FRAGMENT_SIZE;
#endif /* IEEE8021X_EAPOL */
}


/**
 * wpa_config_set - Set a variable in network configuration
 * @ssid: Pointer to network configuration data
 * @var: Variable name, e.g., "ssid"
 * @value: Variable value
 * @line: Line number in configuration file or 0 if not used
 * Returns: 0 on success, -1 on failure
 *
 * This function can be used to set network configuration variables based on
 * both the configuration file and management interface input. The value
 * parameter must be in the same format as the text-based configuration file is
 * using. For example, strings are using double quotation marks.
 */
int wpa_config_set(struct wpa_ssid *ssid, const char *var, const char *value,
		   int line)
{
	size_t i;
	int ret = 0;

	if (ssid == NULL || var == NULL || value == NULL)
		return -1;

	for (i = 0; i < NUM_SSID_FIELDS; i++) {
		const struct parse_data *field = &ssid_fields[i];
		if (os_strcmp(var, field->name) != 0)
			continue;

		if (field->parser(field, ssid, line, value)) {
			if (line) {
				wpa_printf(MSG_ERROR, "Line %d: failed to "
					   "parse %s '%s'.", line, var, value);
			}
			ret = -1;
		}
		break;
	}
	if (i == NUM_SSID_FIELDS) {
		if (line) {
			wpa_printf(MSG_ERROR, "Line %d: unknown network field "
				   "'%s'.", line, var);
		}
		ret = -1;
	}

	return ret;
}


int wpa_config_set_quoted(struct wpa_ssid *ssid, const char *var,
			  const char *value)
{
	size_t len;
	char *buf;
	int ret;

	len = os_strlen(value);
	buf = os_malloc(len + 3);
	if (buf == NULL)
		return -1;
	buf[0] = '"';
	os_memcpy(buf + 1, value, len);
	buf[len + 1] = '"';
	buf[len + 2] = '\0';
	ret = wpa_config_set(ssid, var, buf, 0);
	os_free(buf);
	return ret;
}


/**
 * wpa_config_get_all - Get all options from network configuration
 * @ssid: Pointer to network configuration data
 * @get_keys: Determines if keys/passwords will be included in returned list
 *	(if they may be exported)
 * Returns: %NULL terminated list of all set keys and their values in the form
 * of [key1, val1, key2, val2, ... , NULL]
 *
 * This function can be used to get list of all configured network properties.
 * The caller is responsible for freeing the returned list and all its
 * elements.
 */
char ** wpa_config_get_all(struct wpa_ssid *ssid, int get_keys)
{
	const struct parse_data *field;
	char *key, *value;
	size_t i;
	char **props;
	int fields_num;

	get_keys = get_keys && ssid->export_keys;

	props = os_zalloc(sizeof(char *) * ((2 * NUM_SSID_FIELDS) + 1));
	if (!props)
		return NULL;

	fields_num = 0;
	for (i = 0; i < NUM_SSID_FIELDS; i++) {
		field = &ssid_fields[i];
		if (field->key_data && !get_keys)
			continue;
		value = field->writer(field, ssid);
		if (value == NULL)
			continue;
		if (os_strlen(value) == 0) {
			os_free(value);
			continue;
		}

		key = os_strdup(field->name);
		if (key == NULL) {
			os_free(value);
			goto err;
		}

		props[fields_num * 2] = key;
		props[fields_num * 2 + 1] = value;

		fields_num++;
	}

	return props;

err:
	value = *props;
	while (value)
		os_free(value++);
	os_free(props);
	return NULL;
}


#ifndef NO_CONFIG_WRITE
/**
 * wpa_config_get - Get a variable in network configuration
 * @ssid: Pointer to network configuration data
 * @var: Variable name, e.g., "ssid"
 * Returns: Value of the variable or %NULL on failure
 *
 * This function can be used to get network configuration variables. The
 * returned value is a copy of the configuration variable in text format, i.e,.
 * the same format that the text-based configuration file and wpa_config_set()
 * are using for the value. The caller is responsible for freeing the returned
 * value.
 */
char * wpa_config_get(struct wpa_ssid *ssid, const char *var)
{
	size_t i;

	if (ssid == NULL || var == NULL)
		return NULL;

	for (i = 0; i < NUM_SSID_FIELDS; i++) {
		const struct parse_data *field = &ssid_fields[i];
		if (os_strcmp(var, field->name) == 0)
			return field->writer(field, ssid);
	}

	return NULL;
}


/**
 * wpa_config_get_no_key - Get a variable in network configuration (no keys)
 * @ssid: Pointer to network configuration data
 * @var: Variable name, e.g., "ssid"
 * Returns: Value of the variable or %NULL on failure
 *
 * This function can be used to get network configuration variable like
 * wpa_config_get(). The only difference is that this functions does not expose
 * key/password material from the configuration. In case a key/password field
 * is requested, the returned value is an empty string or %NULL if the variable
 * is not set or "*" if the variable is set (regardless of its value). The
 * returned value is a copy of the configuration variable in text format, i.e,.
 * the same format that the text-based configuration file and wpa_config_set()
 * are using for the value. The caller is responsible for freeing the returned
 * value.
 */
char * wpa_config_get_no_key(struct wpa_ssid *ssid, const char *var)
{
	size_t i;

	if (ssid == NULL || var == NULL)
		return NULL;

	for (i = 0; i < NUM_SSID_FIELDS; i++) {
		const struct parse_data *field = &ssid_fields[i];
		if (os_strcmp(var, field->name) == 0) {
			char *res = field->writer(field, ssid);
			if (field->key_data) {
				if (res && res[0]) {
					wpa_printf(MSG_DEBUG, "Do not allow "
						   "key_data field to be "
						   "exposed");
					os_free(res);
					return os_strdup("*");
				}

				os_free(res);
				return NULL;
			}
			return res;
		}
	}

	return NULL;
}
#endif /* NO_CONFIG_WRITE */


/**
 * wpa_config_update_psk - Update WPA PSK based on passphrase and SSID
 * @ssid: Pointer to network configuration data
 *
 * This function must be called to update WPA PSK when either SSID or the
 * passphrase has changed for the network configuration.
 */
void wpa_config_update_psk(struct wpa_ssid *ssid)
{
#ifndef CONFIG_NO_PBKDF2
	pbkdf2_sha1(ssid->passphrase,
		    (char *) ssid->ssid, ssid->ssid_len, 4096,
		    ssid->psk, PMK_LEN);
	wpa_hexdump_key(MSG_MSGDUMP, "PSK (from passphrase)",
			ssid->psk, PMK_LEN);
	ssid->psk_set = 1;
#endif /* CONFIG_NO_PBKDF2 */
}


#ifndef CONFIG_NO_CONFIG_BLOBS
/**
 * wpa_config_get_blob - Get a named configuration blob
 * @config: Configuration data from wpa_config_read()
 * @name: Name of the blob
 * Returns: Pointer to blob data or %NULL if not found
 */
const struct wpa_config_blob * wpa_config_get_blob(struct wpa_config *config,
						   const char *name)
{
	struct wpa_config_blob *blob = config->blobs;

	while (blob) {
		if (os_strcmp(blob->name, name) == 0)
			return blob;
		blob = blob->next;
	}
	return NULL;
}


/**
 * wpa_config_set_blob - Set or add a named configuration blob
 * @config: Configuration data from wpa_config_read()
 * @blob: New value for the blob
 *
 * Adds a new configuration blob or replaces the current value of an existing
 * blob.
 */
void wpa_config_set_blob(struct wpa_config *config,
			 struct wpa_config_blob *blob)
{
	wpa_config_remove_blob(config, blob->name);
	blob->next = config->blobs;
	config->blobs = blob;
}


/**
 * wpa_config_free_blob - Free blob data
 * @blob: Pointer to blob to be freed
 */
void wpa_config_free_blob(struct wpa_config_blob *blob)
{
	if (blob) {
		os_free(blob->name);
		os_free(blob->data);
		os_free(blob);
	}
}


/**
 * wpa_config_remove_blob - Remove a named configuration blob
 * @config: Configuration data from wpa_config_read()
 * @name: Name of the blob to remove
 * Returns: 0 if blob was removed or -1 if blob was not found
 */
int wpa_config_remove_blob(struct wpa_config *config, const char *name)
{
	struct wpa_config_blob *pos = config->blobs, *prev = NULL;

	while (pos) {
		if (os_strcmp(pos->name, name) == 0) {
			if (prev)
				prev->next = pos->next;
			else
				config->blobs = pos->next;
			wpa_config_free_blob(pos);
			return 0;
		}
		prev = pos;
		pos = pos->next;
	}

	return -1;
}
#endif /* CONFIG_NO_CONFIG_BLOBS */


/**
 * wpa_config_alloc_empty - Allocate an empty configuration
 * @ctrl_interface: Control interface parameters, e.g., path to UNIX domain
 * socket
 * @driver_param: Driver parameters
 * Returns: Pointer to allocated configuration data or %NULL on failure
 */
struct wpa_config * wpa_config_alloc_empty(const char *ctrl_interface,
					   const char *driver_param)
{
	struct wpa_config *config;

	config = os_zalloc(sizeof(*config));
	if (config == NULL)
		return NULL;
	config->eapol_version = DEFAULT_EAPOL_VERSION;
	config->ap_scan = DEFAULT_AP_SCAN;
	config->fast_reauth = DEFAULT_FAST_REAUTH;
	config->p2p_go_intent = DEFAULT_P2P_GO_INTENT;
	config->p2p_intra_bss = DEFAULT_P2P_INTRA_BSS;
	config->bss_max_count = DEFAULT_BSS_MAX_COUNT;
	config->bss_expiration_age = DEFAULT_BSS_EXPIRATION_AGE;
	config->bss_expiration_scan_count = DEFAULT_BSS_EXPIRATION_SCAN_COUNT;
	config->max_num_sta = DEFAULT_MAX_NUM_STA;
	config->access_network_type = DEFAULT_ACCESS_NETWORK_TYPE;

	if (ctrl_interface)
		config->ctrl_interface = os_strdup(ctrl_interface);
	if (driver_param)
		config->driver_param = os_strdup(driver_param);

	return config;
}


#ifndef CONFIG_NO_STDOUT_DEBUG
/**
 * wpa_config_debug_dump_networks - Debug dump of configured networks
 * @config: Configuration data from wpa_config_read()
 */
void wpa_config_debug_dump_networks(struct wpa_config *config)
{
	int prio;
	struct wpa_ssid *ssid;

	for (prio = 0; prio < config->num_prio; prio++) {
		ssid = config->pssid[prio];
		wpa_printf(MSG_DEBUG, "Priority group %d",
			   ssid->priority);
		while (ssid) {
			wpa_printf(MSG_DEBUG, "   id=%d ssid='%s'",
				   ssid->id,
				   wpa_ssid_txt(ssid->ssid, ssid->ssid_len));
			ssid = ssid->pnext;
		}
	}
}
#endif /* CONFIG_NO_STDOUT_DEBUG */


struct global_parse_data {
	char *name;
	int (*parser)(const struct global_parse_data *data,
		      struct wpa_config *config, int line, const char *value);
	void *param1, *param2, *param3;
	unsigned int changed_flag;
};


static int wpa_global_config_parse_int(const struct global_parse_data *data,
				       struct wpa_config *config, int line,
				       const char *pos)
{
	int *dst;
	dst = (int *) (((u8 *) config) + (long) data->param1);
	*dst = atoi(pos);
	wpa_printf(MSG_DEBUG, "%s=%d", data->name, *dst);

	if (data->param2 && *dst < (long) data->param2) {
		wpa_printf(MSG_ERROR, "Line %d: too small %s (value=%d "
			   "min_value=%ld)", line, data->name, *dst,
			   (long) data->param2);
		*dst = (long) data->param2;
		return -1;
	}

	if (data->param3 && *dst > (long) data->param3) {
		wpa_printf(MSG_ERROR, "Line %d: too large %s (value=%d "
			   "max_value=%ld)", line, data->name, *dst,
			   (long) data->param3);
		*dst = (long) data->param3;
		return -1;
	}

	return 0;
}


static int wpa_global_config_parse_str(const struct global_parse_data *data,
				       struct wpa_config *config, int line,
				       const char *pos)
{
	size_t len;
	char **dst, *tmp;

	len = os_strlen(pos);
	if (data->param2 && len < (size_t) data->param2) {
		wpa_printf(MSG_ERROR, "Line %d: too short %s (len=%lu "
			   "min_len=%ld)", line, data->name,
			   (unsigned long) len, (long) data->param2);
		return -1;
	}

	if (data->param3 && len > (size_t) data->param3) {
		wpa_printf(MSG_ERROR, "Line %d: too long %s (len=%lu "
			   "max_len=%ld)", line, data->name,
			   (unsigned long) len, (long) data->param3);
		return -1;
	}

	tmp = os_strdup(pos);
	if (tmp == NULL)
		return -1;

	dst = (char **) (((u8 *) config) + (long) data->param1);
	os_free(*dst);
	*dst = tmp;
	wpa_printf(MSG_DEBUG, "%s='%s'", data->name, *dst);

	return 0;
}


static int wpa_config_process_country(const struct global_parse_data *data,
				      struct wpa_config *config, int line,
				      const char *pos)
{
	if (!pos[0] || !pos[1]) {
		wpa_printf(MSG_DEBUG, "Invalid country set");
		return -1;
	}
	config->country[0] = pos[0];
	config->country[1] = pos[1];
	wpa_printf(MSG_DEBUG, "country='%c%c'",
		   config->country[0], config->country[1]);
	return 0;
}


static int wpa_config_process_load_dynamic_eap(
	const struct global_parse_data *data, struct wpa_config *config,
	int line, const char *so)
{
	int ret;
	wpa_printf(MSG_DEBUG, "load_dynamic_eap=%s", so);
	ret = eap_peer_method_load(so);
	if (ret == -2) {
		wpa_printf(MSG_DEBUG, "This EAP type was already loaded - not "
			   "reloading.");
	} else if (ret) {
		wpa_printf(MSG_ERROR, "Line %d: Failed to load dynamic EAP "
			   "method '%s'.", line, so);
		return -1;
	}

	return 0;
}


#ifdef CONFIG_WPS

static int wpa_config_process_uuid(const struct global_parse_data *data,
				   struct wpa_config *config, int line,
				   const char *pos)
{
	char buf[40];
	if (uuid_str2bin(pos, config->uuid)) {
		wpa_printf(MSG_ERROR, "Line %d: invalid UUID", line);
		return -1;
	}
	uuid_bin2str(config->uuid, buf, sizeof(buf));
	wpa_printf(MSG_DEBUG, "uuid=%s", buf);
	return 0;
}


static int wpa_config_process_device_type(
	const struct global_parse_data *data,
	struct wpa_config *config, int line, const char *pos)
{
	return wps_dev_type_str2bin(pos, config->device_type);
}


static int wpa_config_process_os_version(const struct global_parse_data *data,
					 struct wpa_config *config, int line,
					 const char *pos)
{
	if (hexstr2bin(pos, config->os_version, 4)) {
		wpa_printf(MSG_ERROR, "Line %d: invalid os_version", line);
		return -1;
	}
	wpa_printf(MSG_DEBUG, "os_version=%08x",
		   WPA_GET_BE32(config->os_version));
	return 0;
}

#endif /* CONFIG_WPS */

#ifdef CONFIG_P2P
static int wpa_config_process_sec_device_type(
	const struct global_parse_data *data,
	struct wpa_config *config, int line, const char *pos)
{
	int idx;

	if (config->num_sec_device_types >= MAX_SEC_DEVICE_TYPES) {
		wpa_printf(MSG_ERROR, "Line %d: too many sec_device_type "
			   "items", line);
		return -1;
	}

	idx = config->num_sec_device_types;

	if (wps_dev_type_str2bin(pos, config->sec_device_type[idx]))
		return -1;

	config->num_sec_device_types++;
	return 0;
}


static int wpa_config_process_p2p_pref_chan(
	const struct global_parse_data *data,
	struct wpa_config *config, int line, const char *pos)
{
	struct p2p_channel *pref = NULL, *n;
	unsigned int num = 0;
	const char *pos2;
	u8 op_class, chan;

	/* format: class:chan,class:chan,... */

	while (*pos) {
		op_class = atoi(pos);
		pos2 = os_strchr(pos, ':');
		if (pos2 == NULL)
			goto fail;
		pos2++;
		chan = atoi(pos2);

		n = os_realloc(pref, (num + 1) * sizeof(struct p2p_channel));
		if (n == NULL)
			goto fail;
		pref = n;
		pref[num].op_class = op_class;
		pref[num].chan = chan;
		num++;

		pos = os_strchr(pos2, ',');
		if (pos == NULL)
			break;
		pos++;
	}

	os_free(config->p2p_pref_chan);
	config->p2p_pref_chan = pref;
	config->num_p2p_pref_chan = num;
	wpa_hexdump(MSG_DEBUG, "P2P: Preferred class/channel pairs",
		    (u8 *) config->p2p_pref_chan,
		    config->num_p2p_pref_chan * sizeof(struct p2p_channel));

	return 0;

fail:
	os_free(pref);
	wpa_printf(MSG_ERROR, "Line %d: Invalid p2p_pref_chan list", line);
	return -1;
}
#endif /* CONFIG_P2P */


static int wpa_config_process_hessid(
	const struct global_parse_data *data,
	struct wpa_config *config, int line, const char *pos)
{
	if (hwaddr_aton2(pos, config->hessid) < 0) {
		wpa_printf(MSG_ERROR, "Line %d: Invalid hessid '%s'",
			   line, pos);
		return -1;
	}

	return 0;
}


#ifdef OFFSET
#undef OFFSET
#endif /* OFFSET */
/* OFFSET: Get offset of a variable within the wpa_config structure */
#define OFFSET(v) ((void *) &((struct wpa_config *) 0)->v)

#define FUNC(f) #f, wpa_config_process_ ## f, OFFSET(f), NULL, NULL
#define FUNC_NO_VAR(f) #f, wpa_config_process_ ## f, NULL, NULL, NULL
#define _INT(f) #f, wpa_global_config_parse_int, OFFSET(f)
#define INT(f) _INT(f), NULL, NULL
#define INT_RANGE(f, min, max) _INT(f), (void *) min, (void *) max
#define _STR(f) #f, wpa_global_config_parse_str, OFFSET(f)
#define STR(f) _STR(f), NULL, NULL
#define STR_RANGE(f, min, max) _STR(f), (void *) min, (void *) max

static const struct global_parse_data global_fields[] = {
#ifdef CONFIG_CTRL_IFACE
	{ STR(ctrl_interface), 0 },
	{ STR(ctrl_interface_group), 0 } /* deprecated */,
#endif /* CONFIG_CTRL_IFACE */
	{ INT_RANGE(eapol_version, 1, 2), 0 },
	{ INT(ap_scan), 0 },
	{ INT(fast_reauth), 0 },
	{ STR(opensc_engine_path), 0 },
	{ STR(pkcs11_engine_path), 0 },
	{ STR(pkcs11_module_path), 0 },
	{ STR(driver_param), 0 },
	{ INT(dot11RSNAConfigPMKLifetime), 0 },
	{ INT(dot11RSNAConfigPMKReauthThreshold), 0 },
	{ INT(dot11RSNAConfigSATimeout), 0 },
#ifndef CONFIG_NO_CONFIG_WRITE
	{ INT(update_config), 0 },
#endif /* CONFIG_NO_CONFIG_WRITE */
	{ FUNC_NO_VAR(load_dynamic_eap), 0 },
#ifdef CONFIG_WPS
	{ FUNC(uuid), CFG_CHANGED_UUID },
	{ STR_RANGE(device_name, 0, 32), CFG_CHANGED_DEVICE_NAME },
	{ STR_RANGE(manufacturer, 0, 64), CFG_CHANGED_WPS_STRING },
	{ STR_RANGE(model_name, 0, 32), CFG_CHANGED_WPS_STRING },
	{ STR_RANGE(model_number, 0, 32), CFG_CHANGED_WPS_STRING },
	{ STR_RANGE(serial_number, 0, 32), CFG_CHANGED_WPS_STRING },
	{ FUNC(device_type), CFG_CHANGED_DEVICE_TYPE },
	{ FUNC(os_version), CFG_CHANGED_OS_VERSION },
	{ STR(config_methods), CFG_CHANGED_CONFIG_METHODS },
	{ INT_RANGE(wps_cred_processing, 0, 2), 0 },
#endif /* CONFIG_WPS */
#ifdef CONFIG_P2P
	{ FUNC(sec_device_type), CFG_CHANGED_SEC_DEVICE_TYPE },
	{ INT(p2p_listen_reg_class), 0 },
	{ INT(p2p_listen_channel), 0 },
	{ INT(p2p_oper_reg_class), 0 },
	{ INT(p2p_oper_channel), 0 },
	{ INT_RANGE(p2p_go_intent, 0, 15), 0 },
	{ STR(p2p_ssid_postfix), CFG_CHANGED_P2P_SSID_POSTFIX },
	{ INT_RANGE(persistent_reconnect, 0, 1), 0 },
	{ INT_RANGE(p2p_intra_bss, 0, 1), CFG_CHANGED_P2P_INTRA_BSS },
	{ INT(p2p_group_idle), 0 },
	{ FUNC(p2p_pref_chan), CFG_CHANGED_P2P_PREF_CHAN },
#endif /* CONFIG_P2P */
	{ FUNC(country), CFG_CHANGED_COUNTRY },
	{ INT(bss_max_count), 0 },
	{ INT(bss_expiration_age), 0 },
	{ INT(bss_expiration_scan_count), 0 },
	{ INT_RANGE(filter_ssids, 0, 1), 0 },
	{ INT(max_num_sta), 0 },
	{ INT_RANGE(disassoc_low_ack, 0, 1), 0 },
	{ STR(home_realm), 0 },
	{ STR(home_username), 0 },
	{ STR(home_password), 0 },
	{ STR(home_ca_cert), 0 },
	{ STR(home_imsi), 0 },
	{ STR(home_milenage), 0 },
	{ INT_RANGE(interworking, 0, 1), 0 },
	{ FUNC(hessid), 0 },
	{ INT_RANGE(access_network_type, 0, 15), 0 }
};

#undef FUNC
#undef _INT
#undef INT
#undef INT_RANGE
#undef _STR
#undef STR
#undef STR_RANGE
#define NUM_GLOBAL_FIELDS (sizeof(global_fields) / sizeof(global_fields[0]))


int wpa_config_process_global(struct wpa_config *config, char *pos, int line)
{
	size_t i;
	int ret = 0;

	for (i = 0; i < NUM_GLOBAL_FIELDS; i++) {
		const struct global_parse_data *field = &global_fields[i];
		size_t flen = os_strlen(field->name);
		if (os_strncmp(pos, field->name, flen) != 0 ||
		    pos[flen] != '=')
			continue;

		if (field->parser(field, config, line, pos + flen + 1)) {
			wpa_printf(MSG_ERROR, "Line %d: failed to "
				   "parse '%s'.", line, pos);
			ret = -1;
		}
		config->changed_parameters |= field->changed_flag;
		break;
	}
	if (i == NUM_GLOBAL_FIELDS) {
		if (line < 0)
			return -1;
		wpa_printf(MSG_ERROR, "Line %d: unknown global field '%s'.",
			   line, pos);
		ret = -1;
	}

	return ret;
}
