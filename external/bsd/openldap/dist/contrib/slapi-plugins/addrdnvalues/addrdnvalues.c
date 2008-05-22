/* $OpenLDAP: pkg/ldap/contrib/slapi-plugins/addrdnvalues/addrdnvalues.c,v 1.6 2004/05/23 13:45:32 lukeh Exp $ */
/*
 * Copyright 2003-2004 PADL Software Pty Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include <string.h>
#include <unistd.h>

#include <ldap.h>
#include <lber.h>

#include <slapi-plugin.h>

int addrdnvalues_preop_init(Slapi_PBlock *pb);

static Slapi_PluginDesc pluginDescription = {
	"addrdnvalues-plugin",
	"PADL",
	"1.0",
	"RDN values addition plugin"
};

static int addrdnvalues_preop_add(Slapi_PBlock *pb)
{
	int rc;
	Slapi_Entry *e;

	if (slapi_pblock_get(pb, SLAPI_ADD_ENTRY, &e) != 0) {
		slapi_log_error(SLAPI_LOG_PLUGIN, "addrdnvalues_preop_add",
				"Error retrieving target entry\n");
		return -1;
	}

	rc = slapi_entry_add_rdn_values(e);
	if (rc != LDAP_SUCCESS) {
		slapi_send_ldap_result(pb, LDAP_OTHER, NULL,
			"Failed to parse distinguished name", 0, NULL);
		slapi_log_error(SLAPI_LOG_PLUGIN, "addrdnvalues_preop_add",
			"Failed to parse distinguished name: %s\n",
			ldap_err2string(rc));
		return -1;
	}

	return 0;
}

int addrdnvalues_preop_init(Slapi_PBlock *pb)
{
	if (slapi_pblock_set(pb, SLAPI_PLUGIN_VERSION, SLAPI_PLUGIN_VERSION_03) != 0 ||
	    slapi_pblock_set(pb, SLAPI_PLUGIN_DESCRIPTION, &pluginDescription) != 0 ||
	    slapi_pblock_set(pb, SLAPI_PLUGIN_PRE_ADD_FN, (void *)addrdnvalues_preop_add) != 0) {
		slapi_log_error(SLAPI_LOG_PLUGIN, "addrdnvalues_preop_init",
				"Error registering %s\n", pluginDescription.spd_description);
		return -1;
	}

	return 0;
}

