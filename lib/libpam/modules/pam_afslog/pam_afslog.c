/*	$NetBSD: pam_afslog.c,v 1.2 2006/01/20 16:51:15 christos Exp $	*/

/*-
 * Copyright 2005 Tyler C. Sarna <tsarna@netbsd.org>
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tyler C. Sarna
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__RCSID("$NetBSD: pam_afslog.c,v 1.2 2006/01/20 16:51:15 christos Exp $");

#include <krb5/krb5.h>
#include <krb5/kafs.h>

#define PAM_SM_AUTH
#define PAM_SM_CRED
#include <security/pam_appl.h>
#include <security/pam_modules.h>
#include <security/pam_mod_misc.h>

PAM_EXTERN int
pam_sm_authenticate(pam_handle_t *pamh, int flags __unused,
    int argc __unused, const char *argv[] __unused)
{
	return PAM_IGNORE;
}

PAM_EXTERN int
pam_sm_setcred(pam_handle_t *pamh, int flags,
    int argc __unused, const char *argv[] __unused)
{
	krb5_context ctx;
	krb5_ccache ccache;
	krb5_principal principal;
	krb5_error_code kret;
	const void *service = NULL;
	const char *ccname = NULL;
	int do_afslog = 0, ret = PAM_SUCCESS;
					        
	pam_get_item(pamh, PAM_SERVICE, &service);
	if (service == NULL)
		service = "pam_afslog";

	kret = krb5_init_context(&ctx);
	if (kret != 0) {
		PAM_LOG("Error: krb5_init_context() failed");
		ret = PAM_SERVICE_ERR;
	} else {
		ccname = pam_getenv(pamh, "KRB5CCNAME");
		if (ccname)
			kret = krb5_cc_resolve(ctx, ccname, &ccache);
		else
			kret = krb5_cc_default(ctx, &ccache);
		if (kret != 0) {
			PAM_LOG("Error: failed to open ccache");
			ret = PAM_SERVICE_ERR;
		} else {
			kret = krb5_cc_get_principal(ctx, ccache, &principal);
			if (kret != 0) {
				PAM_LOG("Error: krb5_cc_get_principal() failed");
				ret = PAM_SERVICE_ERR;
			} else {
				krb5_appdefault_boolean(ctx,
					(const char *)service,
					krb5_principal_get_realm(
						ctx, principal),
					"afslog", FALSE, &do_afslog);

				/* silently bail if not enabled */
	
				if (do_afslog && k_hasafs()) {
					switch (flags & ~PAM_SILENT) {
					case 0:
					case PAM_ESTABLISH_CRED:
						k_setpag();
				
						/* FALLTHROUGH */
	
					case PAM_REINITIALIZE_CRED:
					case PAM_REFRESH_CRED:
						krb5_afslog(ctx, ccache,
							NULL, NULL);
						break;

					case PAM_DELETE_CRED:
						k_unlog();
						break;
					}
				}

				krb5_free_principal(ctx, principal);
			}

			krb5_cc_close(ctx, ccache);
		}

		krb5_free_context(ctx);
	}
	
	return ret;
}

PAM_MODULE_ENTRY("pam_afslog");
