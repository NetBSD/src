/*++
/* NAME
/*	get_port 3
/* SUMMARY
/*	trivial host and port extracter
/* SYNOPSIS
/*	#include <get_port.h>
/*
/*	char	*get_port(data)
/*	char	*data;
/*
/* DESCRIPTION
/* 	get_port() extract host name or ip address from
/* 	strings such as [3ffe:902:12::10]:25, [::1]
/* 	or 192.168.0.1:25, and null-terminates the
/* 	\fIdata\fR at the first occurrence of port separator.
/* DIAGNOSTICS
/* 	If port not found return null pointer.
/* LICENSE
/* .ad
/* .fi
/*	BSD Style (or BSD like) license.
/* AUTHOR(S)
/*	Arkadiusz Mi¶kiewicz <misiek@pld.org.pl>
/*	Wroclaw, POLAND
/*--*/

/* System libraries */

#include <sys_defs.h>
#include <string.h>

/* Utility library. */

#include "get_port.h"

/* get_port - extract port number from string */

char *get_port(char *data)
{
	const char *escl=strchr(data,'[');
	const char *sepl=strchr(data,':');
	char *escr=strrchr(data,']');
	char *sepr=strrchr(data,':');

	/* extract from "[address]:port" or "[address]"*/
	if (escl && escr)
	{
		memmove(data, data + 1, strlen(data) - strlen(escr));
		data[strlen(data) - strlen(escr) - 1] = 0;
		*escr++ = 0;
		if (*escr == ':')
			escr++;
		return (*escr ? escr : NULL);
	}
	/* extract from "address:port" or "address" */
	if ((sepl == sepr) && sepr && sepl)
	{
		*sepr++ = 0;
		return sepr;
	}

	/* return empty string */
	return NULL;
}
