# [Shared Memory](../../source/taskete/shared_memory.hpp)

### Purpose

It was designed to allow the user to share data between nodes of the same graph.

In particular it handles:
1. Memory management, allocation/deallocation etc.
2. Object construction, allocating memory, proper alignment etc.
3. Object access, to retrieve objects that have been created
4. Object destruction, after the graph is destroyed

### Design

It consists in 3 part:
1. Memory storage, that contains the data
2. Memory access, that manage how memory is accessed by the user
3. Memory destruction, that cleanups the memory storage

#### Memory Storage

Each object is stored inside a container that keeps track of:
- A _Key_ to identify the object
- _Where_ the object has been constructed
- _Which_ memory resource has been used to allocate it
- _How_ to properly cleanup the object

#### Memory Access

To distinguish the objects we decided to use an integer as the Key.

This works very well with string hashes, as it's more readable than plain integers:

```cpp
template<typename T>
[[nodiscard]] T* get(uint64_t key) const noexcept;

template<typename T, typename... Args>
[[nodiscard]] T* get_or_construct(uint64_t key, Args&&... args);

/////////

Example:

int* idx = shared_memory.get<int>("shared_index"_hash);
```

The common case is that the user uses existing objects more than constructing new ones, that's why we've chosen to use a `shared_mutex`. We block _only_ when we need to construct a new object.

We don't perform any kind of synchronization of the underlying object, but we let the user handle this in any way they prefer.

#### Memory Destruction

Once a graph is not needed anymore, it gets destroyed.

This means that we also need to destroy all the objects constructed by the user.

This is done by the `TypeErasedDestructor` class, that:
1. calls the object destructor
2. deallocates the object
