/* thr_lwp.c - wrappers around SunOS LWP threads */
/* $OpenLDAP: pkg/ldap/libraries/libldap_r/thr_lwp.c,v 1.20.2.3 2008/02/11 23:26:42 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2008 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

/* BUGS:
 * - slurpd calls the get_stack/free_stack functions. Should be fixed, so
 *   they can become static.
 */

#include "portable.h"

#if defined( HAVE_LWP )

/*************
 *           *
 * SunOS LWP *
 *           *
 *************/

/* This implementation NEEDS WORK.   It currently does not compile */

#include <stdio.h>

#include <ac/time.h>
#include <ac/socket.h>

#include "ldap-int.h"

#include "ldap_pvt_thread.h" /* Get the thread interface */
#define LDAP_THREAD_IMPLEMENTATION
#include "ldap_thr_debug.h"	 /* May rename the symbols defined below */

#include <lwp/lwp.h>
#include <lwp/stackdep.h>

#define MAX_STACK	51200
#define MAX_THREADS	20

/*
 * Initialize LWP by spinning of a schedular
 */
int
ldap_int_thread_initialize( void )
{
	thread_t		tid;
	stkalign_t		*stack;
	int			stackno;

	if (( stack = get_stack( &stackno )) == NULL ) {
		return -1;
	}

	lwp_create( &tid, lwp_scheduler, MINPRIO, 0, stack, 1, stackno );
	return 0;
}

int
ldap_int_thread_destroy( void )
{
	/* need to destroy lwp_scheduler thread and clean up private
		variables */
	return 0;
}

struct stackinfo {
	int		stk_inuse;
	stkalign_t	*stk_stack;
};

static struct stackinfo	*stacks;

static stkalign_t * ldap_int_thread_get_stack( int *stacknop )
{
	int	i;

	if ( stacks == NULL ) {
		stacks = (struct stackinfo *) LDAP_CALLOC( 1, MAX_THREADS *
		    sizeof(struct stackinfo) );

		if( stacks == NULL ) {
			Debug( LDAP_DEBUG_ANY, "stacks allocation failed",
				0, 0, 0 );
			return NULL;
		}
	}

	for ( i = 0; i < MAX_THREADS; i++ ) {
		if ( stacks[i].stk_inuse == 0 ) {
			break;
		}
	}

	if ( i == MAX_THREADS ) {
		Debug( LDAP_DEBUG_ANY,
		    "no more stacks (max %d) - increase MAX_THREADS for more",
		    MAX_THREADS, 0, 0 );
		return( NULL );
	}

	if ( stacks[i].stk_stack == NULL ) {
		stacks[i].stk_stack = (stkalign_t *) LDAP_MALLOC(
		    (MAX_STACK / sizeof(stkalign_t) + 1 )
		    * sizeof(stkalign_t) );

		if( stacks[i].stk_stack == NULL ) {
			Debug( LDAP_DEBUG_ANY, "stack allocation failed",
				0, 0, 0 );
			return( NULL );
		}
	}

	*stacknop = i;
	stacks[i].stk_inuse = 1;
	return( stacks[i].stk_stack + MAX_STACK / sizeof(stkalign_t) );
}

static void
ldap_int_thread_free_stack( int	stackno )
{
	if ( stackno < 0 || stackno > MAX_THREADS ) {
		Debug( LDAP_DEBUG_ANY, "free_stack of bogus stack %d\n",
		    stackno, 0, 0 );
	}

	stacks[stackno].stk_inuse = 0;
}

static void
lwp_create_stack( void *(*func)(), void *arg, int stackno )
{
	(*func)( arg );

	ldap_int_thread_free_stack( stackno );
}

int 
ldap_pvt_thread_create( ldap_pvt_thread_t * thread, 
	int detach,
	void *(*start_routine)( void *),
	void *arg)
{
	stkalign_t	*stack;
	int		stackno;

	if ( (stack = ldap_int_thread_get_stack( &stackno )) == NULL ) {
		return( -1 );
	}
	return( lwp_create( thread, lwp_create_stack, MINPRIO, 0, 
			   stack, 3, start_routine, arg, stackno ) );
}

void 
ldap_pvt_thread_exit( void *retval )
{
	lwp_destroy( SELF );
}

unsigned int
ldap_pvt_thread_sleep(
	unsigned int interval
)
{
	thread_t		mylwp;
	tl_t		*t, *nt;
	time_t		now;


	if ( lwp_self( &mylwp ) < 0 ) {
		return -1;
	}

	time( &now );

	mon_enter( &sglob->tsl_mon );

	if ( sglob->tsl_list != NULL ) {
		for ( t = sglob->tsl_list; t != NULL; t = t->tl_next ) {
			if ( SAMETHREAD( t->tl_tid, mylwp )) {
				/* We're already sleeping? */
				t->tl_wake = now + interval;
				mon_exit( &sglob->tsl_mon );
				lwp_suspend( mylwp );
				return 0;
			}
		}
	}

	nt = (tl_t *) LDAP_MALLOC( sizeof( tl_t ));

	if( nt == NULL ) return -1;

	nt->tl_next = sglob->tsl_list;
	nt->tl_wake = now + interval;
	nt->tl_tid = mylwp;
	sglob->tsl_list = nt;

	mon_exit( &sglob->tsl_mon );

	lwp_suspend( mylwp );
	return 0;
}

