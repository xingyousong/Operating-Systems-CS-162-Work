Final Report for Project 1: Threads
===================================

## Group Members

* Jonathan Lin (<jlin703@berkeley.edu>)
* Jim Liu (<jim.liu@berkeley.edu>)
* Mong Ng (<monghim.ng@berkeley.edu>)
* Xingyou Song (<xsong@berkeley.edu>)

# Efficient Alarm Clock

<<<<<<< HEAD
## Code Edits and Additions 

### Ultimately, we edited the following code/functions for this task: 

From timer.c: `timer init`, `timer_sleep`

### We also added the functions:

From timer.c: `less`, `struct list sleeping_thread_queue`, `add_to_sleeping_queue`, `remove_from_sleeping_queue`

From timer.h: `struct sleeping_thread`

## Algorithms

Nothing really changed here from our design document.

## Synchronization

Same reasoning from our design document but we did not have to disable interrupts in timer_interrupt because they would already be off.
=======
## Code Edits and Additions
>>>>>>> 32e7feb1a2c65adf51f87079bfb86ff3b75d1aed

### We edited the following functions/code for this task:

From devices/timer.c: 

`timer_init` to initialize the `sleeping_thread_queue`
`timer_sleep` to put thread to sleep
`timer_interrupt` to handle waking up sleeping threads if that much time has passed

### We added the functions/code:

From devices/timer.h:

`bool compare_wake_time(struct list_elem* elem_a, struct list_elem* elem_b, void* aux)`
`void add_to_sleeping_queue(struct sleeping_thread *thread)`
`struct thread* remove_from_sleeping_queue(int64_t current_time)`

`struct list sleeping_thread_queue`

## Algorithms

Same as detailed in the design document, we disable and enable interrupts whenever we need to modify the `sleeping_thread_queue`. We dequeue if necessary in `timer_interrupt`, and we enqueue in `timer_sleep`. One <b>deviation</b> from the design document is that we added the function `compare_wake_time` for `list_insert_ordered` to compare thread in terms of wake up time. We had originally thought we would code naive, but we later found that such operation is available in lists.

## Synchronization
We disabled interrupts for any function that would modify the `sleeping_thraed_queue`.

## Rationale
We added `compare_wake_time` because then it would simplify the code a lot better than if we have done it naively.

# Priority Scheduler

## Code Edits and Additions

### Ultimately, we edited the following code/functions for this task:

From thread.c: `thread_create`, `thread_unblock`, `thread_exit`, `thread_yield`, `thread_set_priority`, `thread_get_priority`, `init_thread`, `next_thread_to_run`.

From thread.h: `struct thread`

### We also added the functions:

From thread.h:

`bool compare_thread(const struct list_elem *a, const struct list_elem *b, void *aux)`

`bool compare_lock(const struct list_elem *a, const struct list_elem *b, void *aux)`

`void specific_thread_set_effective_priority(struct thread *t, int new_effective_priority)`

`int specific_thread_get_effective_priority_acquired_lock_list(struct thread *t)`

`void update_thread_effective_priority(struct thread *t)`

`int lock_get_priority_thread(struct lock* ell)`

`void lock_update_priority(struct lock* ell)`

## Algorithms

We essentially used the same algorithms as mentioned in the design document, with the exception that in `lock_acquire`, we call a function `propagate_broadcast` (which is just a recursive function which propagates through chains of locks, lock-holders, and blocking locks). We did this in order to solve for cases in chaining priority donation - we realized after discussion with our TA that it is not enough to change the priority of the lock upon acquisition.

## Synchronization
We disabled interrupts for any function that would edit priorities, as mentioned in our initial document. There was not much unexpected here.

## Rationale
From the initial design document, we realized that using a priority queue would take too much effort, and be very error prone. We learned that having a correct implementation was far more important than an efficient implementation from our design review. As such, we stuck with the linked list implementation provided to us in the skeleton code.

