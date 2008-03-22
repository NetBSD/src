/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <popper.h>
__RCSID("$Heimdal: pop_user.c 15289 2005-05-29 21:44:30Z lha $"
        "$NetBSD: pop_user.c,v 1.2 2008/03/22 08:36:55 mlelstv Exp $");

/* 
 *  user:   Prompt for the user name at the start of a POP session
 */

int
pop_user (POP *p)
{
    strlcpy(p->user, p->pop_parm[1], sizeof(p->user));

    if (p->auth_level == AUTH_OTP) {
#ifdef OTP
	char ss[256], *s;

	if(otp_challenge (&p->otp_ctx, p->user, ss, sizeof(ss)) == 0)
	    return pop_msg(p, POP_SUCCESS, "Password %s required for %s.",
			   ss, p->user);
	s = otp_error(&p->otp_ctx);
	return pop_msg(p, POP_FAILURE, "Permission denied%s%s",
		       s ? ":" : "", s ? s : "");
#endif
    }
    if (p->auth_level == AUTH_SASL) {
	return pop_msg(p, POP_FAILURE, "Permission denied");
    }
    return pop_msg(p, POP_SUCCESS, "Password required for %s.", p->user);
}