/*
 * The lwp_scheduler thread periodically checks to see if any threads
 * are due to be resumed.  If there are, it resumes them.  Otherwise,
 * it computes the lesser of ( 1 second ) or ( the minimum time until
 * a thread need to be resumed ) and puts itself to sleep for that amount
 * of time.
 */
static void
lwp_scheduler(
	int		stackno
)
{
	time_t			now, min;
	struct timeval		interval;
	tl_t			*t;

	while ( !sglob->slurpd_shutdown ) {
		mon_enter( &sglob->tsl_mon );

		time( &now );
		min = 0L;
		if ( sglob->tsl_list != NULL ) {
			for ( t = sglob->tsl_list; t != NULL; t = t->tl_next ) {
				if (( t->tl_wake  > 0L ) && ( t->tl_wake < now )) {
					lwp_resume( t->tl_tid );
					t->tl_wake = 0L;
				}

				if (( t->tl_wake > now ) && ( t->tl_wake < min )) {
					min =  t->tl_wake;
				}
			}
		}

		mon_exit( &sglob->tsl_mon );

		interval.tv_usec = 0L;
		if ( min == 0L ) {
			interval.tv_sec = 1L;
		} else {
			interval.tv_sec = min;
		}

		lwp_sleep( &interval );
	}

	mon_enter( &sglob->tsl_mon );

	for ( t = sglob->tsl_list; t != NULL; t = t->tl_next ) {
		lwp_resume( t->tl_tid );
	}

	mon_exit( &sglob->tsl_mon );

	free_stack( stackno );
}

int 
ldap_pvt_thread_join( ldap_pvt_thread_t thread, void **thread_return )
{
	lwp_join( thread );
	return 0;
}

int 
ldap_pvt_thread_kill( ldap_pvt_thread_t thread, int signo )
{
	return 0;
}

int 
ldap_pvt_thread_yield( void )
{
	lwp_yield( SELF );
	return 0;
}

int 
ldap_pvt_thread_cond_init( ldap_pvt_thread_cond_t *cond )
{
	/*
	 * lwp cv_create requires the monitor id be passed in
	 * when the cv is created, pthreads passes it when the
	 * condition is waited for.  so, we fake the creation
	 * here and actually do it when the cv is waited for
	 * later.
	 */

	cond->lcv_created = 0;

	return( 0 );
}

int 
ldap_pvt_thread_cond_signal( ldap_pvt_thread_cond_t *cond )
{
	return( cond->lcv_created ? cv_notify( cv->lcv_cv ) : 0 );
}

int 
ldap_pvt_thread_cond_wait( ldap_pvt_thread_cond_t *cond, 
	ldap_pvt_thread_mutex_t *mutex )
{
	if ( ! cond->lcv_created ) {
		cv_create( &cond->lcv_cv, *mutex );
		cond->lcv_created = 1;
	}

	return( cv_wait( cond->lcv_cv ) );	
}

int 
ldap_pvt_thread_mutex_init( ldap_pvt_thread_mutex_t *mutex )
{
	return( mon_create( mutex ) );
}

int 
ldap_pvt_thread_mutex_destroy( ldap_pvt_thread_mutex_t *mutex )
{
	return( mon_destroy( *mutex ) );
}

int 
ldap_pvt_thread_mutex_lock( ldap_pvt_thread_mutex_t *mutex )
{
	return( mon_enter( *mutex ) );
}

int 
ldap_pvt_thread_mutex_unlock( ldap_pvt_thread_mutex_t *mutex )
{
	return( mon_exit( *mutex ) );
}

int
ldap_pvt_thread_mutex_trylock( ldap_pvt_thread_mutex_t *mp )
{
	return( mon_cond_enter( *mp ) );
}

int
ldap_pvt_thread_cond_destroy( ldap_pvt_thread_cond_t *cv )
{
	return( cv->lcv_created ? cv_destroy( cv->lcv_cv ) : 0 );
}

int
ldap_pvt_thread_cond_broadcast( ldap_pvt_thread_cond_t *cv )
{
	return( cv->lcv_created ? cv_broadcast( cv->lcv_cv ) : 0 );
}

ldap_pvt_thread_t
ldap_pvt_thread_self( void )
{
	thread_t		mylwp;

	lwp_self( &mylwp );

	return mylwp;
}

#endif /* HAVE_LWP */
