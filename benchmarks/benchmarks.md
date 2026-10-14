# Benchmarks
Below, you will find a benchmark of this library made using Google benchmarks.

Benchmarks of I/O operations are always messy. Fluctuations happen. Ignore any minor differences.

### Specs
The benchmarks were made using clang in release on a high-end desktop PC.

- Ran on (32 X 5500 MHz CPU s)
- L1 Data cache 48 KiB (x16)
- L1 Instruction cache 32 KiB (x16)
- L2 Unified cache 2048 KiB (x16) 
- L3 Unified cache 36864 KiB (x1) 
- The disk is a 2TB NVMe, specifically: Samsung SSD 990 EVO 2TB
- ran on linux/GNU.

Keep in mind that guideline benchmarks on a hdd disk might tell a story.

---
## Guidelines

AllOrNothingGuideline benchmarks reveal the following:

- Writing (allocate) is **13.5x** to **14x** slower than reading (Reopen). 

| AllOrNothing            | Time (ns) | CPU (ns) | Iterations |
|-------------------------|-----------|----------|------------|
| Reading Reopen          | 1748      | 1748     | 396684     |
| Writing Reopen Allocate | 24020     | 23273    | 30520      |

AppendGuideline benchmarks reveal the following:
- Writing (allocate) is about **9x** to **10x** slower than reading.
- Writing (allocate) is about **3x** slower than reading (Reopen).
- Reopening the reading stream for each read slows down operation by a factor of about **3.5**.
- Reopening the writing stream for each write slows down operation by a factor of about **1.1** to **1.25**.

| Append                  | Time (ns) | CPU (ns) | Iterations |
|-------------------------|-----------|----------|------------|
| Reading                 | 476       | 476      | 1457588    |
| Writing Allocate        | 4662      | 4182     | 162118     |
| Reading Reopen          | 1711      | 1710     | 408787     |
| Writing Reopen Allocate | 5220      | 5213     | 100000     |

IndexedGuideline benchmarks reveal the following:
- Increasing the number of blocks or the size of each section, by an equal amount, yields the same result.
- Writing is about **1.4x** slower than reading.
- Writing is about **1.5x** slower than reading (Reopen).
- Writing (allocate) is almost **11x** slower than reading.
- Writing (allocate) is about **4x** slower than reading (Reopen).
- Reopening the reading stream for each read slows down operation by a factor of about **3.1** to **3.25**.
- Reopening the writing stream for each write slows down operation by a factor of about **2.85**.
- For a given number of bytes to read, calling the read function multiple times (**227** here) instead of once is about **105x** to **110x** slower.
- For a given number of bytes to write, calling the write function multiple times (**227** here) instead of once is almost **140x** slower.

| Indexed                               | Time (ns) | CPU (ns) | Iterations |
|---------------------------------------|-----------|----------|------------|
| Reading ManyCall                      | 51113     | 51109    | 13730      |
| Writing ManyCall                      | 102245    | 102179   | 6871       |
| Reading ManyBlocks<br>Reading OneCall | 474       | 474      | 1479961    |
| Writing ManyBlocks<br>Writing OneCall | 734       | 734      | 947415     |
| Reading OneCall Reopen                | 1511      | 1510     | 462279     |
| Writing OneCall Reopen                | 2094      | 2093     | 337047     |
| Writing OneCall Allocate              | 7934      | 7087     | 100000     |
| Writing OneCall Reopen Allocate       | 8331      | 8033     | 97642      |

### How to compare results with each other?

All benchmarks with 'Reopen' take longer because they open and flush/close the file. In practice, you would do it once and then rely on functions which are not marked as 'Reopen'.

Writing benchmarks marked as 'Allocate' mean that those operations are forced to allocate new file space on top of actually writing data.

By design, AllOrNothing can't offer functions which don't reopen because it wouldn't make sense (see `learn.md`). In practice, with this guideline, you would call the writing or reading function once. With other guidelines, you would 'reopen' and then call regular functions.

Append has only `Allocate` functions because it cannot edit data. Because of how `AllOrNothing` works, it creates a copy and then allocate data. Hence, it does not provide non `Allocate` functions neither.

Please keep in mind that different guidelines are used for different use cases. This table might help you decide which fits the best but your architecture matters more, making some comparisons a bit pointless.

| Guidelines            | All Or Nothing | Append      | Indexed     |
|-----------------------|----------------|-------------|-------------|
| native write          | No ❌          | No ❌       | 1 ✅        |
| native read           | No ❌          | 2 ✅        | 1 ✅        |
| reopen write          | No ❌          | No ❌       | 1 ✅        |
| reopen read           | 3 (close) ✅   | 1 (tied) ✅ | 1 (tied) ✅ |
| allocate write        | No ❌          | 1 ✅        | 2 ✅        |
| reopen allocate write | 3 ✅           | 1 ✅        | 2 ✅        |

---
## Other exported FileGuard functions

The following function is designed to create a file.<br>
No real differences.

| File creation | Time (ns) | CPU (ns) | Iterations |
|---------------|-----------|----------|------------|
| STL           | 4471      | 4466     | 138124     |
| FileGuard     | 4179      | 4179     | 169080     |

The following function is the same as above. However, it is called when the file already exists.<br>
The STL is about 3.4 slower than FileGuard.

| File creation (already exists) | Time (ns) | CPU (ns) | Iterations |
|--------------------------------|-----------|----------|------------|
| STL                            | 721       | 720      | 965684     |
| FileGuard                      | 210       | 210      | 3320082    |

The following function zeroes a file.<br>
No equivalent exists in the STL.

