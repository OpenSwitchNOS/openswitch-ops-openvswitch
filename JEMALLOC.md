# JEMalloc as Default Memory Manager of OVSDB Server

The OVSDB Server is very memory intensive. It requests and releases memory continually, as it creates and destroy objects like dynamic strings, json objects, etc.

This high frecuency allocations and deallocations produce performance problems, because, as the profiling shows, a lot of time is spent on memory related functions.

With the intention to reduce that time, the memory manager was changed to jemalloc, because:

* JEMalloc perform better than GLibC when allocating and deallocating lots of small objects.
* This change doesn't requiere to modify code.

Swapping the memory manager results in important savings, both in memory use and in time.

The testing with our benchmark tool got the following results:

<table>
<tr><th>Test</th><th>GLibC Duration (s)</th><th>JEMalloc Duration (s)</th><th>GLibC Memory Usage (RSS, MB)</th><th>JEMalloc Memory Usage (RSS, MB)</th></tr>
<tr><td>Update (25k rows, 10 workers, 1000 updates)</td><td>28s</td><td>24s</td><td>7.36MB</td><td>18.93MB</td></tr>
<tr><td>Update (25k rows, 10 workers, 200k updates)</td><td>57s</td><td>38s</td><td>1090.68MB</td><td>82.89MB</td></tr>
<tr><td>Insert (25k rows per worker, 10 workers)</td><td>38s</td><td>32s</td><td>66.66MB</td><td>57.69MB</td></tr>
<tr><td>PubSub 2 (1000 updates, 512 rows, 10 subscribers)</td><td>3s</td><td>3s</td><td>1108.33MB</td><td>28.13MB</td></tr>
<tr><td>Transaction Size</td><td>126.92s</td><td>119.87s</td><td>410.17MB</td><td>116.50MB</td></tr>
</table>

For this replacement, also TCMalloc was considered and tested, but it has a less predictable behaviour, and a significantly bigger memory usage.
