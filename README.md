# ⚡ Build Your Own Redis — From Scratch in C/C++

<div align="center">

![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)

**A Redis-compatible key-value server implemented from scratch — no libraries, no frameworks, just raw C/C++ and Linux system calls.**

</div>

---

## 🎯 What Is This?

A fully functional Redis-compatible key-value server built from the ground up in C/C++, following [Build Your Own Redis](https://build-your-own.org/redis/) by James Smith. Every component — from TCP socket networking to data structures to memory management — is hand-implemented.

> *"What I cannot create, I do not understand."* — Richard Feynman

---

## 🏗️ Architecture & Components

### Part 1: Networking Foundation
| Component | What I Built |
|---|---|
| **TCP Sockets** | Raw socket programming with `socket()`, `bind()`, `listen()`, `accept()` |
| **Client-Server Protocol** | Custom binary-safe request-response protocol with length-prefixed framing |
| **Non-Blocking I/O** | Poll-based event loop for handling thousands of concurrent connections without threads |
| **Event Loop** | Single-threaded, multiplexed I/O loop (similar to Redis's actual architecture) |

### Part 2: Data Structures (All From Scratch)
| Data Structure | Implementation Details |
|---|---|
| **Hashtable** | Chained hashing with progressive resizing (amortized O(1) insert during resize) |
| **AVL Tree** | Self-balancing binary search tree backing sorted sets — O(log n) insert, delete, rank |
| **Sorted Set** | Dual-indexed structure (hashtable + AVL tree) supporting both key lookup and range queries |
| **Heap** | Min-heap for TTL-based cache expiration with O(log n) insert and O(1) min extraction |

### Part 3: Systems Programming
| Feature | Details |
|---|---|
| **Data Serialization** | Custom binary serialization protocol (strings, integers, arrays, errors) |
| **TTL Expiration** | Heap-driven lazy + active expiration system for cache entries |
| **Timer & Timeout** | Millisecond-precision timers for idle connection cleanup |
| **Thread Pool** | Multi-threaded worker pool for offloading blocking I/O operations |

---

## 🧠 What I Learned

- **Network Programming:** Raw TCP sockets, non-blocking I/O, poll/epoll event loops — the foundation of every database, HTTP server, and distributed system
- **Data Structures in Practice:** Not textbook exercises — real hashtables that resize under load, real AVL trees that maintain balance during concurrent operations
- **Systems Thinking:** Memory layout, cache-friendly data access, amortized complexity analysis, and the trade-offs that production systems make
- **C/C++ Discipline:** Manual memory management, pointer arithmetic, buffer handling, and defensive coding against undefined behavior

---

## 🔧 Building & Running

```bash
# Compile the server
make

# Run the server (default port 1234)
./server

# In another terminal, run the client
./client
```

### Supported Commands
```
SET key value
GET key
DEL key
KEYS
ZADD key score member        # Sorted set add
ZRANGEBYSCORE key min max    # Sorted set range query
PEXPIRE key ms               # Set TTL in milliseconds
PTTL key                     # Get remaining TTL
```

---

## 📚 Reference

Built following [Build Your Own Redis with C/C++](https://build-your-own.org/redis/) by James Smith — an excellent book that teaches network programming, data structures, and low-level C through building a real system.

---

## 👤 Author

**Mohamad Atamneh** — Software Engineer | B.Sc. Software Engineering (GPA: 89)

[![LinkedIn](https://img.shields.io/badge/LinkedIn-0077B5?style=flat-square&logo=linkedin&logoColor=white)](https://linkedin.com/in/mohamad-atamleh-a43185381)
[![GitHub](https://img.shields.io/badge/GitHub-181717?style=flat-square&logo=github&logoColor=white)](https://github.com/MohamadAtamneh404)
