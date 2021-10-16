/* $NetBSD: opt_dj.c,v 1.1 2021/10/16 03:20:13 rillig Exp $ */
/* $FreeBSD$ */

#indent input
int i;
int *ip;
const char *ccp;
const void *****vppppp;
const void ******vpppppp;
const void ********vpppppppp;
#indent end

#indent run -dj
int		i;
int	       *ip;
const char     *ccp;
const void *****vppppp;
const void ******vpppppp;
const void ********vpppppppp;
#indent end

#indent input
/* FIXME: The options -dj and -ndj produce the same output. */

int i;
int *ip;
const char *ccp;
const void *****vppppp;
const void ******vpppppp;
const void ********vpppppppp;
#indent end

#indent run -ndj
/* FIXME: The options -dj and -ndj produce the same output. */

int		i;
int	       *ip;
const char     *ccp;
const void *****vppppp;
const void ******vpppppp;
const void ********vpppppppp;
#indent end
