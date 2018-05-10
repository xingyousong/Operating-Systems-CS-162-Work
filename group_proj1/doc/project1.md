Design Document for Project 1: Threads
======================================

## Group Members

* Xingyou Song <xsong@berkeley.edu>
* Jim Liu <jim.liu@berkeley.edu>
* Mong Ng <monghim.ng@berkeley.edu>
* Jonathan Lin <jlin703@berkeley.edu>


# Efficient Alarm Clock

## Data Structures and functions

`struct List sleeping_queue`

A global queue used for ordered sleeping queue that stores the thread to put to sleep and its corresponding wake up time (defined as kernel ticks at time of sleep call + number of ticks to sleep).

```c 
struct sleeping_thread {
int64_t wake_time,
thread* thread
}
```
Used to bundle a sleeping thread with its wake up time

`void add_to_sleeping_queue(sleeping_thread thread)`

This function will add thread to the queue and maintain wake up time ordering in the queue. This function assumes interrupts are disabled.


`struct thread* remove_from_sleeping_queue(int64_t current_time)`

This function will remove the head from the queue only if the head’s wake_time is less than or equal to current_time and return the pointer of the thread removed.This function assumes interrupts are disabled.


We also modified `timer_sleep()` and `timer_interrupt()`


## Algorithms: 

We needed to fix the issues that the previous implementation had that, such as calling thread_yield did not block the thread from being run and the busy waiting that occured from repeatedly calling thread_yield. We fixed the first issue by making sure that the threads were being blocked and then unblocked when the allotted ticks passed. The second issue was resolved by implementing sleeping_queue which allowed us to store all sleeping threads in a sorted order and allow us to quickly be able to switch threads from “sleeping” to ready.

### Modification of timer_sleep
* First disable interrupts
* Calculate wake up time and call `add_to_sleeping_queue()`
* Call to `thread_block()` to block the thread
* Re-enable interrupts

### Modification of timer_interrupt
Within the codes when we disable interrupts, we have a while loop that keeps calling
`remove_from_sleeping_queue()`. If result is a thread, we wake it up by unblocking it. Then repeat, until we get null, which signals that no sleeping thread needs to wake up anymore.
	
## Synchronization: 

While we insert an element into the sleeping queue, we want to avoid synchronization issues that could result when two different threads want to add themselves to the queue at the same time. Our solution to this is to disable interrupts while we insert into the sleeping queue and remove from the sleeping queue using: `intr_disable` in interrupt.c. 

##  Rationale: 

Two design decisions that we decided to make are using the built in linked list implementation for our sleeping queue instead of a min heap/priority queue and using the low-level functions: `thread_block` and `thread_unblock` instead of synchronization primitives. 

We decided to use the built in linked list because it was already given to us and we value less coding over more efficient runtime. Having the linked list be ordered was a good design choice because instead of having to traverse the entire linked list to find a thread to remove, we would just have to remove from the head. We decided to use the low-level functions for the similar reason that it is easier to call 2 functions than creating the primitives and calling their functions. 



# Priority Scheduler 

## Code Edits and Additions 

We will include in the thread struct, two variables `base_priority`, and `effective_priority`. The thread will also have a list of the locks it is using. Furthermore, we will edit the functions `thread_set_priority` and `thread_get_priority`, and `next_thread_to_run`. Since we edited the thread struct, we wil also need to edit `init_thread`. Since the priority queue (heap) is not provided, we will also have to add this additional code into `list.h`.



```c
struct thread {
	int base_priority; 
	int effective_priority;

	struct list lock_list
}
```


We will also therefore modify (due to the algorithm we present), the `struct lock`, by giving it a `int lock_priority`, and a priority queue `thread_requests` as shown here: 

```c
struct lock {
	int lock_priority;
	struct priority_queue thread_requests
}
```

We must therefore edit the functions `lock_init`, `lock_acquire`, `lock_release`, and write a function `lock_held_by_current_thread` to signal which thread is currently holding the lock. 


In addition, for condition variables such as semaphores `sema_up`, we will also specify the top effective priority thread in the function. For condition variables such as monitors, they should not be changed by the current rule, as iti s handled in lock.

## Algorithms 