All other edits on Task 2 essentially followed our original design doc.

# Advanced Scheduler

## Code Edits and Additions

### Ultimately, we edited the following code/functions for this task:

From thread.c: `thread_get_priority`, `thread_set_priority`, `thread_tick`, `init_thread`, `thread_create`.

From thread.h: `struct thread`

### We also added the functions:

From thread.h:

`int thread_get_nice (void)`

`void thread_set_nice (int)`

`int thread_get_recent_cpu (void)`

`int thread_get_load_avg (void)`

`int update_load_avg (void)`

`void thread_update_recent_cpu (struct thread *t, void *aux)`

<<<<<<< HEAD
## Code Edits and Additions

### Ultimately, we edited the following code/functions for this task:

From thread.c: `thread_get_priority`, `thread_set_priority`, `thread_tick`, `init_thread`, `thread_create`.

From thread.h: `struct thread`

### We also added the functions:
=======
`void thread_update_priority (struct thread *t, void *aux)`

## Algorithms

We got rid of the idea of using 64 queues after discussion with our TA. We realized that it would be simpler and just as functional to use one queue for all the threads. In fact, we implemented this part without initializing any new lists, and just relying off of the `ready_list` and `all_list` lists that were already there, and just editing the way the scheduler selected the next thread (also had to coordinate with Task 2 for this). We were sure to make sure of the `thread_foreach` function provided - it was very helpful for updating priorities and `recent_cpu` values.

Another change was choosing to just take the length of the `ready_list` when needed to get the value of `ready_threads` instead of defining our own variable for it. Defining our own variable and updating it in functions led to issues when functions were called more times than expected for whatever reason. As the length of the `ready_list` correctly indicates the number of non-sleeping threads (because Task 1 implemented it this way), this would always be a correct indicator.

## Synchronization
No major issues or changes here. However, we just had to make sure that we put an `if(!thread_mlfqs)` check in front of a couple of the functions in synch.c which were edited for Task 2.
>>>>>>> 32e7feb1a2c65adf51f87079bfb86ff3b75d1aed

From thread.h:

`int thread_get_nice (void)`

`void thread_set_nice (int)`

`int thread_get_recent_cpu (void)`

`int thread_get_load_avg (void)`

`int update_load_avg (void)`

<<<<<<< HEAD
`void thread_update_recent_cpu (struct thread *t, void *aux)`

`void thread_update_priority (struct thread *t, void *aux)`

## Algorithms

We got rid of the idea of using 64 queues after discussion with our TA. We realized that it would be simpler and just as functional to use one queue for all the threads. In fact, we implemented this part without initializing any new lists, and just relying off of the `ready_list` and `all_list` lists that were already there, and just editing the way the scheduler selected the next thread (also had to coordinate with Task 2 for this). We were sure to make sure of the `thread_foreach` function provided - it was very helpful for updating priorities and `recent_cpu` values.

Another change was choosing to just take the length of the `ready_list` when needed to get the value of `ready_threads` instead of defining our own variable for it. Defining our own variable and updating it in functions led to issues when functions were called more times than expected for whatever reason. As the length of the `ready_list` correctly indicates the number of non-sleeping threads (because Task 1 implemented it this way), this would always be a correct indicator.

## Synchronization
No major issues or changes here. However, we just had to make sure that we put an `if(!thread_mlfqs)` check in front of a couple of the functions in synch.c which were edited for Task 2.





# Reflection

## Member Tasks

Jonathan Lin - Wrote all Task 3 code

Jim Liu - Wrote task 1 code and cleaned up code.
=======
Jonathan Lin - Wrote all Task 3 code

Jim Liu -  Wrote task 1 code and cleaned up code.

Mong Ng - Helped debug part1. Communicate between Task 2 and Task 3 such that an interface is agreed upon for the common code in both parts, such as the determining the next thread to run.

