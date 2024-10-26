/*
 * Copyright (c) 2022 Apple Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DNS_OBJ_LOG_H
#define DNS_OBJ_LOG_H

// Overrides the default assert print.
#ifndef DEBUG_ASSERT_MESSAGE
	#define DEBUG_ASSERT_MESSAGE(name, assertion, label, message, file, line, value) \
		log_fault("AssertMacros: " PUB_S ", " PUB_S " file: " PUB_S ", line: %d, value: %ld", assertion, (message != 0) ? message : "", file, line, (long) (value))
#endif

//======================================================================================================================
// MARK: - Headers

#include "mDNSDebug.h"
#include "domain_name_labels.h"
#include "dns_obj_domain_name.h"

//======================================================================================================================
// MARK: - Macros

// Short version logging for DNS object.

#ifndef log_debug
	#define log_debug(FORMAT, ...)		LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEBUG, FORMAT, ## __VA_ARGS__)
#endif

#ifndef log_info
	#define log_info(FORMAT, ...)		LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_INFO, FORMAT, ## __VA_ARGS__)
#endif

#ifndef log_default
	#define log_default(FORMAT, ...)	LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_DEFAULT, FORMAT, ## __VA_ARGS__)
#endif

#ifndef log_error
	#define log_error(FORMAT, ...)		LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_ERROR, FORMAT, ## __VA_ARGS__)
#endif

#ifndef log_fault
	#define log_fault(FORMAT, ...)		LogRedact(MDNS_LOG_CATEGORY_DEFAULT, MDNS_LOG_FAULT, FORMAT, ## __VA_ARGS__)
#endif


// Log specifier for domain name labels in bytes.
#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
	#define PUB_NAME_LABELS				PUB_DM_NAME
	#define PRI_NAME_LABELS				PRI_DM_NAME
	#define NAME_LABELS_PARAM(labels)	((labels != NULL) ? (int)domain_name_labels_length(labels) : 0), (labels)
#else
	#define PUB_NAME_LABELS				PUB_DM_NAME
	#define PRI_NAME_LABELS				PUB_NAME_LABELS
	#define NAME_LABELS_PARAM(labels)	(labels)
#endif

// Log specifier for domain name object.
#if MDNSRESPONDER_SUPPORTS(APPLE, OS_LOG)
	#define PUB_DNS_DM_NAME				PUB_DM_NAME
	#define PRI_DNS_DM_NAME				PRI_DM_NAME
	#define DNS_DM_NAME_PARAM(NAME)		(((NAME) != NULL) ? (int)dns_obj_domain_name_get_length(NAME) : 0), (((NAME) != NULL) ? dns_obj_domain_name_get_labels(NAME) : NULL)
#else
	#define PUB_DNS_DM_NAME				PUB_DM_NAME
	#define PRI_DNS_DM_NAME				PUB_DNS_DM_NAME
	#define DNS_DM_NAME_PARAM(NAME)		(((NAME) != NULL) ? dns_obj_domain_name_get_labels(NAME) : NULL)
#endif

#endif // DNS_OBJ_LOG_H
