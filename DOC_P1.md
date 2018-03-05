			+--------------------+
			|        CS 140      |
			| PROJECT 1: THREADS |
			|   DESIGN DOCUMENT  |
			+--------------------+

---- GROUP ----

Teo Price-Broncucia <teo.price-broncucia@ucdenver.edu>

Aaron Roberts <aaron.roberts@ucdenver.edu>

---- PRELIMINARIES ----

>> If you have any preliminary comments on your submission, notes for the
>> TAs, or extra credit, please give them here.

>> Please cite any offline or online sources you consulted while
>> preparing your submission, other than the Pintos documentation, course
>> text, lecture notes, and course staff.

			     ALARM CLOCK
			     ===========

---- DATA STRUCTURES ----

>> A1: Copy here the declaration of each new or changed `struct' or
>> `struct' member, global or static variable, `typedef', or
>> enumeration.  Identify the purpose of each in 25 words or less.
struct list sleep_list;
//This holds a list of all sleeping threads sorted by wake-up time.
int64_t wake_up_time
//This is part of the thread struct and holds a wake up time for a thread
list_elem sleep_elem
//This is also part of the thread struct and holds place in the sleep lists
semaphore sleep_sema
//Part of thread struct. Used to track wake up time.



---- ALGORITHMS ----

>> A2: Briefly describe what happens in a call to timer_sleep(),
>> including the effects of the timer interrupt handler.
//When timer_sleep is called first, a wake up time is calculated. Then
//an Assert confirms interrupts are turned on. Then the thread variables are
updated and the thread is added to the sleep list in order of its wake up time.
Finally sema_down() is called on the thread's sleep_sema.

>> A3: What steps are taken to minimize the amount of time spent in
>> the timer interrupt handler?
As the list of sleeping threads is ordered the interrupt only needs to check
the minimum number of threads.

---- SYNCHRONIZATION ----

>> A4: How are race conditions avoided when multiple threads call
>> timer_sleep() simultaneously?
//This is not handled currently.

>> A5: How are race conditions avoided when a timer interrupt occurs
>> during a call to timer_sleep()?
//This is not handled currently.

---- RATIONALE ----

>> A6: Why did you choose this design?  In what ways is it superior to
>> another design you considered?
//This seemed to allow the minimum extra data to be stored while still allowing
the list to be sorted in a way that made the interrupt efficient.

			 PRIORITY SCHEDULING
			 ===================

---- DATA STRUCTURES ----

>> B1: Copy here the declaration of each new or changed `struct' or
>> `struct' member, global or static variable, `typedef', or
>> enumeration.  Identify the purpose of each in 25 words or less.

 ### `struct thread`
 ```
    // Priority
    int priority;                       /* Priority. */
    int base_priority;
    struct list donations_given;
    struct list donations_received;
```
- Priority was repurposed to represent effective priority.
- Base priority was added as a restore point when all received donations have been "returned".
- Lists are maintained for donations received as well as given to facilitate the various donation scenarios.

### `struct donation`
This is a new data structure for tracking relevant data related to donations. It is used as a member in `struct lock`.
```
struct donation
  {
    int priority;
    struct thread *recipient;
    struct list_elem donor_elem;
    struct list_elem recipient_elem;
    bool is_listed;
  };
```
- Priority is the value of the priority donated.
- Recipient is used for recursive, or nested, donations.
- The list elements allow threads to keep track of the donations they've made and received.
- `is_listed` is essentially a check on whether this is a real donation, or a placeholder in a lock which hasn't currently been donated to.

### `struct lock`
```
    struct donation donation;
```
- Ties data related to donations to the relevant lock. As the lock is the link between donors and recipients, it seemed the most logical place to put it.

>> B2: Explain the data structure used to track priority donation.
>> Use ASCII art to diagram a nested donation.  (Alternately, submit a
>> .png file.)

Priority donation is tracked using `struct donation`, which is kept as a member of `struct lock`. Threads maintain two lists: donations made, and donations received. The `recipient` pointer in the donation can be followed to the recipient's list of donations made, and so on recursively, which allows nested donations.

```
Scenario from priority-donate-nest, with locks {a, b} and threads {L. M. H}. L holds a, M holds b. Donation operations numbered in order.

L:  a <-----------<
    ^             |
    |(1: M to L)  |(2: H to M to L)
    |             |
M:--|    b--------^
         ^
         |
H:-------|
```

---- ALGORITHMS ----

>> B3: How do you ensure that the highest priority thread waiting for
>> a lock, semaphore, or condition variable wakes up first?

For semaphores (and by extension locks) this is accomplished using a `get_max_priority_thread` function, which takes a list of threads and runs `list_max` on it using a comparator on priority. The thread is then removed from its waiting list and unblocked. Condition variables work similarly, except now there is a list of lists (as the condition variable holds a list of semaphores). This required another comparator, which compares max values in each sub-list across the parent list.

>> B4: Describe the sequence of events when a call to lock_acquire()
>> causes a priority donation.  How is nested donation handled?

In this scenario, `lock_acquire` simply manipulates the members of `struct donation`. It will adjust the list elements based on the lock holder and the current thread, augment the donation's priority value, set the `is_listed` flag (if necessary), and finally augment the lock holder's priority.

Nested donation is then handled by a recursive function, `donate_nested`. This function is called every time a donation is made. The base case is that the `struct donation` passed into it has a recipient who has made no donations. Otherwise, the function iterates through the recipient's list of donations made. Each time it finds a (now nested) donation with a lower priority than the donation originally passed into the function, it augments that donation's priority, and recursively calls itself on that nested donation.

