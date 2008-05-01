/*
 * Automated Testing Framework (atf)
 *
 * Copyright (c) 2007, 2008 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>

#include <atf-c.h>

#include "atf-c/config.h"
#include "atf-c/env.h"

/* ---------------------------------------------------------------------
 * Auxiliary functions.
 * --------------------------------------------------------------------- */

#define CE(stm) ATF_CHECK(!atf_is_error(stm))

void __atf_config_reinit(void);

/* ---------------------------------------------------------------------
 * Test cases for the free functions.
 * --------------------------------------------------------------------- */

ATF_TC(get);
ATF_TC_HEAD(get, tc)
{
    atf_tc_set_md_var(tc, "descr", "Tests the atf_config_get function");
}
ATF_TC_BODY(get, tc)
{
    /* Unset all known environment variables and make sure the built-in
     * values do not match the bogus value we will use for testing. */
    CE(atf_env_unset("ATF_ARCH"));
    CE(atf_env_unset("ATF_CONFDIR"));
    CE(atf_env_unset("ATF_LIBEXECDIR"));
    CE(atf_env_unset("ATF_MACHINE"));
    CE(atf_env_unset("ATF_PKGDATADIR"));
    CE(atf_env_unset("ATF_SHELL"));
    CE(atf_env_unset("ATF_WORKDIR"));
    __atf_config_reinit();
    ATF_CHECK(strcmp(atf_config_get("atf_arch"), "env-value") != 0);
    ATF_CHECK(strcmp(atf_config_get("atf_confdir"), "env-value") != 0);
    ATF_CHECK(strcmp(atf_config_get("atf_libexecdir"), "env-value") != 0);
    ATF_CHECK(strcmp(atf_config_get("atf_machine"), "env-value") != 0);
    ATF_CHECK(strcmp(atf_config_get("atf_pkgdatadir"), "env-value") != 0);
    ATF_CHECK(strcmp(atf_config_get("atf_shell"), "env-value") != 0);
    ATF_CHECK(strcmp(atf_config_get("atf_workdir"), "env-value") != 0);

    /* Make sure empty values in the environment are not considered. */
    atf_env_set("ATF_ARCH", "");
    atf_env_set("ATF_CONFDIR", "");
    atf_env_set("ATF_LIBEXECDIR", "");
    atf_env_set("ATF_MACHINE", "");
    atf_env_set("ATF_PKGDATADIR", "");
    atf_env_set("ATF_SHELL", "");
    atf_env_set("ATF_WORKDIR", "");
    __atf_config_reinit();
    ATF_CHECK(strlen(atf_config_get("atf_arch")) > 0);
    ATF_CHECK(strlen(atf_config_get("atf_confdir")) > 0);
    ATF_CHECK(strlen(atf_config_get("atf_libexecdir")) > 0);
    ATF_CHECK(strlen(atf_config_get("atf_machine")) > 0);
    ATF_CHECK(strlen(atf_config_get("atf_pkgdatadir")) > 0);
    ATF_CHECK(strlen(atf_config_get("atf_shell")) > 0);
    ATF_CHECK(strlen(atf_config_get("atf_workdir")) > 0);

    /* Check if the ATF_ARCH variable is recognized. */
    atf_env_set     ("ATF_ARCH", "env-value");
    CE(atf_env_unset("ATF_LIBEXECDIR"));
    CE(atf_env_unset("ATF_MACHINE"));
    CE(atf_env_unset("ATF_PKGDATADIR"));
    CE(atf_env_unset("ATF_SHELL"));
    CE(atf_env_unset("ATF_WORKDIR"));
    __atf_config_reinit();
    ATF_CHECK(strcmp(atf_config_get("atf_arch"), "env-value") == 0);
    ATF_CHECK(strcmp(atf_config_get("atf_confdir"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_libexecdir"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_machine"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_pkgdatadir"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_shell"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_workdir"), "env-value") != 0);

    /* Check if the ATF_CONFDIR variable is recognized. */
    CE(atf_env_unset("ATF_ARCH"));
    atf_env_set     ("ATF_CONFDIR", "env-value");
    CE(atf_env_unset("ATF_LIBEXECDIR"));
    CE(atf_env_unset("ATF_MACHINE"));
    CE(atf_env_unset("ATF_PKGDATADIR"));
    CE(atf_env_unset("ATF_SHELL"));
    CE(atf_env_unset("ATF_WORKDIR"));
    __atf_config_reinit();
    ATF_CHECK(strcmp(("atf_arch"), "env-value") != 0);
    ATF_CHECK(strcmp(atf_config_get("atf_confdir"), "env-value") == 0);
    ATF_CHECK(strcmp(("atf_libexecdir"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_machine"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_pkgdatadir"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_shell"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_workdir"), "env-value") != 0);

    /* Check if the ATF_LIBEXECDIR variable is recognized. */
    CE(atf_env_unset("ATF_ARCH"));
    CE(atf_env_unset("ATF_CONFDIR"));
    atf_env_set     ("ATF_LIBEXECDIR", "env-value");
    CE(atf_env_unset("ATF_MACHINE"));
    CE(atf_env_unset("ATF_PKGDATADIR"));
    CE(atf_env_unset("ATF_SHELL"));
    CE(atf_env_unset("ATF_WORKDIR"));
    __atf_config_reinit();
    ATF_CHECK(strcmp(("atf_arch"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_confdir"), "env-value") != 0);
    ATF_CHECK(strcmp(atf_config_get("atf_libexecdir"), "env-value") == 0);
    ATF_CHECK(strcmp(("atf_machine"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_pkgdatadir"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_shell"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_workdir"), "env-value") != 0);

    /* Check if the ATF_MACHINE variable is recognized. */
    CE(atf_env_unset("ATF_ARCH"));
    CE(atf_env_unset("ATF_CONFDIR"));
    CE(atf_env_unset("ATF_LIBEXECDIR"));
    atf_env_set     ("ATF_MACHINE", "env-value");
    CE(atf_env_unset("ATF_PKGDATADIR"));
    CE(atf_env_unset("ATF_SHELL"));
    CE(atf_env_unset("ATF_WORKDIR"));
    __atf_config_reinit();
    ATF_CHECK(strcmp(("atf_arch"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_confdir"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_libexecdir"), "env-value") != 0);
    ATF_CHECK(strcmp(atf_config_get("atf_machine"), "env-value") == 0);
    ATF_CHECK(strcmp(("atf_pkgdatadir"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_shell"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_workdir"), "env-value") != 0);

    /* Check if the ATF_PKGDATADIR variable is recognized. */
    CE(atf_env_unset("ATF_ARCH"));
    CE(atf_env_unset("ATF_CONFDIR"));
    CE(atf_env_unset("ATF_LIBEXECDIR"));
    CE(atf_env_unset("ATF_MACHINE"));
    atf_env_set     ("ATF_PKGDATADIR", "env-value");
    CE(atf_env_unset("ATF_SHELL"));
    CE(atf_env_unset("ATF_WORKDIR"));
    __atf_config_reinit();
    ATF_CHECK(strcmp(("atf_arch"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_confdir"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_libexecdir"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_machine"), "env-value") != 0);
    ATF_CHECK(strcmp(atf_config_get("atf_pkgdatadir"), "env-value") == 0);
    ATF_CHECK(strcmp(("atf_shell"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_workdir"), "env-value") != 0);

    /* Check if the ATF_SHELL variable is recognized. */
    CE(atf_env_unset("ATF_ARCH"));
    CE(atf_env_unset("ATF_CONFDIR"));
    CE(atf_env_unset("ATF_LIBEXECDIR"));
    CE(atf_env_unset("ATF_MACHINE"));
    CE(atf_env_unset("ATF_PKGDATADIR"));
    atf_env_set     ("ATF_SHELL", "env-value");
    CE(atf_env_unset("ATF_WORKDIR"));
    __atf_config_reinit();
    ATF_CHECK(strcmp(("atf_arch"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_confdir"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_libexecdir"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_machine"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_pkgdatadir"), "env-value") != 0);
    ATF_CHECK(strcmp(atf_config_get("atf_shell"), "env-value") == 0);
    ATF_CHECK(strcmp(("atf_workdir"), "env-value") != 0);

    /* Check if the ATF_WORKDIR variable is recognized. */
    CE(atf_env_unset("ATF_ARCH"));
    CE(atf_env_unset("ATF_CONFDIR"));
    CE(atf_env_unset("ATF_LIBEXECDIR"));
    CE(atf_env_unset("ATF_MACHINE"));
    CE(atf_env_unset("ATF_PKGDATADIR"));
    CE(atf_env_unset("ATF_SHELL"));
    atf_env_set     ("ATF_WORKDIR", "env-value");
    __atf_config_reinit();
    ATF_CHECK(strcmp(("atf_arch"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_confdir"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_libexecdir"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_machine"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_pkgdatadir"), "env-value") != 0);
    ATF_CHECK(strcmp(("atf_shell"), "env-value") != 0);
    ATF_CHECK(strcmp(atf_config_get("atf_workdir"), "env-value") == 0);
}

/* ---------------------------------------------------------------------
 * Main.
 * --------------------------------------------------------------------- */

ATF_TP_ADD_TCS(tp)
{
    ATF_TP_ADD_TC(tp, get);

    return atf_no_error();
}
