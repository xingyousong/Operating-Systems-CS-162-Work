#ifndef DEVICES_TIMER_H
#define DEVICES_TIMER_H

#include <round.h>
#include <stdint.h>
#include <list.h>

/* Number of timer interrupts per second. */
#define TIMER_FREQ 100

void timer_init (void);
void timer_calibrate (void);

int64_t timer_ticks (void);
int64_t timer_elapsed (int64_t);

//Our code for sleeping_thread
struct sleeping_thread
{
  int64_t wake_time;
  struct thread* thread;
  struct list_elem elem;
};

//Our code for new functions for sleeping_thread
void add_to_sleeping_queue(struct sleeping_thread *thread);
struct thread* remove_from_sleeping_queue(int64_t current_time);
bool less(struct list_elem* perm_elem, struct list_elem* changing_elem, void* aux);

/* Sleep and yield the CPU to other threads. */
void timer_sleep (int64_t ticks);
void timer_msleep (int64_t milliseconds);
void timer_usleep (int64_t microseconds);
void timer_nsleep (int64_t nanoseconds);

/* Busy waits. */
void timer_mdelay (int64_t milliseconds);
void timer_udelay (int64_t microseconds);
void timer_ndelay (int64_t nanoseconds);

void timer_print_stats (void);

#endif /* devices/timer.h */
