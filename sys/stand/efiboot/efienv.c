/* $NetBSD: efienv.c,v 1.4 2019/04/21 22:30:41 thorpej Exp $ */

/*-
 * Copyright (c) 2019 Jason R. Thorpe
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "efiboot.h"
#include "efienv.h"

#define EFIBOOT_VENDOR_GUID \
	{ 0x97cde9bd, 0xac88, 0x4cf9, { 0x84, 0x86, 0x01, 0x33, 0x0f, 0xe1, 0x95, 0xd4 } }

static EFI_GUID EfibootVendorGuid = EFIBOOT_VENDOR_GUID;

void
efi_env_from_efibootplist(void)
{
	/*
	 * We support pre-loading the EFI environment from efiboot.plist
	 * using the following schema:
	 *
	 *	<key>environment-variables</key>
	 *	<dict>
	 *		<key>varname1</key>
	 *		<string>value_for_varname1</string>
	 *		<key>varname2</key>
	 *		<string>value_for_varname2</string>
	 *	</dict>
	 *
	 * Only string values are supported.
	 */
	prop_dictionary_t environment;
	prop_dictionary_keysym_t key;
	prop_string_t value;
	prop_object_iterator_t iter;

	const char *env_key;
	char *env_value;

	environment = prop_dictionary_get(efibootplist,
	    "environment-variables");
	if (environment == NULL)
		return;

	iter = prop_dictionary_iterator(environment);
	if (iter == NULL)
		goto failed;

	while ((key = prop_object_iterator_next(iter)) != NULL) {
		value = prop_dictionary_get_keysym(environment, key);
		if (value == NULL) {
			printf("boot: env: failed to get value for '%s'\n",
			    prop_dictionary_keysym_cstring_nocopy(key));
			continue;
		}
		if (prop_object_type(value) != PROP_TYPE_STRING) {
			printf("boot: env: value for '%s' is not a string\n",
			    prop_dictionary_keysym_cstring_nocopy(key));
			continue;
		}
		/* XXX __UNCONST because gnuefi */
		env_key = prop_dictionary_keysym_cstring_nocopy(key);;
		env_value = __UNCONST(prop_string_cstring_nocopy(value));;
#ifdef EFIBOOT_DEBUG
		printf(">> efiboot.plist env: '%s' = '%s'\n", env_key,
		    env_value);
#endif
		efi_env_set(env_key, env_value);
	}
	prop_object_iterator_release(iter);

	return;

 failed:
	printf("boot: failed to load environment from efiboot.plist");
}

void
efi_env_set(const char *key, char *val)
{
	EFI_STATUS status;
	CHAR16 *ukey;
	size_t len;

	ukey = NULL;
	utf8_to_ucs2(key, &ukey, &len);
	status = LibSetNVVariable(ukey, &EfibootVendorGuid, strlen(val) + 1, val);
	FreePool(ukey);

	if (EFI_ERROR(status))
		printf("env: failed to set variable '%s': %#lx\n", key, (u_long)status);
}

char *
efi_env_get(const char *key)
{
	CHAR16 *ukey;
	size_t len;
	char *ret;

	ukey = NULL;
	utf8_to_ucs2(key, &ukey, &len);
	ret = LibGetVariable(ukey, &EfibootVendorGuid);
	FreePool(ukey);

	return ret;
}

void
efi_env_clear(const char *key)
{
	CHAR16 *ukey;
	size_t len;

	ukey = NULL;
	utf8_to_ucs2(key, &ukey, &len);
	LibDeleteVariable(ukey, &EfibootVendorGuid);
	FreePool(ukey);
}

void
efi_env_reset(void)
{
	EFI_STATUS status;
	CHAR16 ukey[256];
	EFI_GUID vendor;
	UINTN size;

retry:
	ukey[0] = '\0';
	vendor = NullGuid;

	for (;;) {
		size = sizeof(ukey);
		status = uefi_call_wrapper(RT->GetNextVariableName, 3, &size, ukey, &vendor);
		if (status != EFI_SUCCESS)
			break;

		if (CompareGuid(&vendor, &EfibootVendorGuid) == 0) {
			LibDeleteVariable(ukey, &vendor);
			goto retry;
		}
	}
}

void
efi_env_print(void)
{
	EFI_STATUS status;
	CHAR16 ukey[256];
	EFI_GUID vendor;
	UINTN size;
	char *val;

	ukey[0] = '\0';
	vendor = NullGuid;

	for (;;) {
		size = sizeof(ukey);
		status = uefi_call_wrapper(RT->GetNextVariableName, 3, &size, ukey, &vendor);
		if (status != EFI_SUCCESS)
			break;

		if (CompareGuid(&vendor, &EfibootVendorGuid) != 0)
			continue;

		Print(L"\"%s\" = ", ukey);

		val = LibGetVariable(ukey, &vendor);
		printf("\"%s\"\n", val);
		FreePool(val);
	}
}
