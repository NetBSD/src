/*	$NetBSD$	*/

#include <security/pam_appl.h>
#include <security/openpam.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atf-c.h>

static int
unexpected_conversation(int nmsg, const struct pam_message **msg,
    struct pam_response **resp, void *arg)
{
	(void)nmsg;
	(void)msg;
	(void)resp;
	(void)arg;
	atf_tc_fail("unexpected PAM conversation");
}

static void
check_authentication(const char *module_config, int expected)
{
	const struct pam_conv conv = { unexpected_conversation, NULL };
	char policy[] = "./pam-policy.XXXXXX";
	pam_handle_t *pamh;
	FILE *fp;
	int fd, result, end_result;

	fd = mkstemp(policy);
	ATF_REQUIRE_MSG(fd != -1, "mkstemp: %s", strerror(errno));
	fp = fdopen(fd, "w");
	ATF_REQUIRE_MSG(fp != NULL, "fdopen: %s", strerror(errno));
	ATF_REQUIRE(fprintf(fp, "auth required %s\n", module_config) > 0);
	ATF_REQUIRE_EQ(0, fclose(fp));

	/* Load only the policy created in this test's working directory. */
	ATF_REQUIRE_EQ(PAM_SUCCESS,
	    openpam_set_feature(OPENPAM_RESTRICT_SERVICE_NAME, 0));
	ATF_REQUIRE_EQ(PAM_SUCCESS,
	    openpam_set_feature(OPENPAM_FALLBACK_TO_OTHER, 0));
	result = pam_start(policy, "pam-test-user", &conv, &pamh);
	ATF_REQUIRE_EQ_MSG(PAM_SUCCESS, result, "pam_start for %s: %s",
	    module_config, pam_strerror(NULL, result));

	result = pam_authenticate(pamh, 0);
	end_result = pam_end(pamh, result);
	ATF_CHECK_EQ_MSG(expected, result, "pam_authenticate for %s: %s",
	    module_config, pam_strerror(NULL, result));
	ATF_CHECK_EQ(PAM_SUCCESS, end_result);
}

ATF_TC_WITHOUT_HEAD(permit);
ATF_TC_BODY(permit, tc)
{
	check_authentication("pam_permit", PAM_SUCCESS);
}

ATF_TC_WITHOUT_HEAD(deny);
ATF_TC_BODY(deny, tc)
{
	check_authentication("pam_deny", PAM_AUTH_ERR);
}

ATF_TC_WITHOUT_HEAD(exec_success);
ATF_TC_BODY(exec_success, tc)
{
	check_authentication("pam_exec /usr/bin/true", PAM_SUCCESS);
}

ATF_TC_WITHOUT_HEAD(exec_failure);
ATF_TC_BODY(exec_failure, tc)
{
	check_authentication("pam_exec /usr/bin/false", PAM_SYSTEM_ERR);
}

#ifdef HAVE_PAM_KRB5
ATF_TC_WITHOUT_HEAD(krb5_missing_keytab);
ATF_TC_BODY(krb5_missing_keytab, tc)
{
	static const char config[] =
	    "[libdefaults]\n"
	    "    default_realm = PAM.TEST\n"
	    "    dns_lookup_realm = false\n"
	    "    dns_lookup_kdc = false\n"
	    "    dns_canonicalize_hostname = false\n";
	FILE *fp;

	fp = fopen("krb5.conf", "w");
	ATF_REQUIRE_MSG(fp != NULL, "fopen krb5.conf: %s", strerror(errno));
	ATF_REQUIRE(fputs(config, fp) != EOF);
	ATF_REQUIRE_EQ(0, fclose(fp));
	ATF_REQUIRE_EQ(0, setenv("KRB5_CONFIG", "./krb5.conf", 1));
	ATF_REQUIRE_EQ(0, setenv("KRB5_KTNAME", "FILE:./missing.keytab", 1));

	/* Reject authentication before prompting or contacting a KDC. */
	check_authentication("pam_krb5 no_warn", PAM_SERVICE_ERR);
}
#endif

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, permit);
	ATF_TP_ADD_TC(tp, deny);
	ATF_TP_ADD_TC(tp, exec_success);
	ATF_TP_ADD_TC(tp, exec_failure);
#ifdef HAVE_PAM_KRB5
	ATF_TP_ADD_TC(tp, krb5_missing_keytab);
#endif

	return atf_no_error();
}
