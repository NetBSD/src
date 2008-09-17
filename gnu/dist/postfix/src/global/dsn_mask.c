/*++
/* NAME
/*	dsn_mask 3
/* SUMMARY
/*	DSN embedding in SMTP
/* SYNOPSIS
/*	#include <dsn_mask.h>
/*
/*	int	dsn_notify_mask(str)
/*	const char *str;
/*
/*	const char *dsn_notify_str(mask)
/*	int	mask;
/*
/*	int	dsn_ret_code(str)
/*	const char *str;
/*
/*	const char *dsn_ret_str(code)
/*	int	mask;
/* DESCRIPTION
/*	dsn_ret_code() converts the parameters of a MAIL FROM ..
/*	RET option to internal form.
/*
/*	dsn_ret_str() converts internal form to the representation
/*	used in the MAIL FROM .. RET command. The result is in
/*	stable and static memory.
/*
/*	dsn_notify_mask() converts the parameters of a RCPT TO ..
/*	NOTIFY option to internal form.
/*
/*	dsn_notify_str() converts internal form to the representation
/*	used in the MAIL FROM .. NOTIFY command. The result is in
/*	volatile memory and is clobbered whenever str_name_mask()
/*	is called.
/*
/*	Arguments:
/* .IP str
/*	Information received with the MAIL FROM or RCPT TO command.
/* .IP mask
/*	Internal representation.
/* DIAGNOSTICS
/*	dsn_ret_code() and dsn_notify_mask() return 0 when the string
/*	specifies an invalid request.
/*
/*	dsn_ret_str() and dsn_notify_str() abort on failure.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/


/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <name_code.h>
#include <name_mask.h>
#include <msg.h>

/* Global library. */

#include <dsn_mask.h>

/* Application-specific. */

static const NAME_MASK dsn_notify_table[] = {
    "NEVER", DSN_NOTIFY_NEVER,
    "SUCCESS", DSN_NOTIFY_SUCCESS,
    "FAILURE", DSN_NOTIFY_FAILURE,
    "DELAY", DSN_NOTIFY_DELAY,
    0, 0,
};

static const NAME_CODE dsn_ret_table[] = {
    "FULL", DSN_RET_FULL,
    "HDRS", DSN_RET_HDRS,
    0, 0,
};

/* dsn_ret_code - string to mask */

int     dsn_ret_code(const char *str)
{
    return (name_code(dsn_ret_table, NAME_CODE_FLAG_NONE, str));
}

/* dsn_ret_str - mask to string */

const char *dsn_ret_str(int code)
{
    const char *cp ;

    if ((cp = str_name_code(dsn_ret_table, code)) == 0)
	msg_panic("dsn_ret_str: unknown code %d", code);
    return (cp);
}

/* dsn_notify_mask - string to mask */

int     dsn_notify_mask(const char *str)
{
    int     mask = name_mask_opt("DSN NOTIFY command", dsn_notify_table,
				 str, NAME_MASK_ANY_CASE | NAME_MASK_RETURN);

    return (DSN_NOTIFY_OK(mask) ? mask : 0);
}

/* dsn_notify_str - mask to string */

const char *dsn_notify_str(int mask)
{
    return (str_name_mask_opt((VSTRING *) 0, "DSN NOTIFY command",
			      dsn_notify_table, mask,
			      NAME_MASK_FATAL | NAME_MASK_COMMA));
}
