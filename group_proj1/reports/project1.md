Final Report for Project 1: Threads
===================================

## Group Members

* Jonathan Lin <jlin703@berkeley.edu>
* Jim Liu <jim.liu@berkeley.edu>
* Mong Ng <monghim.ng@berkeley.edu>
* Xingyou Song <xsong@berkeley.edu>

# Efficient Alarm Clock



# Priority Scheduler 

## Code Edits and Additions 


### Ultimately, we edited the following code/functions for this task: 

From thread.c: `thread_create`, `thread_unblock`, `thread_exit`, `thread_yield`, `thread_set_priority`, `thread_get_priority`, `init_thread`, `next_thread_to_run`. 

From thread.h: `struct thread`

From synch.c: `sema_up`, `lock_init`, `lock_acquire`, `lock_release`, `lock_try_acquire`, `cond_signal`. 

From synch.h: `struct lock`. 

### We also added the functions: 

From thread.h: 

`bool compare_thread(const struct list_elem *a, const struct list_elem *b, void *aux)`

`bool compare_lock(const struct list_elem *a, const struct list_elem *b, void *aux)`

`void specific_thread_set_effective_priority(struct thread *t, int new_effective_priority)`

`int specific_thread_get_effective_priority_acquired_lock_list(struct thread *t)`

`void update_thread_effective_priority(struct thread *t)`

`int lock_get_priority_thread(struct lock* ell)`

`void lock_update_priority(struct lock* ell)`

From synch.h: 


`void propagate_broadcast(int moving_priority, struct thread *current_thread)`

`void remove_lock(struct lock *lok)`

`bool compare_sema(const struct list_elem *a, const struct list_elem *b, void *aux)`





## Algorithms 

We used the same algorithms as mentioned in the design document, but with the edit that in lock_acquire, we call a `propagate_broadcast` (which is just a recursive function) in order to solve for cases in chaining priority donation. 

## Synchronization 
We disabled interrupts for any function that would edit priorities, as mentioned in our initial document. 

## Rationale 
From the initial design document, we realized that using a priority queue would take too much effort, and be very error prone. We learned that having a correct implementation was far more important than an efficient implementation from our design review. 

For all other edits on task 2, they essentially followed our original design doc. 

# Advanced Scheduler 







# Reflection

## Member Tasks



Jonathan Lin -

Jim Liu - 

Mong Ng - 

Xingyou Song - Code up Task 2 and ensured that it works, attended a lot of office hours to tell team what is good and bad

## Jonathan Lin

## Jim Liu 


## Mong Ng



## Xingyou Song
I think where we did well was with the work-load separation - We split the code into independent components and each coded our own parts (and recombined at the very end), to reduce complexity of understanding the entire code, and confidently progress. 

I think in retrospect, in order to make debugging faster, we should have read what the test cases are using - for example, I forgot to edit the `thread_set_priority` function, which was why I had so many errors for a long time on the priority test cases. 

Also, we should've had a more thorough initial design document that detailed every single function to be edited - this would've helped us follow a systematic list of to-do's.