# Taskete &mdash; Task Parallel Library

![CMake Badge](https://img.shields.io/badge/C++-17-lightgreen)
![CMake Badge](https://img.shields.io/badge/CMake-3.10-lightgreen)
[![Spdlog Badge](https://img.shields.io/badge/spdlog-1.8.0-lightgreen)](https://github.com/gabime/spdlog/releases/tag/v1.8.0)

![CMake Badge](https://img.shields.io/badge/Windows_10-2004-lightgreen)
![CMake Badge](https://img.shields.io/badge/Visual_Studio_2019-16.7.3-lightgreen)

![CMake Badge](https://img.shields.io/badge/Ubuntu-20.04-lightgreen)
![CMake Badge](https://img.shields.io/badge/Clang-10-lightgreen)

## Description

![Graph Image](https://upload.wikimedia.org/wikipedia/commons/thumb/e/ef/Tred-Gprime.svg/168px-Tred-Gprime.svg.png)

Manages the execution of [DAGs](https://en.wikipedia.org/wiki/Directed_acyclic_graph) via a minimal, yet expressive, API.

### Features

- Create DAGs with a fluent API by dividing tasks into nodes.
- Order the execution of nodes by specifying which one runs before and after another.
- Fork/Join-like API to create multiple indipendent nodes with just few lines of code.
- DAG's private shared memory to let nodes communicate.
- Execute any kind of callable with any kind of parameters.
- Parallel execution of indipendent nodes belonging to the same graph.
- Control over all the allocations made by the library through [`<memory_resource>`](https://en.cppreference.com/w/cpp/header/memory_resource).
- Create and enqueue graphs anywhere, anytime.
- Fast and opt-in logging facilities thanks to [`spdlog`](https://github.com/gabime/spdlog).

### Constraints

- Doesn't support exceptions thrown by the node's callable.
- Graphs are static, can't be modified nor cancelled once enqueued.
- Can't specify the order of execution of different graphs.
- Doesn't guarantee ABI stability.

## Project Structure

### Dependencies

To consume this library you need:

- CMake 3.8+
- A C++ compiler that supports the C++17 standard
- A C++ standard library that supports the C++17 standard

To build and run the tests you need the aforementioned dependencies, and:

- **on Windows**, only Visual Studio 2019 is required (refer to the badge at the top for supported version).
- **on Ubuntu**, you need Clang 10 and a standard library that supports the memory sanitizers.


| Folder | Purpose |
|:-------|:--------|
| [`include`](include) | Public API |
| [`source`](source) | Internal implementation |
| [`docs`](docs) | Documentation regarding the public API |
| [`docs/internal`](docs/internal) | Documentation regarding the implementation |
| [`test`](test) | Test runners |