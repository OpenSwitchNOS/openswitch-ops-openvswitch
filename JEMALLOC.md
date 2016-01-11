# jemalloc as Default Memory Manager for OVSDB-Server

OVSDB-Server do a lot of memory related operations. Many of those operations are related to
allocating small pieces of memory.

From our tests, we found that replacing GLibC memory allocator with jemalloc can improve significantly
the performance of the OVSDB-Server, and also, in some cases, reducing the memory usage.

## Implementation

The change was included in several parts:
- A yocto recipe for download, compile and install jemalloc 4.0.4 in the image.
- A change to ops-openvswitch configuration flags, to add --enable-jemalloc as default.
- A change to ops-openvswitch's configure.ac and automake.mk, to compile ovsdb-server
  linked with jemalloc, when given the --enable-jemalloc flag.

## Notes for Daemon Developers

As of now, only ovsdb-server links with libjemalloc by default. That means that the IDL, probably used
by your daemon, still uses GLibC memory allocator.

Changing to jemalloc is very easy. There are two options, one that requires to recompile the daemon 
(and changing the building instructions), and other that just need to change how the daemon is loaded
(systemd scripts).

### Recompiling way

In this case we need to add libjemalloc as a library. With gcc we can use the flag -ljemalloc. It's 
necesary that your yocto recipe includes jemalloc as a RDEPENDS and DEPENDS dependency.

### Systemd script
In this case add the following line (under [Service]) to your script:

    Environment="LD_PRELOAD=/lib/libjemalloc.so.2"

The daemon's yocto recipe also needs to specify jemalloc as a RDEPENDS dependency.

## Performance Improvements
We compare GLibC performance and memory usage with jemalloc and tcmalloc, using several tests (insertion, updates, PubSub and transaction size). The results are favorable to jemalloc.

- Update 1: Each of 10 parallel workers do 25000 updates over 1000 rows.
- Update 2: Each of 10 parallel workers do 25000 updates over 200000 rows.
- Insert: Each of 10 parallel workers insert 25000 rows.
- Message Queue Simulation: One producer waits for requests (requested by 10 parallel workers). With each request
  the producer updates 512 rows, composed of one map column with 256 elements.
- Transaction Size: The program inserts 100, 1000, 10000, 100000 and 500000 rows, alternating between inserting
  all the rows in the same transaction, or inserting one row per transaction.


| Test     |   GLibC   |  jemalloc  |  tcmalloc  |
|----------|-----------|------------|------------|
| Update 1 |   28s     |    24s     |    31s     |
| Update 2 |   57s     |    38s     |    33s     |
| Insert   |   38s     |    32s     |    27s     |
| Queue    |    4s     |     4s     |     4s     |
| Size     |  129.92s  |   119.87s  |   114.21s  |
|            Duration (seconds)                  |




| Test     |   GLibC   |  jemalloc  |  tcmalloc  |
|----------|-----------|------------|------------|
| Update 1 |    7.35   |    18.92   |   171.42   |
| Update 2 | 1090.7    |    82.88   |   111.08   |
| Insert   |   66.66   |    57.86   |   127.56   |
| Queue    | 1094.40   |    28.13   |   270.47   |
| Size     |  410.17   |   116.50   |   114.21   |
|        Memory Usage (RSS, megabytes)           |

From this tests we considered that jemalloc was consistently better than GLibC in performance and
memory usage. TCMalloc was faster in some benchmarks, but uses a lot more of RAM than jemalloc.