Xingyou Song - Wrote Task 2 code and ensured that it works, attended a lot of office hours to tell team what is good and bad
>>>>>>> 32e7feb1a2c65adf51f87079bfb86ff3b75d1aed

## Jonathan Lin
I felt that we did well in coordinating between the different tasks to ensure smooth integration in the end. We kept all the relevant work in separate branches as to give us fine-grained control over what code we were working on at any given time. We made good use of the debugging tools at our disposable (aka gdb) and our group was pretty good at lending a hand to different tasks when another needed assistance - a fresh pair of eyes really helped debugging at times.

Conceptually, I also think we grasped the concepts relatively well but definitely could have done better in fully flushing out our ideas in more detail during the design doc stage as to avoid some bugs which were definitely preventable.

## Jim Liu
For task 1, I correctly implemented the sleep_thread struct with list elem elements in order to use the linked list provided. I also made sure not to disable interrupts erroneously inside of timer_interrupt because timer interrupts were already off. The rest of task 1 was coded using our design document as reference. 

<<<<<<< HEAD
I felt that we did well in coordinating between the different tasks to ensure smooth integration in the end. We kept all the relevant work in separate branches as to give us fine-grained control over what code we were working on at any given time. We made good use of the debugging tools at our disposable (aka gdb) and our group was pretty good at lending a hand to different tasks when another needed assistance - a fresh pair of eyes really helped debugging at times.

Conceptually, I also think we grasped the concepts relatively well but definitely could have done better in fully flushing out our ideas in more detail during the design doc stage as to avoid some bugs which were definitely preventable.

## Jim Liu 

For task 1, I correctly implemented the sleep_thread struct with list elem elements in order to use the linked list provided. I also made sure not to disable interrupts erroneously inside of timer_interrupt because timer interrupts were already off. The rest of task 1 was coded using our design document as reference. 

We did a good job at dividing work up even though there were 3 parts and 4 team members. We had 1 team member on each task and the last team member as a flex debugger and jack of all trades. The coding portion of the project went well even though all the team members had significant commitments. The fact that the 3 parts were largely independent of each other in terms of auto-grader progress made it easy for us to work as we had time. 

The thing that we could majorly improve on is our design document. We will try to have every team member come up with a scarcely detailed mock up of a document after reading the project specs. We should also review our document together to make sure we are not missing any details for any part.

=======
We did a good job at dividing work up even though there were 3 parts and 4 team members. We had 1 team member on each task and the last team member as a flex debugger and jack of all trades. The coding portion of the project went well even though all the team members had significant commitments. The fact that the 3 parts were largely independent of each other in terms of auto-grader progress made it easy for us to work as we had time. 

The thing that we could majorly improve on is our design document. We will try to have every team member come up with a scarcely detailed mock up of a document after reading the project specs. We should also review our document together to make sure we are not missing any details for any part.
>>>>>>> 32e7feb1a2c65adf51f87079bfb86ff3b75d1aed

## Mong Ng
I feel that we were exceptionally in the use of Git that we barely have any conflict. We also used GDB extensively in debugging. Great team as Task2 and Task3 people would first decide the code that is needed in both parts.

One thing I learned is that sometimes bugs introduced in one part might not be because of that part. For example, at one point I was debugging part1 when the bug was actually because of part3; namely, we were not ready to pass that test yet.

We could also go to office hour more and get help instead of spending coutless hours debugging and staring at code. We should also read more carefully the given code, for example, list.h, to understand what operations are given to us.


## Xingyou Song
I think where we did well was with the work-load separation - We split the code into independent components and each coded our own parts (and recombined at the very end), to reduce complexity of understanding the entire code, and confidently progress.

I think in retrospect, in order to make debugging faster, we should have read what the test cases are using - for example, I forgot to edit the `thread_set_priority` function, which was why I had so many errors for a long time on the priority test cases.

Also, we should've had a more thorough initial design document that detailed every single function to be edited - this would've helped us follow a systematic list of to-do's.
