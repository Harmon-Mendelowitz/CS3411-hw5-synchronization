# Synchronization Specification

In this assignment, you'll implement the synchronization primitives you've learned about in class, and used in lab.
We'll break down our discussion about these primitives into *spin-based* techniques, and *block-based* techniques.

## User-Level Interface

The main lock API is somewhat similar to the `pthread`s API, and focuses on being relatively simple.

```
#define MAX_NUM_LOCKS 16
#define LOCK_ADAPTIVE_SPIN 1024

typedef enum {
	LOCK_SPIN,
	LOCK_BLOCK,
	LOCK_ADAPTIVE,
} lock_type_t;

struct lock {
	int lockid;
	lock_type_t type;
	/* add your implementation here... */
};

int  lock_create(struct lock *l, lock_type_t type);
int  lock_take(struct lock *l);
int  lock_release(struct lock *l);
void lock_delete(struct lock *l);
```

You should place these in `ulock.h` (for user-level lock), and all of your user-level implementation should be either in that file, or in `ulib.c`.
Each lock is identified by a unique integer (`lockid`).

- `lock_create` returns `0` on success and initializes the `struct lock *`.
	It takes the *type* of the lock that is being created.
	You'll support spin-based locks (`LOCK_SPIN`), block-based locks (`LOCK_BLOCK`), and adaptive locks that combine both (`LOCK_ADAPTIVE`).
	If an error is encountered (i.e. the system already has `MAX_NUM_LOCKS`), it returns `-1`.
- `lock_take` takes as its argument a lock that was previously created by `lock_create` within this process, or within a parent of this process.
	Once this function returns without error (`0`), the current thread has a critical section.
	This function can return an error (`-1`) if the calling thread *already holds the lock*!
- `lock_release` releases the critical section it has previously taken, returning `0`.
	Return `-1` for an error if we do not already hold the lock.
- `lock_delete` deallocates the kernel part of the lock, and makes it inaccessible to the current process (i.e. its lock id can no longer be used to access that lock).
	If the lock is currently held by the thread doing the `lock_delete`, this also releases it.

## Lock Implementation Details

Locks have four main components.

1. The `struct lock` that is used by user-level to track whatever state is required there (esp. for spin-based implementations).
1. The lock id which is an integer identifier that the user and kernel use as a shared means to identify the lock.
1. The kernel's data-structure associated with each process that includes the locks that process has access to.
1. The kernel's data-structure which tracks *all* of the system's locks (i.e. `MAX_NUM_LOCKS` locks).

**Lock ids, and process lock tables.**
Lock ids, should be used at the system call layer to identify which lock user-level is trying to operate on.
Lock ids are simple integers that identify one of a process's locks.
Each process has a "lock table" which is just an array of the locks that it has open (very much like the open file array in processes).
For simplicity, you might as well make the lock id be the lock's offset into this table.

**Lock type.**
The `type` of a lock determines how it interacts with the kernel.
If it is a `LOCK_SPIN`, then it *never* makes system calls, and instead implements spin-based critical section contention management.
See `cas.h` for a compare-and-swap function you can use.
If the type is `LOCK_BLOCK`, then the lock implementation will make system calls for all operations, and the kernel will block the thread when there is critical section contention.
For `LOCK_ADAPTIVE`, if the critical section is contended, the implementation will spin at user-level `LOCK_ADAPTIVE_SPIN` times, and if the lock is still contended, it will call the kernel, using the blocking implementation.

It is important to understand in which situation, each will be most useful.
One of the main discriminating factors is the number of cores active in the system, and the other is how many contending threads there are.
If `CPUS = 1` (`CPUS=1 make qemu`)?
If the number of contending threads is much greater than `CPUS`?
If the number of contending threads is less than than `CPUS`?

**Interaction with `fork`.**
When a process forks, you must copy the lock table of the parent into the child.
If a parent has open locks when they fork, the child should be able to access those locks.
This means that multiple processes can have references to the same lock.
When any of the processes `exit`s, or call `lock_delete`, they will remove the lock from their process's lock table, but only when the *last process* that has a reference to the lock `exit`s or calls `lock_delete` is the actual lock in the kernel reclaimed for future lock allocations.

## Example

Locks provide critical sections, but I'm not assuming for this assignment that your thread implementation is complete.
So the natural question is what is the "shared resource" if it isn't something in global, shared memory?
We are going to use `pipe`s.
These are message passing channels that are asynchronous, but with a fixed size (`512` in `xv6`).
We are going to have a number of child processes that are all writing a single byte at a time to a shared pipe.
Our "critical section" is to ensure that each of the child processes must write in batches of three bytes to the pipe.
Without locks, the children will interleave their access to the pipe, and possibly preempt each other's access to the pipe, thus interleaving their access to the pipe at granularities less than three.

Please see `lock_example.c` for an example of how critical sections are used to protect multiple groups of writes to a shared pipe between child processes.

## Kernel Implementation

Each lock will need a kernel data-structure to track its state.
As `xv6` discourages dynamic allocation, we are going to statically allocate an array of locks for all processes in the system to use (much like there is a static array of processes).
This array has `MAX_NUM_LOCKS` locks in it.
Additionally, you'll need to track per-process, all of the locks that have been created by that process (or were inherited from a parent) as discussed earlier.

# Leveling Up!

Your implementation will receive credit by implementing a number of successive levels:

- **Level 0 (5%).**
	Make the `lock_example.c` compile when you uncomment the `//#define LOCKS_IMPLEMENTED` line.
- **Level 1 ().**
	Mutual exclusion works given the lock API with `type = LOCK_SPIN`.
	This implementation should include no system calls, and you must use the `cas` function within `cas.h`
- **Level 0 ().**
- **Level 0 ().**

# Suggested Reading

**Locks.**
I suggest that you understand *all* of the existing lock interfaces (as discussed in lab, this includes `spinlock`s and `sleeplock`s), and the `wakeup`/`sleep` interface (which is similar to condition variables) in `xv6`.
There is a relatively simple way to implement the blocking functionality in the kernel, if you understand these APIs well.

**Lock tables.**
To understand how to implement the per-process lock tables, and properly interact with `fork` and `exit`, you can look at the file table implementation for existing `xv6` processes.
These are the list of open files for each process.
Just as with our locks, open file descriptors (similar to lock ids) are inherited across `fork`s, and only when the last reference to a file is removed (via `exit` or `close`), will we drop our access to the file.

**Global lock table.**
You have the single global array of locks that are used to allocate (`lock_create`) locks in your system.
This is very similar to how processes exist in a big array until we need to allocate them (`fork`).

# Extra Credit
