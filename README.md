# The C++ FileGuard library ![Language](https://img.shields.io/badge/C++-23-lightgrey) 
Thanks for checking out FileGuard 1.0.0!<br>

---
## Overview
FileGuard is a modern cross-platform open-source C++23 library for safe I/O operations with files. It is designed to have a simpler and more secure API than `std::fstreams`, while maintaining similar performance.

This library has a strong focus on security, which is built on 3 aspects: error handling through the API, guidelines and encryption.

The API of this project uses `std::error_code` and `std::expected` instead of throwing exceptions or setting bit flags like `std::fstream`. This allows to centralize all errors in one place, simplifying the error handling. The library also uses modules to eliminate macros.

Guidelines are classes that allow the developer to organize files more easily. It eliminates the need of additional code and reduces the number of potential bugs. Each guideline has their own strengths and weaknesses (e.g. avoid file corruption, simplify user management).

The encryption is built on a wrapper that supports symmetric/asymmetric algorithms and custom key retrieval functions. It is compatible with most guidelines. 

---
## Prerequisites
The library is designed to be cross-platform, and it has been primarily tested on Linux, macOS and Windows. All three major compilers and their STL libraries have been tested. The `CMakeLists.txt` file was tested with Ninja and Visual Studio.

This project **requires** the following:
- a C++23 compatible compiler (including support for modules/`import std`).
- a generator/cmake version that supports modules/`import std`.

Although the following is not necessarily required, it is highly recommended to use:
- The Ninja or Visual Studio CMake generator.
- The MSVC or clang or GCC compiler.
- An IDE that supports syntax highlighting for C++23 modules.

If you are using one of the following, this project **will not** work:
- The clang-cl compiler (clang for windows) does not work. Clang requires libc++ or libstdc++ for `import std;` support, but clang-cl uses MSVC STL. Clang-cl could in theory be forced to use libc++/libstdc++ on Windows, but it defeats the point of clang-cl over mingw-clang.
- The MakeFile or Xcode generators.
- Apple clang does not work. Use LLVM clang instead (you can install it using brew)

Please note that at the time of this writing, libc++ does not support `std::move_only_function`. On linux, clang falls back to using libstdc++. On other platforms, where libc++ is used, all `std::move_only_function` are replaced by `std::function`.<br>

Here is the list of tested toolchain/Build systems:
- GCC 16:
  - on Ubuntu/Fedora with CMake 4.4.2 using Ninja, in release and debug.
  - via mingw on Windows with CMake 4.4.2 using Ninja, in release and debug.
- Clang:
  - on Ubuntu/Fedora with CMake 4.4.2 and 4.3.1 using Ninja, in release and debug (clang 22).
  - via mingw on Windows with CMake 4.4.2 using Ninja, in release and debug (clang 22).
  - via brew on macOS with CMake 4.4.2 using Ninja, in release and debug (clang 23)
- MSVC 19:
  - on Windows with CMake 4.4.2 using Ninja and Visual Studio, in release and debug.

---
## Set up

The first way of setting up this library is to copy and paste the folder modules/FileGuard into your own project. Be sure to respect the prerequisites.

The second way is to fetch the library with CMake (recommended). Below is a snippet that you can use.
```cmake
# cmake_minimum_required(...)

# The following should be put after the call to cmake_minimum_required
# If you are using ninja and cmake version 4.4.X or lower, this step is mandatory. Use the correct UUID for your version. 
# Otherwise if you are using Visual Studio OR cmake 4.5 even with ninja, you can omit this whole block.
if (CMAKE_GENERATOR MATCHES "Ninja")
  if (CMAKE_VERSION VERSION_GREATER_EQUAL "4.4.0")
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
  elseif (CMAKE_VERSION VERSION_GREATER_EQUAL "4.3.0")
    set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")     
  # Other cmake versions if needed
  endif ()
endif ()
# And it should be put before the call to project

# project(...)

# C++23 or higher must be set.
set(CMAKE_CXX_STANDARD 23)

# ...

include(FetchContent) # Not needed if you already included it.

FetchContent_Declare(
    FileGuard
    GIT_REPOSITORY https://github.com/omegaDIL/FileGuard.git
    GIT_TAG        v1.0.0
)
FetchContent_MakeAvailable(FileGuard)

# ...

# add_executable(YourProject main.cpp) # This is your executable/library. 
target_link_libraries(YourProject PRIVATE FileGuard::FileGuard) # After "add_executable/library". "YourProject" should be replaced by the actual name of your project.
# ...

# ...
```
---
## Features & learning
check out learn.md for a detailed explanation of the library's features and how to use them after reading this section.

