/* This testcase is part of GDB, the GNU debugger.

   Copyright 2023 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

/* The delay that the main thread gives once all the worker threads have
   reached the barrier before the main thread enters the function on which
   GDB will have placed a breakpoint.  */

#define MAIN_THREAD_DELAY 2

/* The maximum time we allow this test program to run for before an alarm
   signal is sent and everything will exit.  */
#define WATCHDOG_ALARM_TIME 600

/* Aliases for the signals used within this script.  Each signal
   corresponds to an action (from the FINAL_ACTION enum) that the signal
   handler will perform.  */

#define SPIN_SIGNAL SIGUSR1
#define SYSCALL_SIGNAL SIGUSR2

/* Describe the final action that a thread should perform.  */

enum final_action
  {
    /* Thread should spin in an infinite loop.  */
    SPIN = 0,

    /* Thread should block in a syscall.  */
    SYSCALL,

    /* This is just a marker to allow for looping over the enum.  */
    LAST_ACTION
  };

/* Where should the thread perform this action?  */

enum exec_location
  {
    /* Just a normal thread, on a normal stack.  */
    NORMAL = 0,

    /* In a signal handler, but use the normal stack.  */
    SIGNAL_HANDLER,

    /* In a signal handler using an alternative stack.  */
    SIGNAL_ALT_STACK,

    /* This is just a marker to allow for looping over the enum.  */
    LAST_LOCACTION
  };

/* A descriptor for a single thread job.  We create a new thread for each
   job_description.  */

struct job_description
{
  /* What action should this thread perform.  */
  enum final_action action;

  /* Where should the thread perform the action.  */
  enum exec_location location;

  /* The actual thread handle, so we can join with the thread.  */
  pthread_t thread;
};

/* A pthread barrier, used to (try) and synchronise the threads.  */
pthread_barrier_t global_barrier;

/* Return a list of jobs, and place the length of the list in *COUNT.  */

struct job_description *
get_job_list (int *count)
{
  /* The number of jobs.  */
  int num = LAST_ACTION * LAST_LOCACTION;

  /* The uninitialised array of jobs.  */
  struct job_description *list
    = malloc (num * sizeof (struct job_description));
  assert (list != NULL);

  /* Fill the array with all possible jobs.  */
  for (int i = 0; i < (int) LAST_ACTION; ++i)
    for (int j = 0; j < (int) LAST_LOCACTION; ++j)
      {
	int idx = (i * LAST_LOCACTION) + j;
	list[idx].action = (enum final_action) i;
	list[idx].location = (enum exec_location) j;
      }

  /* Return the array of jobs.  */
  *count = num;
  return list;
}

/* This function should never be called.  If it is then an assertion will
   trigger.  */

void
assert_not_reached (void)
{
  assert (0);
}

/* The function for a SPIN action.  Just spins in a loop.  The LOCATION
   argument exists so GDB can identify the expected context for this
   function.  */

void
do_spin_task (enum exec_location location)
{
  (void) location;

  /* Let everyone know that we're about to perform our action.  */
  int res = pthread_barrier_wait (&global_barrier);
  assert (res == PTHREAD_BARRIER_SERIAL_THREAD || res == 0);

  while (1)
    {
      /* Nothing.  */
    }
}

/* The function for a SYSCALL action.  Just spins in a loop.  The LOCATION
   argument exists so GDB can identify the expected context for this
   function.  */

void
do_syscall_task (enum exec_location location)
{
  (void) location;

  /* Let everyone know that we're about to perform our action.  */
  int res = pthread_barrier_wait (&global_barrier);
  assert (res == PTHREAD_BARRIER_SERIAL_THREAD || res == 0);

  sleep (600);
}

/* Return the required size for a sigaltstack.  We start with a single
   page, but do check against the system defined minimums.  We don't run
   much on the alternative stacks, so we don't need a huge one.  */

size_t
get_stack_size (void)
{
  size_t size = getpagesize ();	/* Arbitrary starting size.  */
  if (size < SIGSTKSZ)
    size = SIGSTKSZ;
  if (size < MINSIGSTKSZ)
    size = MINSIGSTKSZ;
  return size;
}

/* A descriptor for an alternative stack.  */

struct stack_descriptor
{
  /* The base address of the alternative stack.  This is the address that
     must be freed to release the memory used by this stack.  */
  void *base;

  /* The size of this alternative stack.  Tracked just so we can query this
     from GDB.  */
  size_t size;
};

/* Install an alternative signal stack.  Return a descriptor for the newly
   allocated alternative stack.  */

struct stack_descriptor
setup_alt_stack (void)
{
  size_t stack_size = get_stack_size ();

  void *stack_area = malloc (stack_size);

  stack_t stk;
  stk.ss_sp = stack_area;
  stk.ss_flags = 0;
  stk.ss_size = stack_size;

  int res = sigaltstack (&stk, NULL);
  assert (res == 0);

  struct stack_descriptor desc;
  desc.base = stack_area;
  desc.size = stack_size;

  return desc;
}

/* Return true (non-zero) if we are currently on the alternative stack,
   otherwise, return false (zero).  */

int
on_alt_stack_p (void)
{
  stack_t stk;
  int res = sigaltstack (NULL, &stk);
  assert (res == 0);

  return (stk.ss_flags & SS_ONSTACK) != 0;
}

/* The signal handler function.  All signals call here, so we use SIGNO
   (the signal that was delivered) to decide what action to perform.  This
   function might, or might not, have been called on an alternative signal
   stack.  */

