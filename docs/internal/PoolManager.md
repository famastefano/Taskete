# [Pool Manager](../../source/taskete/pool_manager.hpp)

### Purpose

Handle the lifetime of Graph and Node objects with a pool allocator.

### Design

It's a thread-safe pool allocator, that uses handles instead of raw pointers to refer to the objects.

It is composed by 3 data structures:
1. `pool`, that contains a region of allocated memory.
2. `free_list`, that keeps track of free blocks.
3. `pools`, that is a collection of `pool`.

#### Free List

At the moment it's a single linked list ordered by address.

This gives us `O(1)` time to find a free block, and `O(n)` time to mark a block as free.

#### Pools

At the moment it's a `vector<pool*>`.

In general, we want to:
- find the pool as fast as possible once we extract which one is it from the handle.
- prevent the pool relocation to keep the synchronization to the bare minimum

#### Handle

At the moment it's a bitset consisting in:
- the pool's index
- the block's index

This keeps the `handle_t` lightweight and requires simple operations to convert a handle to an address.

#### Locking

At the moment we use 2 locks:
1. the one to search/insert a pool, and
2. another one to construct/access/destroy an object inside/from a pool.