This project introduces the module FileGuard. All identifiers are defined in the `FileGuard` namespace.

API overview:
- 3 guidelines (`AllOrNothingGuideline`, `IndexedGuideline`, `AppendGuideline`).
- The `EncryptionWrapper` class.
- The `g_toyWrapper` global variable, which is a `EncryptionWrapper` toy instance.
- The `createFile()` function.
- The `basicFileZeroizing()` function.
- The `zeroizeValue()` function.
- The `ErrorCode` enum class that represents all FileGuard custom error code.
- The `Category` that defines a custom error category for `std::error_code`.

This is everything that FileGuard exports.

### Guideline
A Guideline allows you to organize a file the way you want by wrapping `std::fstream`. There are multiple guidelines to cover more potential cases. Each has their strengths and weaknesses. Those are the core of FileGuard.

Which guideline should you use?

| Guidelines               | All Or Nothing                 | Append                              | Indexed                                |
|--------------------------|--------------------------------|-------------------------------------|----------------------------------------|
| native encryption        | Yes ✅                         | Not natively ❌ (workarounds exist) | Yes ✅ (but slows down I/O operations) |
| edit data                | mid 🟡(needs complete rewrite) | Never ❌                            | Yes ✅                                 |
| corruption free          | Excellent ✅                   | Mostly[0] ✅                        | No ❌                                  |
| suitable for large files | No ❌                          | Yes ✅                              | Excellent ✅                           |
| efficient for writing    | No ❌                          | Yes ✅                              | Yes ✅                                 |
| efficient for reading    | Yes with no restoration 🟡     | Yes ✅                              | Yes ✅                                 |
| ease of use              | Yes ✅                         | Usually[1] ✅                       | Mid 🟡 to Hard[2] ❌                   |
| error handling           | expected/error code ✅         | exepected/error code ✅             | expected/error code ✅                 |

- **Mostly[0]**: Whether the append guideline can be considered corruption free depends on how you use it and what you 
consider to be safe. Any data previously written will NOT be corrupted. However, partial write can happen (usually 
after power outage) but is *extremely* rare.<br>
- **Usually[1]**: lifetime is a bit hard to manage.<br>
- **Mid/Hard[2]**: The "hard" nature of this guideline relies entirely on the fact that you manage indexes. Out-of-bounds
errors are one of the most notorious vulnerabilities. I don't trust you, and you shouldn't trust yourself. Therefore,
this is marked 'hard' to encourage extra caution.<br>

### Encryption
The `EncryptionWrapper` class is what allows you to encrypt and decrypt data. You can plug your own encryption algorithm and key retriever function to use with the wrapper.

Guidelines generate multiple nonces to ensure that the same data encrypted multiple times yields different ciphertexts. The current main limitation of using wrappers with guidelines is that you cannot change the nonce generator function. If you need a specific one, you will not be able to encrypt stuff using the wrapper argument that is passed to guidelines.

