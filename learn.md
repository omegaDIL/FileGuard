# Learning FileGuard
This library was designed to be simple and easy to learn. However, as always, there are some gotchas that you may need to be aware of.

An API overview is available in the `README.md` file. For the complete API, you can use Doxygen to generate it. A `Doxyfile` file is present at the root folder.

Reading the whole `Features & learning` section of the `README.md` file before `learn.md` is  encouraged because it gives you basic information not necessarily included in this file.

While it is recommended to read this whole file, you can still jump to each section whenever you want as they are independent of one another. That being said, you should begin this file by reading the next section "*A few things to know first*".

---
## A few things to know first
- This library is NOT thread-safe regarding operations performed on a same file. For different files, there is no reason not to work as everything is independent/thread local (including errno), but this has not been tested.
- Functions don't check if there is enough file space available beforehand. Failures will be reported through `std::error_code` anyway.
- The documentation for functions follow a specific order for preconditons/postconditions/consequences. See below 
- Unless stated otherwise, the only `std::error_code` indicating success is `0`.
- A function that "fails" is a function that returns a non-successful `std::error_code` (which is almost always anything except `0`), not necessarily a function that crashes or throws.
- Functions return success when their jobs are done. This means that if those jobs were already done in the first place, they will immediately return the success code. See `createFile()` for example.
- Choose ONE guideline per file. Don't try to use different guidelines for one file, you'll probably either write garbage or read garbage. The same goes for the encryption process.
- Always implement error handling for the functions that you use. Otherwise, it defeats one of the main point of this library.

### Understanding how preconditions/postconditions are documented
Most functions come with their preconditions that are stated in the documentation. Some preconditions are grouped together because they share the same postconditions and consequences.

A function may or may not have multiple groups of preconditions. The groups are separated by a blank line. If a precondition is violated, the postconditions of the according group won't be respected, and the consequences will be triggered.

Consequences can take the forms of multiple (or one) `\note`, `\throw` or `\warning` clauses depending on the gravity and effects. They will be appended at the end of each group. Different consequences will usually be triggered depending on the build mode

Put simply, each group has the preconditions first, followed by their postconditions, and then by the consequences.

`\note` is the most common clause to describe consequences. `\throw` are rarely used. `\warning` are used when the function asserts.
```
/**
 * ...
 *
 * \pre  `value` must not be equal to 0.
 * \pre  `value` must be lower than 192.
 * \post this function will do something.
 * \note Otherwise, this function will fail.
 *
 * \pre ptr must not be nullptr.
 * \post the pointed object will be modified.
 * \throw std::invalid_argument In release, otherwise. 
 * \warning otherwise, asserts in debug.
 * 
 * \note This note is a regular note clause, not a consequence of a precondition because there is no \pre clause above.
 * ...
 */
```
In this example, you can see two groups because there is one blank line between `\pre`/`\post` clauses. The last note is not a precondition group as there is no `\pre` clause at the beginning (this is a regular note). Contrary to `\pre`, `\post` are not required to form a group.

The first group requires the value to be within a certain range (preconditions) for the function to do something (postcondition). If it is not respected, the function will fail (consequence). Remember that a function failing usually means returning a non-successful `std::error_code`.

The second group requires ptr not to be `nullptr` (precondition) so the function can access it (postcondition). Otherwise, a consequence will happen. You can see that this consequence depends on the build mode. Please note that this is the kind of function to be marked `NOEXCEPTDEBUG` (see below).

Again, groups are supposed to be independent (but real-world operations, especially with I/O interactions, are a bit messy).

### Handling `NOEXCEPTDEBUG` functions
Some functions, mostly constructors, are marked with a macro named `NOEXCEPTDEBUG`. This macro is equal to `noexcept` in debug and nothing in release.
```c++
#ifndef NDEBUG
#define NOEXCEPTDEBUG noexcept
#else
#define NOEXCEPTDEBUG
#endif //NDEBUG
```
This means that a given function is guaranteed not to throw in debug, but loses that guarantee in release mode. In this library, this means that in debug, those functions assert instead.<br>

`NOEXCEPTDEBUG` functions are used instead of `std::error_code`/`std::expected` when such types cannot be used. This is true with constructors because they don't have return values. They are also not used when errors are so uncommon that it wouldn't make much sense to implement them and likely mean that the developer made a mistake (thus it is simpler to assert/throw). 

