/* Creates several threads all at the same priority and ensures
   that they consistently run in the same round-robin order.

   Based on a test originally submitted for Stanford's CS 140 in
   winter 1999 by by Matt Franklin
   <startled@leland.stanford.edu>, Greg Hutchins
   <gmh@leland.stanford.edu>, Yu Ping Hu <yph@cs.stanford.edu>.
   Modified by arens. */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "devices/timer.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"


#define THREAD_CNT 400
#define ITER_CNT 100

extern long long int sch_time;

static thread_func simple_thread_func;

void
test_priority_fifo (void) 
{
  int i, cnt;

  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  /* Make sure our priority is the default. */
  ASSERT (thread_get_priority () == PRI_DEFAULT);

  msg ("%d threads will played %d ticks.",
       THREAD_CNT, ITER_CNT);
  msg ("If the order varies then there is a bug.");

  thread_set_priority (256);
  for (i = 1; i < THREAD_CNT+1; i++) 
    {
      char name[16];
      snprintf (name, sizeof name, "%d ", i);
      thread_create (name, 32+(i*5)/(THREAD_CNT), simple_thread_func, NULL);
    }
  thread_set_priority (1);
  thread_yield();
  printf("sch_time:%lld\n",sch_time);
  /* All the other threads now run to termination here. */
	/*
  cnt = 0;
  for (; output < op; output++) 
    {
      struct simple_thread_data *d;

      ASSERT (*output > 0 && *output <= THREAD_CNT);
      d = data + *output;
      printf ("%d", d->id);
    }
	*/
	
}

static void 
simple_thread_func (void *data_) 
{
  int64_t t=timer_ticks();
  while(timer_elapsed(t) < ITER_CNT);
}