The global variable `g_toyWrapper` should NOT be used for real encryption purposes. This instance is a toy to play around. If you want usable algorithms, you can use the [libsodium library](https://github.com/jedisct1/libsodium) for example.

---
## Author & License 
This library was created by Lesaffre Ismael, under the pseudonym "[omegaDIL](https://github.com/omegaDIL)".<br>
email: Contact me at <lesaffreismael@proton.me><br>
repo: Find FileGuard on GitHub [here](https://github.com/omegaDIL/FileGuard).

This project is licensed under the Boost software license, version 1.0. See the LICENSE.txt file for details.<br>

Simply put, you can:
- use it for commercial, non-commercial or personal purposes
- share/distribute it
- edit it and share your version
- publish binary files without credits

The only thing required is to keep the copyright notice in your own source code for open source projects.

--- 
## LLM usage during development
Although this library is not LLM-free, this project has a strict restriction on the use of LLMs during development. LLMs had little to no impact on the project.

No subscriptions or money were paid to any LLM companies during the development of this library.

LLMs were primarily used:
- as an extended search engine.
- to fix grammatical errors in the project.
- as a debugging tool.
- to suggest a fix for a few functions (not entire files or classes).
- to generate the installation process for compilers in the `CI.yml` file.
- to generate some parts of the `CMakeLists.txt` file, mainly the one that fetches catch2 and google benchmarks

LLMs did NOT:
- engineer the library and its architecture.
- generate whole source files, classes or functions.
- generate the documentation for the library.
- generate test cases* or benchmarks.
- generate any other files.
- generate ideas**.

*test cases: the only exception is the test case for the entropy of the nonce generator. It was initially generated by a LLM, but was reviewed and refactored.<br>
**generate ideas: The only exception is the `AppendGuideline` class. Originally, LLMs did give the idea for a guideline that appends data.

--- 
## Maintenance policy
The library is maintained by Lesaffre Ismael.

This project will be maintained as long as it is useful to the author. While it is realistically impossible to say that this library will be maintained forever, FileGuard is guaranteed to be maintained at least until September 2029 for now. It will likely extend beyond this point.<br>
If the project stop being maintained, it will be clearly stated in the README.md file of the latest release.

This project will have updates other than bug fixes to add new features or improve existing ones, at least until September 2029. After that, it will depend on the author's availability and interest. The README.md file will be adjusted accordingly to reflect whether new features will be added to the project.<br>
Until September 2029, the goal is to have at least one important update per year.

ABI stability will be broken for major updates.

The usage of LLMs will remain similar for future updates and will be clearly stated in the README.md file.

If you find any issues, feel free to open a GitHub issue (even for typos).

Pull Requests are closed for now. You may send an email though.

### Here are potential future updates
While none of these features are guaranteed, all are likely to be implemented over time based on priority and technical complexity.

**Core features**:
- A encryptCopy() function that copies a file and encrypts it.
- isEnoughSpaceAvailable(nbCharacter) that checks if there is enough space on disk.
- Functions that convert files from one guideline to another.
- adding a new guideline: noGuideline which simply wraps fstream and call FileGuard's functions to flush, close, open... instead of the raw fstream member functions.
- being able to change the nonce generator. *(requires ABI break)*
- considering whether to export the function that checks if a AllOrNothing file is corrupted.

**Performance-related**:
- considering ways to increase performance for AllOrNothing::write(). *(requires ABI break)*
- implementing a buffer in reading/writingStream to increase perf of those functions when they need to read a lot of data (see `benchmarks.md`). *(requires ABI break)*
- INTERNALS: replacing `std::fstream` by `std::print`/`std::FILE*` where possible in order to increase perf. *(requires ABI break)*
- INTERNALS: having a function that reads data until eof and make AllOrNothing use it. *(requires ABI break)*

**Security-related**:
- using C++26 contracts instead of NOEXCEPTDEBUG functions and other. *(requires ABI & API breaks)*
- including libsodium to have a secure `EncryptionWrapper` instance by default.
- Depending on the way the restoration of AllOrNothing fails (e.g. permissions for renaming/removing files), the content may be essentially lost unless the user opens the file themselves to extract their data. To be fixed. *(requires ABI break)*

**Refactoring/Simplification**:
- see if the `EncryptionWrapper` constructor should be constexpr and use pointer to functions instead of `std::function`s. *(requires ABI & API breaks)*
  - if not, change the argument order so we can change the nonce size without giving the encryptor twice, first as the encryption and then as the decryption. *(requires ABI & API breaks)*
- split functions of AllOrNothing in 2 categories: w/ and w/o encryption to eliminate a branch and clarify code. *(requires ABI & API breaks)*
- adding an argument to `IndexedGuideline::write()` which fills the input string with null character if it does not fit the size (when encryption is not enabled). *(requires ABI & API breaks)*
- changing the argument type of readStream/overwriteStream instead of uint32 to allow bigger I/O operations (allows to change the template types of the indexedGuideline for example). *(requires ABI break)*
- encryption wrappers should use `std::byte` instead of `std::uint8_t` (to be checked).
- INTERNALS: creating a new file (.cppm & .cpp) where exported functions are implemented (e.g. like `createFile()`, `FileGuard::Category`, other new functions).
- INTERNALS: moving the zeroize function to the `EncryptionWrapper.*` file.

**Toolchain support and testing**:
- adding benchmarks on hdd disks.
- measure branch misprediction (e.g. using perf stats on linux).
- proper testing will be brought to XCode generator when Apple decides to support `import std`.
- editing the `CMakeLists.txt` to use libc++ with clang when libc++ supports `std::move_only_function`. Then delete the macro `STLFUNCTION` in `Encryption.cppm` + header version. *(requires ABI break)*
- if clang-cl is ever capable of running import std using msvc stl on Windows, proper testing will be brought as well.
- declaring getInstance for FileGuard::Category constexpr when mingw gcc supports it.
- for now, the library is static.
- simplify the configuration for llvm clang on macOS in CI.yml (at the time of this writing, there is currently a bug)
- INTERNALS: completing the test coverage for all unhandled error branches left.
- INTERNALS: consider splitting all guidelines benchs in their own benchs file











