/*	$NetBSD: driver.c,v 1.1.1.1 2018/08/12 12:07:38 christos Exp $	*/

/*
 * Driver API implementation and main entry point for BIND.
 *
 * BIND calls dyndb_version() before loading, dyndb_init() during startup
 * and dyndb_destroy() during shutdown.
 *
 * It is completely up to implementation what to do.
 *
 * dyndb <name> <driver> {} sections in named.conf are independent so
 * driver init() and destroy() functions are called independently for
 * each section even if they reference the same driver/library. It is
 * up to driver implementation to detect and catch this situation if
 * it is undesirable.
 *
 * Copyright (C) 2009-2015  Red Hat ; see COPYRIGHT for license
 */

#include <config.h>

#include <isc/commandline.h>
#include <isc/hash.h>
#include <isc/mem.h>
#include <isc/lib.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/dyndb.h>
#include <dns/lib.h>
#include <dns/types.h>

#include "db.h"
#include "log.h"
#include "instance.h"
#include "util.h"

dns_dyndb_destroy_t dyndb_destroy;
dns_dyndb_register_t dyndb_init;
dns_dyndb_version_t dyndb_version;

/*
 * Driver init is called for each dyndb section in named.conf
 * once during startup and then again on every reload.
 *
 * @code
 * dyndb example-name "sample.so" { param1 param2 };
 * @endcode
 *
 * @param[in] name        User-defined string from dyndb "name" {}; definition
 *                        in named.conf.
 *                        The example above will have name = "example-name".
 * @param[in] parameters  User-defined parameters from dyndb section as one
 *                        string. The example above will have
 *                        params = "param1 param2";
 * @param[in] file	  The name of the file from which the parameters
 *                        were read.
 * @param[in] line	  The line number from which the parameters were read.
 * @param[out] instp      Pointer to instance-specific data
 *                        (for one dyndb section).
 */
isc_result_t
dyndb_init(isc_mem_t *mctx, const char *name, const char *parameters,
	   const char *file, unsigned long line,
	   const dns_dyndbctx_t *dctx, void **instp)
{
	isc_result_t result;
	unsigned int argc;
	char **argv = NULL;
	char *s = NULL;
	sample_instance_t *sample_inst = NULL;

	REQUIRE(name != NULL);
	REQUIRE(dctx != NULL);

	/*
	 * Depending on how dlopen() was called, we may not have
	 * access to named's global namespace, in which case we need
	 * to initialize libisc/libdns
	 */
	if (dctx->refvar != &isc_bind9) {
		isc_lib_register();
		isc_log_setcontext(dctx->lctx);
		dns_log_setcontext(dctx->lctx);
	}

	isc_hash_set_initializer(dctx->hashinit);

	s = isc_mem_strdup(mctx, parameters);
	if (s == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}

	result = isc_commandline_strtoargv(mctx, s, &argc, &argv, 0);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	log_write(ISC_LOG_DEBUG(9),
		  "loading params for dyndb '%s' from %s:%lu",
		  name, file, line);

	/* Finally, create the instance. */
	CHECK(new_sample_instance(mctx, name, argc, argv, dctx, &sample_inst));

	/*
	 * This is an example so we create and load zones
	 * right now.  This step can be arbitrarily postponed.
	 */
	CHECK(load_sample_instance_zones(sample_inst));

	*instp = sample_inst;

 cleanup:
	if (s != NULL)
		isc_mem_free(mctx, s);
	if (argv != NULL)
		isc_mem_put(mctx, argv, argc * sizeof(*argv));

	return (result);
}

/*
 * Driver destroy is called for every instance on every reload and then once
 * during shutdown.
 *
 * @param[out] instp Pointer to instance-specific data (for one dyndb section).
 */
void
dyndb_destroy(void **instp) {
	destroy_sample_instance((sample_instance_t **)instp);
}

/*
 * Driver version is called when loading the driver to ensure there
 * is no API mismatch betwen the driver and the caller.
 */
int
dyndb_version(unsigned int *flags) {
	UNUSED(flags);

	return (DNS_DYNDB_VERSION);
}