The main crux of the algorithm is the method for priority donation. With all methods considered, we decided to give each lock a priority queue (i.e. heap), which contains the `base_priority` of all of the threads that wish to use it. We may assign the `lock_priority` of a lock to therefore be the maximum `base_priority` in this priority queue. Thus, this implies that a thread's `effective_priority` is therefore the maximum of the `lock_priority`s of the locks it holds, and its `base_priority`. Because of the heap structure as well, the lock can easily assign itself to the correct thread as well. 

Thus, we will edit `next_thread_to_run` to be the maximum of all of the existing threads' `effective_priority`.

The cases for when we need to update priority values only occur when 1. A lock is released 2. A thread requests a lock 3. A thread acquires a lock. These should be the allotted times for updating scheduling. 

## Synchronization 

When priority is being updated for any threads (or locks), we disallow any interrupts to occur. The timeline for the priority scheduler is therefore (run thread, schedule, run thread, schedule...). 

## Rationale 
We considered other methods of doing this part at first. The conceptual question here can be represented as a directed graph, with vertices representing threads, and directed edges from threads u to v as "u requires a resource that v has locked to". We may then color edges according to the lock they represent, and thus the edges of this graph may be partitioned into lock id's. 

The `effective_priority` of a vertex in this graph would therefore be the maximum `base_priority` of all of the threads/vertices in the vertex's connected component in reverse edge direction. This would allow the `effective_priority` to be a DFS traversal of the graph. However, due to changes in requesting locks from other vertices, this would require a very inefficient and complicated update scheme to each vertex's `effective_priority`, which made this idea ineffective.

Anther idea, due to the finite size limit of the priority, is for each priority number to have a linked-list of all the of threads whose `effective_priority` is equal to that priority number. This would allow the `next_thread_to_run` to be a constant runtime, but this would also lead to very complicated updating rules for priority donations. 

Ultimately, we decided to use the current setup to avoid large, complicated priority donation solutions, at the expense of the efficiency of the `next_thread_to_run` function. 

# Advanced Scheduler 

## Data structures and functions
```c
struct thread_list_elem {
	struct thread t;
	struct list_elem elem;
};
```
Struct that will represent each element in our ready queues for each priority

```c
struct list priority_ready_queues[64];
Array of 64 linked lists: represents the 64 ready queues
```

`int priority_update(struct thread);`
Called every four ticks, uses `recent_cpu_update` and `thread_get_priority` to recalculate the thread’s priority, and updates what linked list it belongs to, if necessary

`int thread_get_recent_cpu(void)` 
Returns 100 times the current thread’s recent_cpu value, rounded to the nearest integer.

`int thread_get_load_avg(void)`
[Specified in spec] Returns 100 times the current system load average, rounded to the nearest integer.

`int thread_get_nice (void)`
[Specified in spec] Returns the current thread’s nice value.

`void thread_set_nice (int new_nice)`
[Specified in spec] Sets the current thread’s nice value to new_nice and recalculates the thread’s priority based on the new value. If the running thread no longer has the highest priority, it should yield the CPU.

We will modify `struct thread` to include the variable `fixed_point_t recent_cpu` and `int nice`.

We will also have to modify the functions `thread_create()`, `thread_set_priority()`, and `thread_get_priority`

`int ready_threads`
Global variable updated every time thread is created or dies

`int load_avg`
Global variable updated every second

## Algorithm

### Calculating the `ready_threads`  and Global Load Average
The `ready_threads` at any given moment can only be changed when threads dies, blocks, unblocks, and created. In all of these situation, we always have part of the corresponding function to disable interrupt. This ensures that `ready_threads` can be changed by one thread at any given moment and ensures its correctness. In `timer_interrupt()`, we mod the `kernel_ticks` by 4 to check if it is time to update `load_avg`. If it is, we simply use the provided equation to calculate and assign. Note that `timer_interrupt()` disables interrupts.

### Calculating Recent CPU and Priority for each thread
In `timer_interrupt()`, we mod the `kernel_ticks` by 4 to check if it is time to update `recent_cpu` of every thread. If it is, for each thread, we simply use the provided equation to calculate `recent_cpu` of each thread using `nice`. Note that `timer_interrupt()` disables interrupts. Then later `thread_get_priority()` will use the given formula to calculate the priority dynamically whenever the value is needed (rounded to the nearest valid value). 