>> B5: Describe the sequence of events when lock_release() is called
>> on a lock that a higher-priority thread is waiting for.

Here, `lock_release` will remove its `struct donation` from both the donor and receipients' lists. If the running thread no longer holds any donations, it returns to its base priority. Otherwise, it finds the max priority donation that it still holds, and lowers itself to that priority. Finally, a few bookkeeping values in `struct donation` are reset.  

---- SYNCHRONIZATION ----

>> B6: Describe a potential race in thread_set_priority() and explain
>> how your implementation avoids it.  Can you use a lock to avoid
>> this race?

As this was not an obstacle to passing any tests, it was not an issue we addressed. Since the function must know whether the current thread has received any donations before deciding whether to set both the effective and base priorities, the size of the `donations_received` list must be checked. Pintos lists are noted in the documentation as not being thread safe, so this could cause a race condition in which a thread may or may not correctly set its effective priority. Tying this to a lock would be challenging considering locks themselves depend on this list.

---- RATIONALE ----

>> B7: Why did you choose this design?  In what ways is it superior to
>> another design you considered?

The design, inasmuch as it exists for this task, is primarily based around preserving the original structure of the threading system. As such, the implementation was largely an augmentation of already existing systems.

The only significant addition was `struct donation`. Initially, that data was just kept in the lock, but eventually it made sense to abstract it into its own data structure, just to make it easier to work with. As a lock can only have one holder, and only one effective donation, it made sense to keep that data attached to the lock, and then manage lists of those donations in `struct thread`. Some prior implementations which passed earlier donation tests had more complicated logic around tracking changes in donation, but that turned out to be untenable for handling nested donations. Ultimately, it's a straightforward design which made it easier to reason about when building the `donate_nested` function.

			  ADVANCED SCHEDULER
			  ==================

---- DATA STRUCTURES ----

>> C1: Copy here the declaration of each new or changed `struct' or
>> `struct' member, global or static variable, `typedef', or
>> enumeration.  Identify the purpose of each in 25 words or less.
In the thread struct nice and recent_cpu are added. There is also a global
variable in Timer.c load_avg. All of these are used in numerous calculations
for the mlfqs scheduler. There is also a new struct fixP which holds fixed point
values in 17.14 format.

---- ALGORITHMS ----

>> C2: Suppose threads A, B, and C have nice values 0, 1, and 2.  Each
>> has a recent_cpu value of 0.  Fill in the table below showing the
>> scheduling decision and the priority and recent_cpu values for each
>> thread after each given number of timer ticks:

timer  recent_cpu    priority   thread
ticks   A   B   C   A   B   C   to run
-----  --  --  --  --  --  --   ------
 0			0		0		0		63	61	59			A
 4			4		0		0		62	61	59			A
 8			8		0		0		61	61	59			A
12			12	0		0		60	61	59			B
16		etc
20
24
28
32
36

>> C3: Did any ambiguities in the scheduler specification make values
>> in the table uncertain?  If so, what rule did you use to resolve
>> them?  Does this match the behavior of your scheduler?
It made the thread to run when the priorities are equal slightly uncertain
it would seem this would depend on the order they were added to the ready list.

>> C4: How is the way you divided the cost of scheduling between code
>> inside and outside interrupt context likely to affect performance?
Tried to do as little calculation inside the interrupt context as possible so
no ticks or interrupts are missed. This should improve performance

---- RATIONALE ----

>> C5: Briefly critique your design, pointing out advantages and
>> disadvantages in your design choices.  If you were to have extra
>> time to work on this part of the project, how might you choose to
>> refine or improve your design?
The fixed-point math is fairly clunky and makes Order of operations errors
likely as we experienced. It would be good to improve. Interrupt disables could  
likely be improved as well. Doing the design of other parts with MLFQS would possibly have helped


>> C6: The assignment explains arithmetic for fixed-point math in
>> detail, but it leaves it open to you to implement it.  Why did you
>> decide to implement it the way you did?  If you created an
>> abstraction layer for fixed-point math, that is, an abstract data
>> type and/or a set of functions or macros to manipulate fixed-point
>> numbers, why did you do so?  If not, why not?
We did create a fixP struct to hold the data type. We didn't want to be possibly
making errors in the fixed-p math by redoing the conversions every time.
			   SURVEY QUESTIONS
			   ================

Answering these questions is optional, but it will help us improve the
course in future quarters.  Feel free to tell us anything you
want--these questions are just to spur your thoughts.  You may also
choose to respond anonymously in the course evaluations at the end of
the quarter.

>> In your opinion, was this assignment, or any one of the three problems
>> in it, too easy or too hard?  Did it take too long or too little time?
It took a fairly long time 40 - 50 hours of coding for the alarm and
mlfqs portions alone. That doesn't really include time just wrapping one's head around
pintos layout and functions. But not all of that time seemed super productive.
I think a collation of frequent stupid problems from the past could have been helpful.
Slack was useful because often someone else had run into a similar problem. I think a
formalized version of this could help people focus on the learning parts as opposed to
dumb confusions.
>> Did you find that working on a particular part of the assignment gave
>> you greater insight into some aspect of OS design?


>> Is there some particular fact or hint we should give students in
>> future quarters to help them solve the problems?  Conversely, did you
>> find any of our guidance to be misleading?

>> Do you have any suggestions for the TAs to more effectively assist
>> students, either for future quarters or the remaining projects?
?

>> Any other comments?
