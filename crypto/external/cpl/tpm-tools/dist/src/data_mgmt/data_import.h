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

#ifndef __DATA_IMPORT_H
#define __DATA_IMPORT_H

#define TOKEN_ID_X509_CERT	_("X509 Public Key Certificate")
#define TOKEN_ID_RSA_PUBKEY	_("RSA Public Key")
#define TOKEN_ID_RSA_KEY	_("RSA Public/Private Key")

#define TOKEN_ID_MISSING_PROMPT	_("The subject name and key identifier can not be obtained.\n" \
				"Certificate to key association may not be possible after " \
				"the import is complete.  If the key does not correspond " \
				"to a certficate or the key can be associated with the " \
				"certificate in another way this may not be an issue.\n" \
				"Import the object? [y/N]: ")
#define TOKEN_ID_PROMPT		_("One or more %s objects matching the subject name and key " \
				"identifier already exist.  Importing this object will replace " \
				"all of these matching objects.\n" \
				"Import the object? [y/N]: ")
#define TOKEN_ID_YES		_("y")
#define TOKEN_ID_NO		_("n")

#define TOKEN_FILE_ERROR	_("Error, an import file must be specified\n")
#define TOKEN_RSA_KEY_ERROR	_("Error, the X509 certificate does not contain an RSA key\n")
#define TOKEN_OBJECT_ERROR	_("Error, no objects were found that could be imported\n")
#define TOKEN_ID_ERROR		_("Error, unable to obtain the required subject and id attributes\n")

#endif