### Putting Each Thread in the Correct Priority Bucket in the `priority_ready_queues`
Every 4 ticks in `timer_interrupt()`, we call `priority_update()`. This function reallocates `priority_ready_queues[64]` and 64 linked lists. For each thread in each old bucket, we recalculate the priority by `thread_get_priority()` and use the roundedvalue to determine which bucket to put the thread into.

### Determine Next Thread to Run
We loop through `priority_ready_queues` to find the next available highest priority thread: We first locate the bucket with the highest priority number, then just round robin-styled pop the next ready thread.

### Disabling Priority Donation
`thread_set_priority()`: If `thread_mlfqs` is true, we do nothing. If false, we continue the functionality of part 2.
thread_get_priority: If `thread_mlfqs` is true, we recalculate on the fly the priority from recent_cpu and nice. If false, we continue the functionality of part 2.
thread_create: If `thread_mlfqs` is true, we ignore the priority parameter.


## Synchronization
Whenever a thread is created, destroyed, or blocked, interrupts are turned off for a section of code, during which we can update the `ready_threads` variable, so there should be no issues there.

When we change `load_avg` or recalculating `recent_cpu` or moving threads to the new buckets, we only does so in `timer_interrupt()`, which disables interrupts. So there can never be any race condition.

If we ensure that the `load_avg` is set before calling `thread_get_recent_cpu`, there should be no synchronization issues. 

Since we recalculate priority in `thread_get_priority()` dynamically using the thread's `nice`, we can reflect the change in nice anytime.

## Rationale
Note that `ready_threads` has to be up-to-date , `load_avg` has to be updated once per second (and done before `recent_cpu` is updated), and `recent_cpu` is updated once per second for all threads (once per tick for the running thread). 
We make sure to constantly update `ready_threads` in real time so it should only ever take constant time to update `load_avg`. Updating all the `recent_cpu` values, and therefore `priority` values as well, should be linear with respect to the number of threads. 

By separating threads by their priority and putting them in different buckets, we get bounded constant time to find the next ready thread.

We always update load average, recent cpu time, and shuffle threads in `timer_interrupt()`, which disables interrupts, so we ensure that we begin changing these values, they are all changed together and without external interrupts.


# Additional Questions 

## 1. 
Suppose that I have threads A,C whose priorities are in the order A < C (i.e. C has the highest priority). Suppose that C requires a lock on a resource that A currently uses. Therefore A has an effective priority of C's base. Now suppose all threads in this set are blocked due to outside threads. Now suppose that sema_up() is called by an outside thread. Then the correct behavior (assuming effective priority is used) is to unblock A, and allow it to run, which will allow C to run later. If we unblock C only (based on base priority), then it will still be unable to run due to the lock acquiring procedure. 


## 2. 

timer ticks | R(A) | R(B) | R(C) | P(A) | P(B) | P(C) | thread to run
------------|------|------|------|------|------|------|--------------
 0          | 0    | 0    | 0    | 63   | 61   | 59   | A
 4          | 4    | 0    | 0    | 62   | 61   | 59   | A
 8          | 8    | 0    | 0    | 61   | 61   | 59   | A and B
12          | 10   | 2    | 0    | 60.5 | 60.5 | 59   | A and B
16          | 12   | 4    | 0    | 60   | 60   | 59   | A and B
20          | 14   | 6    | 0    | 59.5 | 59.5 | 59   | A and B and C
24          | 16   | 7    | 1    | 59   | 59.25| 58.75| A and B
28          | 18   | 9    | 1    | 58.5 | 58.75| 58.75| A and B and C
32          | 19   | 11   | 2    | 58.25| 58.25| 58.5 | A and B and C
36          | 20   | 12   | 4    | 58   | 58   | 58   | A and B and C


## 3. 

When 2 threads were both in the highest priority queue, I would allow one thread to run for 2 ticks and the other thread to run for the other 2 ticks in the 4 tick interval. 

When all 3 threads were in the same priority queue, I used round robin scheduling to allocate 2 ticks for 1 thread and 1 tick each for the other 2 threads. In the first tie, A got 2 ticks. In the second tie, B got 2 ticks. In the third tie, C got 2 ticks. Since the last tie was in the last time slot, I did not have to increment the recent CPU again, but I would have allocated 2 ticks to thread A if I had to.
