# [LockfreeRingbuffer](../../source/taskete/lockfree_ringbuffer.hpp)

### Purpose

It was designed to be used by the Workers as their queue.

### Design

While it is designed as a "standalone" component, its design depends on how the Worker's queues were meant to be used.

The only constraints we have for now are that the element type shall be:
- Default Constructible
- Copy Assignable
- Trivially Destructible

This allows us to not care about construction/destruction of each element everytime we read/write into the ringbuffer.

#### Fixed Size

Because we decided that each Worker has a separate queue that won't be resized.

#### Lockfree

It weren't a requirement but emerged from the fact that we use 2 indipendent cursors to track the Producer/Consumer position.

So we opted for `std::atomic<T*>` for the cursor type.

#### Thread-safe

This were required because while the Worker is still _consuming_ data from its queue, the Scheduler _might produce_ additional data.

#### Contiguous

Like the Lockfree feature, it emerged later and this time due to the fixed-size constraint.