| Zerioze   | size    | Time (ns) | CPU (ns) | Iterations | throughput /s |
|-----------|---------|-----------|----------|------------|---------------|
| FileGuard | 64      | 1840      | 1839     | 383151     | 33.1882Mi     |
| FileGuard | 256     | 1850      | 1850     | 378669     | 131.982Mi     |
| FileGuard | 1024    | 1848      | 1848     | 378798     | 528.563Mi     |
| FileGuard | 4096    | 1919      | 1919     | 367091     | 1.98759Gi     |
| FileGuard | 16384   | 2508      | 2507     | 281196     | 6.08603Gi     |
| FileGuard | 65536   | 5741      | 5741     | 122209     | 10.6312Gi     |
| FileGuard | 262144  | 21833     | 21832    | 32158      | 11.1826Gi     |
| FileGuard | 1048576 | 188638    | 188638   | 3691       | 5.1769Gi      |
| FileGuard | 4194304 | 1979590   | 1979553  | 355        | 1.9733Gi      |

---
## Internal functions
Those functions are functions you cannot directly use.

The following function is used to open a reading stream.<br>
No real differences.

| Open reading streams | Time (ns) | CPU (ns) | Iterations |
|----------------------|-----------|----------|------------|
| STL                  | 623       | 623      | 1119943    |
| FileGuard            | 668       | 667      | 1044911    |

The following function is used to open a writing stream.<br>
A considerable difference exists: FileGuard is 1.4 times slower than the raw STL.<br>
However, this is because those two functions do not actually do the same thing. FileGuard calls `std::filesystem::exist()` before opening the stream to prevent the creation of a file if it didn't exist. This function is designed not to succeed in that case.

| Open writing streams | Time (ns) | CPU (ns) | Iterations |
|----------------------|-----------|----------|------------|
| STL                  | 659       | 659      | 1059290    |
| FileGuard            | 937       | 937      | 745394     |

The following function is used to flush a stream.<br>
No real differences.

| Flushing streams | Time (ns) | CPU (ns) | Iterations |
|------------------|-----------|----------|------------|
| STL              | 17654 ns  | 10119 ns | 71114      |
| FileGuard        | 17751 ns  | 10103 ns | 69912      |

The following function reads from a file.<br>
No real differences.

| Reading   | size    | Time (ns) | CPU (ns)  | iterations | throughput /s |
|-----------|---------|-----------|-----------|------------|---------------|
| STL       | 64      | 250       | 250       | 2814541    | 244.239Mi     |
| STL       | 256     | 255       | 255       | 2741734    | 957.242Mi     |
| STL       | 1024    | 264       | 264       | 2635031    | 3.61042Gi     |
| STL       | 4096    | 341       | 341       | 2047847    | 11.1773Gi     |
| STL       | 16384   | 475       | 475       | 1475359    | 32.1178Gi     |
| STL       | 65536   | 1936      | 1936      | 363096     | 31.5317Gi     |
| STL       | 262144  | 7084      | 7083      | 98416      | 34.4706Gi     |
| STL       | 1048576 | 30107     | 30100     | 23132      | 32.4441Gi     |
| STL       | 4194304 | 203499    | 203498    | 3356       | 19.1956Gi     |
| FileGuard | 64      | 264       | 264       | 2605255    | 231.614Mi     |
| FileGuard | 256     | 271       | 271       | 2613356    | 901.96Mi      |
| FileGuard | 1024    | 283       | 283       | 2463880    | 3.37266Gi     |
| FileGuard | 4096    | 353       | 353       | 1980470    | 10.8071Gi     |
| FileGuard | 16384   | 484       | 484       | 1444663    | 31.5322Gi     |
| FileGuard | 65536   | 1936      | 1936      | 360472     | 31.5344Gi     |
| FileGuard | 262144  | 7027      | 7027      | 99505      | 34.7451Gi     |
| FileGuard | 1048576 | 30294     | 30294     | 22918      | 32.2361Gi     |
| FileGuard | 4194304 | 211706    | 211703    | 3420       | 18.4516Gi     |

The following function writes into a file.<br>
No real differences.

| Writing   | size    | Time (ns) | CPU (ns)  | iterations | throughput /s |
|:----------|---------|-----------|-----------|------------|---------------|
| STL       | 64      | 450       | 450       | 1607909    | 135.71Mi      |
| STL       | 256     | 435       | 435       | 1617983    | 560.651Mi     |
| STL       | 1024    | 443       | 443       | 1585786    | 2.15345Gi     |
| STL       | 4096    | 471       | 471       | 1500293    | 8.09273Gi     |
| STL       | 16384   | 572       | 571       | 1222426    | 26.7413Gi     |
| STL       | 65536   | 1441      | 1441      | 482220     | 42.367Gi      |
| STL       | 262144  | 4399      | 4399      | 158912     | 55.5042Gi     |
| STL       | 1048576 | 20942     | 20942     | 33943      | 46.6327Gi     |
| STL       | 4194304 | 143198    | 143171    | 4607       | 27.2838Gi     |
| FileGuard | 64      | 434       | 434       | 1614491    | 140.722Mi     |
| FileGuard | 256     | 437       | 437       | 1602152    | 558.508Mi     |
| FileGuard | 1024    | 447       | 447       | 1570973    | 2.13589Gi     |
| FileGuard | 4096    | 475       | 475       | 1476893    | 8.03117Gi     |
| FileGuard | 16384   | 580       | 580       | 1197373    | 26.3163Gi     |
| FileGuard | 65536   | 1518      | 1443      | 484721     | 42.2868Gi     |
| FileGuard | 262144  | 4397      | 4396      | 134894     | 55.5323Gi     |
| FileGuard | 1048576 | 20854     | 20853     | 33562      | 46.8304Gi     |
| FileGuard | 4194304 | 139400    | 139392    | 5132       | 28.0234Gi     |
