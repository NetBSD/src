/*
 * The Initial Developer of the Original Code is International
 * Business Machines Corporation. Portions created by IBM
 * Corporation are Copyright (C) 2005 International Business
 * Machines Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the Common Public License as published by
 * IBM Corporation; either version 1 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Common Public License for more details.
 *
 * You should have received a copy of the Common Public License
 * along with this program; if not, a copy can be viewed at
 * http://www.opensource.org/licenses/cpl1.0.php.
 */

#ifndef __DATA_COMMON_H
#define __DATA_COMMON_H

#define TOKEN_OBJECT_KEY		"key"
#define TOKEN_OBJECT_CERT		"cert"

#define TOKEN_SO_INIT_PIN		"87654321"
#define TOKEN_SO_PIN_PROMPT		_("Enter the TPM security officer password: ")
#define TOKEN_SO_NEW_PIN_PROMPT		_("A new TPM security officer password is needed. " \
					"The password must be between %d and %d characters " \
					"in length.\n" \
					"Enter new password: ")

#define TOKEN_USER_INIT_PIN		"12345678"
#define TOKEN_USER_PIN_PROMPT		_("Enter your TPM user password: ")
#define TOKEN_USER_NEW_PIN_PROMPT	_("A new TPM user password is needed. " \
					"The password must be between %d and %d characters " \
					"in length.\n" \
					"Enter new password: ")

#define TOKEN_INVALID_PIN		_("The password entered is not valid, please try again.\n")

#define TOKEN_PROTECT_KEY_LABEL		"User Data Protection Key"

#define TOKEN_NOT_INIT_ERROR		_("Error, the TPM token has not been initialized\n")
#define TOKEN_MEMORY_ERROR		_("Error, unable to allocate needed memory\n")
#define TOKEN_OPENSSL_ERROR		_("Error, OpenSSL error: %s\n")
#define TOKEN_FILE_OPEN_ERROR		_("Error, unable to open file %s: %s\n")
#define TOKEN_FILE_WRITE_ERROR		_("Error writing to file %s: %s\n")

#define TOKEN_CMD_SUCCESS		_("%s succeeded\n")
#define TOKEN_CMD_FAILED		_("%s failed\n")

#endif