void
signal_handler (int signo)
{
  enum exec_location location
    = on_alt_stack_p () ? SIGNAL_ALT_STACK : SIGNAL_HANDLER;

  switch (signo)
    {
    case SPIN_SIGNAL:
      do_spin_task (location);
      break;

    case SYSCALL_SIGNAL:
      do_syscall_task (location);
      break;

    default:
      assert_not_reached ();
    }
}

/* The thread worker function.  ARG is a job_description pointer which
   describes what this thread is expected to do.  This function always
   returns a NULL pointer.  */

void *
thread_function (void *arg)
{
  struct job_description *job = (struct job_description *) arg;
  struct stack_descriptor desc = { NULL, 0 };
  int sa_flags = 0;

  switch (job->location)
    {
    case NORMAL:
      /* This thread performs the worker action on the current thread,
	 select the correct worker function based on the requested
	 action.  */
      switch (job->action)
	{
	case SPIN:
	  do_spin_task (NORMAL);
	  break;

	case SYSCALL:
	  do_syscall_task (NORMAL);
	  break;

	default:
	  assert_not_reached ();
	}
      break;

    case SIGNAL_ALT_STACK:
      /* This thread is to perform its action in a signal handler on the
	 alternative stack.  Install the alternative stack now, and then
	 fall through to the normal signal handler location code.  */
      desc = setup_alt_stack ();
      assert (desc.base != NULL);
      assert (desc.size > 0);
      sa_flags = SA_ONSTACK;

      /* Fall through.  */
    case SIGNAL_HANDLER:
      {
	/* This thread is to perform its action in a signal handler.  We
	   might have just installed an alternative signal stack.  */
	int signo, res;

	/* Select the correct signal number so that the signal handler will
	   perform the required action.  */
	switch (job->action)
	  {
	  case SPIN:
	    signo = SPIN_SIGNAL;
	    break;

	  case SYSCALL:
	    signo = SYSCALL_SIGNAL;
	    break;

	  default:
	    assert_not_reached ();
	  }

	/* Now setup the signal handler.  */
	struct sigaction sa;
	sa.sa_handler = signal_handler;
	sigfillset (&sa.sa_mask);
	sa.sa_flags = sa_flags;
	res = sigaction (signo, &sa, NULL);
	assert (res == 0);

	/* Send the signal to this thread.  */
	res = pthread_kill (job->thread, signo);
	assert (res == 0);
      }
      break;

    default:
      assert_not_reached ();
    };

  /* Free the alt-stack if we allocated one, if not DESC.BASE will be
     NULL so this call is fine.  */
  free (desc.base);

  /* Thread complete.  */
  return NULL;
}

void
start_job (struct job_description *job)
{
  int res;

  res = pthread_create (&job->thread, NULL, thread_function, job);
  assert (res == 0);
}

/* Join with the thread for JOB.  This will block until the thread for JOB
   has finished.  */

void
finalise_job (struct job_description *job)
{
  int res;
  void *retval;

  res = pthread_join (job->thread, &retval);
  assert (res == 0);
  assert (retval == NULL);
}

/* Function that GDB can place a breakpoint on.  */

void
breakpt (void)
{
  /* Nothing.  */
}

/* Function that triggers a crash, if the user has setup their environment
   correctly this will dump a core file, which GDB can then examine.  */

void
crash_function (void)
{
  volatile int *p = 0;
  volatile int n = *p;
  (void) n;
}

/* Entry point.  */

int
main ()
{
  int job_count, res;
  struct job_description *jobs = get_job_list (&job_count);

  /* This test is going to park some threads inside infinite loops.  Just
     in case this program is left running, install an alarm that will cause
     everything to exit.  */
  alarm (WATCHDOG_ALARM_TIME);

  /* We want each worker thread (of which there are JOB_COUNT) plus the
     main thread (hence + 1) to wait at the barrier.  */
  res = pthread_barrier_init (&global_barrier, NULL, job_count + 1);
  assert (res == 0);

  /* Start all the jobs.  */
  for (int i = 0; i < job_count; ++i)
    start_job (&jobs[i]);

  /* Notify all the worker threads that we're waiting for them.  */
  res = pthread_barrier_wait (&global_barrier);
  assert (res == PTHREAD_BARRIER_SERIAL_THREAD || res == 0);

  /* All we know at this point is that all the worker threads have reached
     the barrier, which is just before they perform their action.  But we
     really want them to start their action.

     There's really no way we can be 100% certain that the worker threads
     have started their action, all we can do is wait for a short while and
     hope that the machine we're running on is not too slow.  */
  sleep (MAIN_THREAD_DELAY);

  /* A function that GDB can place a breakpoint on.  By the time we get
     here we are as sure as we can be that all of the worker threads have
     started and are in their worker action (spinning, or syscall).  */
  breakpt ();

  /* If GDB is not attached then this function will cause a crash, which
     can be used to dump a core file, which GDB can then analyse.  */
  crash_function ();

  /* Due to the crash we never expect to get here.  Plus the worker actions
     never terminate.  But for completeness, here's where we join with all
     the worker threads.  */
  for (int i = 0; i < job_count; ++i)
    finalise_job (&jobs[i]);

  /* Cleanup the barrier.  */
  res = pthread_barrier_destroy (&global_barrier);
  assert (res == 0);

  /* And clean up the jobs list.  */
  free (jobs);

  return 0;
}