**How should you handle these then?** `NOEXCEPTDEBUG` functions are an alternative to simple asserts. Most libraries assert in debug and do nothing in release because `assert()` is disabled.

In general, you should handle these the very same way you handle those functions: don't put any error handling and see if it actually asserts in debug mode. Simply ignore release mode, exactly how you do with most libraries. This will work 99% of the time. 

However, there is the remaining 1% of the time. That covers the cases where testing in debug mode is not sufficient (possibly because inputs are too deeply tied with the user). With regular asserts, this is a problem. With `NOEXCEPTDEBUG`, you can simply wrap function calls with try-catch statements.

With C++26 coming, the macro `NOEXCEPTDEBUG` will likely be replaced by contracts. 

### Choosing what to use
Choosing which guidelines to use for a file depends on what you want to do with it. Check out the section `Features & learning` (precisely the subsection `Guidelines`) in the `README.md` file to understand the strengths and weaknesses of each guideline. Below you will learn more details about them. Once you understand everything, it is recommended to read the documentation of the guideline you're about to use.<br>
Check out `benchmarks/benchmarks.md` for guidelines benchmarks.

Choosing whether to use the `EncryptionWrapper` class depends on if the data you save should be encrypted. If so, create your own instance (see below). Whatever you do, please, DO NOT USE `g_toyWrapper` for real encryption purposes (unless you want to hide data from your little brother who doesn't know anything /s).

---
## Encryption
### Using the encryption class

To use the `EncryptionWrapper` class, you must create an instance. For demonstration purposes, a global variable exists in the namespace FileGuard named `g_toyWrapper`. Never use it for real encryption purposes.

The `EncryptionWrapper` class uses your own algorithms and key retriever.

Using an instance is straightforward, you have to use `.generateKey()` before encryption/decrypting anything. If you don't, an error will be returned (`FileGuard::ErrorCode::emptyKey`). This sets up the key internally.

Once you're done encrypting, the key must be clear. Although the destructor of the `EncryptionWrapper` class clears and zeroes the key (RAII), you shouldn't always rely on it. Some attacks exploit that the possibility that the key lives in memory longer than necessary. So, clear it using `.clearKey()` to ensure proper zeroization.

To actually encrypt, use the functions `.encrypt()` and `.decrypt()`. They will return the error code of your algorithm. They need the nonce as argument. To generate this nonce, use the `getCryptoNonce()` static function. You have to pass the size of your nonce (use the member function `.getNonceSize()` on your instance). If the size is not valid, `FileGuard::ErrorCode::invalidNonce` will be returned.

There are actually two other functions to encrypt/decrypt named `encryptAndAddNonce()` and `decryptWithNoneAppended()`. Those functions behave the same way except that they append the nonce at the end of your encrypted data. `decryptWithNoneAppended()` then expects the nonce to be at the end of the data you want to decrypt (it does not ask for a nonce as an argument). 

For sensitive data, you should clear or zeroize the original non encrypted (or decrypted) data if needed. Use the function `zeroizeValue()` in that case.

### Implementing your own algorithm

To create your own instance, you'll have to create 2 or 3 functions depending on the symmetry of your algorithm.

The first one is the key retriever. The function should have the following definition:
- The return type is `std::error_code`.
- The first argument is a `std::string_view`.
- The second argument is a `std::vector<std::uint8_t>`.

The first argument is used as the "info viewer". It allows you to use external data to generate the key like a server port to connect to.

The second and third one is the encryptor. The function should have the following definition:
- The return type is `std::error_code`.
- The first, second and third arguments are `std::span<const std::uint8_t>`.
- The fourth argument is `std::string&`.

The first argument is the data. The second is the key. The third is the nonce. The fourth argument is the output string.

If you are using an external library, you'll need to wrap the functions.

Then, you can instantiate the objects:
```c++
FileGuard::EncryptionWrapper asymmetric{ keyRetriever, encryptor, decryptor, 32 };
// encryptor and decryptor are of the same types. Those are functions but obviously, without parenthesis.
// The last argument is the nonce size, by default 12.

FileGuard::EncryptionWrapper symmetric{ keyRetriever, encryptor };
// No third function. The wrapper knows it is a symmetric algorithm.
// The nonce size is 12 (by default).

FileGuard::EncryptionWrapper symmetric{ keyRetriever, encryptor, encryptor, 16 };
// Same thing as above except we specified the nonce size.
// Encryptor is given twice, as the encryption algo and the decryption one.
```

---
## AppendGuideline
AppendGuideline is a relatively simple guideline to use. This guideline simply adds data at the end of the file. It reads the last characters starting at eof.

The main limitations are:
- This guideline doesn't support the `EncryptionWrapper` class.
- You cannot edit data.

You don't risk a lot regarding corruption. The worst that can happen is partial write, usually caused, although extremely rare, by power outage. Any data that you already wrote is safe.

The `AppendGuideline` is fast, easily comparable to `std::fstream` with `std::ios::app`. It is the fastest with `IndexedGuideline` for a given number of characters to read. It is a bit slower to write because file space has to be allocated, but it remains fast.

To open a file, use `.open()` or `.openAndAppend()` (the last one also writes immediately). You can also open a file via the constructor but this is deprecated because the error handling is lacking (the overload is marked `NOEXCEPTDEBUG`).

Don't forget to explicitly close the file via `.close()` or `.readAndClose()`. If you don't, the destructor will assert in debug to warn you. This aims to force you to add proper error handling during closing because the system can fail to flush. Note that if `readAndClose()` fails, you should call `close()` again.

If an error happened before, you should still close the guideline. When you call the function `.close()`, you can pass a `std::error_code` as an argument. This argument is irrelevant to the function itself (the stream will be closed anyway) but is used to simplify your error management. The return value of `.close()` is as follows:
- Most of the time, it is the `std::error_code` of the function itself that is returned, which tells you whether closing is successful or not.
- However, if and only if you passed a non-0 `std::error_code` as the argument, that error code is returned by the function after attempting to close the stream, whether it succeeded or not.

That way, instead of having to handle two potential error codes (the one from the `close()` function and the one you passed as an argument), you handle only one in one place. So, the error code passed is prioritary over the one returned by the function. The one passed is the one you want to handle because it happened first and could have caused the error during closing. The one returned by the function would be a consequence of the first one.

The following is a full example with error handling:
```c++
import std;
import FileGuard;

std::error_code handleIO(const std::filesystem::path& path)
{
    std::error_code ec{}; // Creates an empty error code to store the result of the operations.
    FileGuard::AppendGuideline guideline{}; // Creates the instance.

    ec = FileGuard::createFile(path); // Tries to create a file if it does not exist. If it exists, nothing happens.
    if (ec) return ec; // Error management: here we return the error code to the caller. The file is not opened, so there is no need to close it.

    ec = guideline.openAndAppend(path, "Hello, World!"); // Opening + writing someting.
    if (ec) return guideline.close(ec); // Always close even if the previous operation failed. 
    
    // Remember that the 'close()' function will return the error code passed as argument if it is not successful. The
    // actual error code by the `close()` function is returned only when that argument is set to 0. Otherwise, it is
    // discarded. Here, the benefit is that the caller will have to handle only one error code. The `close()` error
    // code is not important because it may have been caused by the previous failed operation, which is the one that
    // really matters.

    // Reading + closing the file. 
    std::expected<std::string, std::error_code> result{ guideline.readAndClose(6) }; // Tries to read the last 6 characters of the file and then tries to close it.
    if (!result.has_value()) return guideline.close(result.error()); // Same as above: always close. `readAndClose()` is guaranteed to close the file ONLY if it succeeded (here, it didn't).

    std::string& str{ result.value() }; // Because the previous operation was successful, we can get the value of the expected object.
    // Do something with your data.
    std::println("Read from file: {}", str);

    // `readAndClose` already closed the file, so there is no need to call `close` again. 
    // If you called `readAndRestore()` instead, you would have to call `close` here to close the file. For example, 
    // you may write: `return guideline.close();` (there is no need to pass an error code as argument since everything
    // went fine).

    return {}; // Everything went fine, we return an empty error code. 
}


int main()
{
    const std::error_code ec{ handleIO("dummyAppend.txt") };

    if (ec) std::println("An error happened: {}", ec.message());
    else    std::println("Everything went fine!, \"Hello, World!\" was appended.");

    return ec.value();
}
```

---
## IndexedGuideline

`IndexedGuideline` is the hardest to use. But, it is also arguably the most powerful. This guideline allows you to read, edit or extend a file where you want. 

Indexes are integer values that tell the class where to write/read in the file. Index 0 is the first section, index 1 is the next section, and so on... A "section" is the number of bytes to write/read per index, which is fixed for every instance. It is equal to the template argument you give when instantiating an object. 
```c++
// There is no error handling here because it is for demonstration. The full example below includes it. 

FileGuard::IndexedGuideline</*number of bytes per instance: */16> instance{}; // Each section of this instance is EXACTLY 16 bytes.
// stuff... (like creating/opening a file)
instance.write("Hello", 0); // tries to write "Hello" at section of index 0. However, this fails because "Hello" is of size 5, not 16. Returns `std::errc::invalid_argument` via an error code.
instance.write("This is 16 bytes", 0); // Ok, at index 0.
instance.write("This is 16 bytesThis is 16 byte", 0); // Ok, at index 0 and 1: writes two sections of 16 bytes each.
```

You can write multiple sections at once. Also, the section can be located anywhere, even far from the end of the file. The file will be extended. Whether that extension takes space on your disk depends on your OS. Reading past eof will return an error.

The second-biggest risk with this guideline is corruption. While it's safe to assume that the data you already wrote are safe, no guarantees can be made regarding data that you are writing. May be totally, partially or not updated after a crash.

The biggest risk with this guideline is you. You have to handle indexes and off-by-one errors, which are notoriously complicated. While you won't have any undefined behavior, you will have erroneous behavior. Because this interacts with files, you may accidentally (and permanently) remove precious data. Be cautious with indexes.

`IndexedGuideline` is the fastest guideline with the `AppendGuideline` guideline. It is even arguably faster because you can move around the file freely. In writing, `IndexedGuideline` doesn't necessarily need to allocate new file space. In reading, you don't need to start reading from the end if the data you are looking for are located at the beginning.

Note that to fully benefit from the `IndexedGuideline` guideline performance, each call to write/read should be as close to the I/O buffer (usually around 4KB) as possible. You can either increase the template arguments (tips: don't use 16 like in the example) or read/write multiple sections at once. Avoid many call to read/write because this destroys performance.

To open a file, use `.open()`. You can also open a file via the constructors but this is deprecated because the error handling is lacking (the overloads are marked `NOEXCEPTDEBUG`).

Don't forget to explicitly close the file via `.close()`. If you don't, the destructor will assert in debug to warn you. This aims to force you to add proper error handling during closing because the system can fail to flush.

If an error happened before, you should still close the guideline. When you call the function `.close()`, you can pass a `std::error_code` as an argument. This argument is irrelevant to the function itself (the stream will be closed anyway) but is used to simplify your error management. The return value of `.close()` is as follows:
- Most of the time, it is the `std::error_code` of the function itself that is returned, which tells you whether closing is successful or not.
- However, if and only if you passed a non-0 `std::error_code` as the argument, that error code is returned by the function after attempting to close the stream, whether it succeeded or not.

That way, instead of having to handle two potential error codes (the one from the `close()` function and the one you passed as an argument), you handle only one in one place. So, the error code passed is prioritary over the one returned by the function. The one passed is the one you want to handle because it happened first and could have caused the error during closing. The one returned by the function would be a consequence of the first one.

The following is a full example with error handling:
```c++
import std;
import FileGuard;

std::error_code handleIO(const std::filesystem::path& path)
{
    std::error_code ec{}; // Creates an empty error code to store the result of the operations.
    FileGuard::IndexedGuideline<12> guideline{}; // Every string written to the file MUST be a multiple of 12 characters long.

    ec = FileGuard::createFile(path); // Tries to create a file if it does not exist. If it exists, nothing happens.
    if (ec) return ec; // Error management: here we return the error code to the caller. The file is not opened, so there is no need to close it.

    ec = guideline.open(path); // Opening the file.
    if (ec) return guideline.close(ec); // Always close even if the previous operation failed.

    ec = guideline.write("Hello, World", 2); // Starts writing at position of index 2. (This means that there is a file hole between index 0 and index 2)
    if (ec) return guideline.close(ec); // Same as above: always close.

    ec = guideline.write("Hello, World", 1); // Writes at position of index 1. This fills the file hole a bit.
    if (ec) return guideline.close(ec);
    
    // Remember that the 'close()' function will return the error code passed as argument if it is not successful. The
    // actual error code by the `close()` function is returned only when that argument is set to 0. Otherwise, it is
    // discarded. Here, the benefit is that the caller will have to handle only one error code. The `close()` error
    // code is not important because it may have been caused by the previous failed operation, which is the one that
    // really matters.

    // Reading + closing the file. 
    std::expected<std::string, std::error_code> result{ guideline.read(1, 2) }; // Reads 2 block (of 12 characters each) starting at position 1
    if (!result.has_value()) return guideline.close(result.error());

    std::string& str{ result.value() }; // Because the previous operation was successful, we can get the value of the expected object.
    // Do something with your data.
    std::println("Read from file: {}", str);

    return guideline.close(); // Everything went fine, we return the status of the `close()` operation. 
}

int main()
{
    const std::error_code ec{ handleIO("dummyIndexed.txt") };

    if (ec) std::println("An error happened: {}", ec.message());
    else    std::println("Everything went fine!, two \"Hello, World!\" strings were added.");

    return ec.value();
}
```

### Using encryption with this guideline

You can ignore this section if you don't plan on using the encryption system.

This guideline type allows you to use the `EncryptionWrapper` class. To create an instance that supports encryption, you need to give the size of the algorithm's nonce as a template argument at construction time. You must also pass that algorithm to the constructor (via reference).  DON'T DEALLOCATE THIS WRAPPER INSTANCE. Otherwise, it's UB when encrypting/decrypting.

```c++
// There is no error handling here because it is for demonstration. The full example below includes it. 

FileGuard::IndexedGuideline<16, /*number of bytes per none: */12> instance{ FileGuard::g_toyWrapper }; // g_toyWrapper is an instance of the `EncryptionWrapper` class that needs a 12 bytes nonce.
// stuff... (like creating/opening a file)
FileGuard::g_toyWrapper.generateKey("45689"); // Let's say this string is a server port that sends you the actual key. Also remember that `instance` has a pointer to this wrapper: it will use your key.
instance.write("This is 16 bytes", 0); // Ok. A nonce is generated.
instance.write("This is a 28 bytes string ex", 0); // Not Ok. Though 16 + 12 is 28, the nonce size does not count. The string must still be 16 bytes. 
// You can still write multiple sections at once. Each section will have its own nonce. It slows down operation a bit.
FileGuard::g_toyWrapper.clearKey(); // Super-duper important if clearing early is essential (RAII would still do it at destruction time if you forget).
```
When using encryption, each section size is increased by the nonce size. When you store data, the nonce is stored right next to your encrypted data. Hence, this guideline takes more file space the more you encrypt.

Be aware of a common gotcha: contrary to the toy example, most encryption algorithms output data with a specific size.
```c++
// There is no error handling here because it is for demonstration. The full example below includes it. 

FileGuard::IndexedGuideline<20, 12> instance{ myWrapper }; // myWrapper encrypts data and outputs blocks of 20.
// stuff... (like creating/opening a file)
instance.write("This isn't 20 bytes", 0); // This will work. myWrapper encrypts data and gives an output of 20 bytes.
```

This guideline will use the `.getCryptoNonce()` member from `EncryptionWrapper`. You cannot change that (this is a known limitation). If you absolutely need a specific nonce getter, use the guideline without encryption and encrypt your data beforehand.

The following is a full example with error handling and encryption enabled:
```c++
import std;
import FileGuard;

std::error_code handleIO(const std::filesystem::path& path)
{
    std::error_code ec{}; 
    constexpr std::uint16_t nonceSize{ FileGuard::EncryptionWrapper::s_defaultNonceSize }; // The guideline needs the nonce size.
    FileGuard::IndexedGuideline<12, nonceSize> guideline{ FileGuard::g_toyWrapper }; // Gives the wrapper by reference.

    ec = FileGuard::createFile(path);
    if (ec) return ec;

    ec = guideline.open(path);
    if (ec) return guideline.close(ec);

    FileGuard::g_toyWrapper.generateKey("azerty"); // Always generate a key before writing OR reading.

    ec = guideline.write("Hello, World", 2); 
    if (ec) return guideline.closeAndClearKey(ec); // `closeAndClearKey()` is the exact same as `close()` but it also clears the key from `g_toyWrapper`.

    ec = guideline.write("Hello, World", 1); 
    if (ec) return guideline.closeAndClearKey(ec); // Again, same as `close()` but it also clears the key from `g_toyWrapper`. 
    // You could use `close()` and then clear the key manually from `g_toyWrapper` but this is more convenient since we have one return statement.
   
    FileGuard::g_toyWrapper.clearKey(); // Clear the key from `g_toyWrapper` since we don't want to keep it longer than necessary. This is a good practice to avoid keeping sensitive data in memory.
    // We do something...
    // We need to read the file, so we generate a key again.
    FileGuard::g_toyWrapper.generateKey("azerty"); // Same seed as before, otherwise we won't be able to read the file.

    // Reading + closing the file. 
    std::expected<std::string, std::error_code> result{ guideline.read(1, 2) }; // Reads 2 block (of 12 characters each) starting at position 1
    if (!result.has_value()) return guideline.close(result.error());

    std::string& str{ result.value() }; // Because the previous operation was successful, we can get the value of the expected object.
    // Do something with your data.
    std::println("Read from file: {}", str);

    return guideline.closeAndClearKey(); // Everything went fine, we return the status of the `close()` operation. 
}

int main()
{
    const std::error_code ec{ handleIO("dummyIndexed.txt") };

    if (ec) std::println("An error happened: {}", ec.message());
    else    std::println("Everything went fine!, two \"Hello, World!\" strings were added.");

    return ec.value();
}
```

---
## AllOrNothingGuideline
`AllOrNothingGuideline` is the most robust. It allows to perform I/O operations with the guarantee that if it fails, the data will not be corrupted, though at the cost of performance.

Contrary to what you might think, this guideline is not atomic. Being atomic would require being isolated from other threads, which it isn't. Other threads may still step in the way if you are using them without being careful enough.

`AllOrNothing` is an exported alias of `AllOrNothingGuideline` to shorten the name.

This guideline writes and reads a whole file at once. Because of this, you don't need to perform multiple operations at different places in the file. Hence, all functions are static and the guideline is not instantiatable in order to eliminate the need of an object. For each call, the guideline will open and close the stream automatically making it easier to use.

One advantage with `AllOrNothingGuideline` is that you will always know if your last writing operation succeeded given proper error handling. That is true even after power outage or crash of any sorts. 

`AllOrNothing` stores the data in a separate empty file, before replacing the original file by this new version once it's done writing. If a crash happens, the `::read()` static member will look for the state of both files and choose the most complete (and up-to-date) one. `AllOrNothing` files take an additional 64 bytes of storage to store the "confirmation string". This string tells, if it is there, that a given file is complete. Otherwise, the file is incomplete and thus corrupted.

If a file actually ends up corrupted, it is usually caused by the user who interacted and wrote things into the file, either by hand or by using another guideline.

The biggest disadvantage with `AllOrNothing` is slowness in writing. Check out the guideline section of "benchmarks/benchmarks.md" to understand how. To sum up, reading is almost similar to the `AppendGuideline` or `IndexedGuideline` while writing is significantly slower.

The proper way to handle the "::read()" static member can be a bit tricky. This function returns a `std::pair` of `std::string` and `ReadResult` that you'll have to handle. The reason why it is not a `std::expected` is because, with `AllOrNothing`, not all errors are fatal errors.

```c++
// This is what `ReadResult` contains:
struct ReadResult
{
    std::error_code ec{}; ///< The actual error code.
    bool isUpToDate{};    ///< `true` if the data extracted aren't outdated.
};

static std::pair<std::string, ReadResult> read(const std::filesystem::path& path); // Slightly simplified definition.
```

Both values (`std::string` and `ReadResult`) tell you something different but useful. A fatal error refers to the string being returned as empty.

Checking the value of `std::string` will tell you if the data could be extracted. If it is empty, you know a fatal error happened. If it isn't, usable data were returned. However, this doesn't tell you that no error happened. Non-fatal errors could have occurred leaving you with valid but outdated data.

`ReadResult` contains two values. The first is the error code, which could come from STL's own generic error category, or FileGuard's category. If this error code is null, no error happened. When the FileGuard category is set, 200 also means it is a success, only that a crash happened while being fully recovered from. In any other cases an error happened, a fatal or non-fatal one (check out the emptiness of the `std::string`).  The second value is a bool that tells you if the string that is contained is up to date (true) or a backup du to a crash (false).

If the error category is the STL, a fatal error happened. There is no non-fatal errors in that case. 

If any errors happen (including non-fatal ones), `isUpToDate` will be set to `false`. Because checking if ec is 0 is not enough (there are two valid error codes), you would want to check `isUpToDate` to know if your data is both valid and up to date.
```c++
auto [data, error] = FileGuard::AllOrNOthing::read(...);
if (error.isUpToDate) // Is data valid
{
    if (error.ec) // error happened (not 0). This is necessary the 200 code from FileGuard because isUpToDate is true.
    {
        std::println("there was a bug but everything is fine");
        std::cout << error.ec.message() << '\n';
    }
    
    // Do what you want with the data. data is NOT empty.
}
```

In the case where isUpToDate is not true, you'd want to check if there is data (not empty). Then, handle the error using the error code. You can handle the error code the same way as usual. It is recommended to do it differently depending on if the string is empty.

Remember that only the FileGuard category has non-fatal errors. So, if there is an error and the string is not empty, this is from FileGuard. This occurs when a crash happened during the previous write operation, and it couldn't be finished.

Note: Don't forget encryption errors (from FileGuard's category) if you are using encryption. They all are fatal.

Fatal error codes don't necessarily mean the data is lost. It could be a permission issue, a file not found, a file in use... (sets the generic category from the STL). There is one fatal error code from FileGuard that tells you the data is lost (see error code from FileGuard). However, this is not supposed to happen by the design of AllOrNothing. It likely means the user interacted with your files (or you used that file with other guidelines). 

Check out FileGuard's error code for all possible error codes.

The following is a full example with full error handling:
```c++
import std;
import FileGuard;

std::error_code handleIO(const std::filesystem::path& path)
{
    std::error_code ec{};

    ec = FileGuard::createFile(path);
    if (ec) return ec;

    ec = FileGuard::AllOrNothing::write(path, "Hello, World!");
    if (ec) return ec;

    auto [data, error] = FileGuard::AllOrNothing::read(path);
    // Here is the tricky part.
    if (data.empty())
    {
        std::println("fatal error: no data could be loaded"); // Fatal error. No data could be loaded. 
    }
    else if (!error.isUpToDate) // Non fatal error. Data is valid but not up to date.
    {
        std::println("A non fatal error happened: there is data loss but an older save was restored");
        
        // Do something with your data.
        std::cout << data << '\n';
    }
    else // The data is valid
    {
        // The following if block is optional. You can just ignore the error and use the data.
        if (error.ec) // a quirk happened. This is necessary the 200 code from FileGuard because isUpToDate is true.
            std::println("there was a crash but everything is fine. Just be aware.");

        // Do something with your data.
        std::cout << data << '\n';
    }

    return error.ec; // Return the error code. If everything went fine, this will be 0.
}

int main()
{
    const std::error_code ec{ handleIO("dummyAllOrNothing.txt") };

	if (ec) std::println("Something happened: {}", ec.message()); // May be an actual (even fatal) error or just the 200 code from FileGuard. Check the error code to know what happened.
    else    std::println("Everything went fine!, two \"Hello, World!\" strings were added.");

    return ec.value();
}
```

### Using encryption with this guideline

You can ignore this section if you don't plan on using the encryption system.

This guideline also supports encryption. To enable encryption, you simply need to pass a wrapper as an argument:
```c++
// There is no error handling here because it is for demonstration. The full example above includes it. 

FileGuard::g_toyWrapper.generateKey("azerty");
FileGuard::AllOrNothing::write("myFile.txt", "some data to save", &FileGuard::g_toyWrapper); // Pass it as a pointer.
FileGuard::AllOrNothing::read("myFile.txt", &FileGuard::g_toyWrapper); // Pass it as a pointer.
FileGuard::g_toyWrapper.clearKey();
```
The nonce is stored at the end of the file, increasing the space that the file takes by the nonce size.

The same thing as `IndexedGuideline` applies here regarding nonces. This guideline will use the `.getCryptoNonce()` member from `EncryptionWrapper`. You cannot change that (this is a known limitation). If you absolutely need a specific nonce getter, use the guideline without encryption and encrypt your data beforehand.