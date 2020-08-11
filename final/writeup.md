# COMP6447 - Final Fuzzer Submission

---
    Jarrod Cameron (z5210220)
    Shantanu Kulkarni (z5205339)
    Michelle Qiu (z5163295)
    Brandan Nyholm (z5206679)
---

# How The Fuzzer Works

This fuzzer works by initially detecting the file type out of plaintext, CSV,
JSON or XML and then parsing the file's data into a data structure for the
fuzzer to manipulate.

After determining the file data type the fuzzer employs tailored strategies
based on the binary's datatype. Such as maintaining `{} [] ,` characters for
JSON when fuzzing for format strings but mutating them when testing the
binary's parsing ability.

The fuzzer will always run a few shorter functions at the beginning of the
process for vulnerabilities that are easy to check for and can be ruled out.

After this the fuzzer will begin mutating the input in line with some internal
strategies with random data or at random locations in the input. These
strategies will continue to run varying on each execution until the program is
quit.

# What kind of bugs the fuzzer can find

The fuzzer is split into four different components, one for each file type and
is optimised to look for particular bugs depending on the four file types.

## CSV

- Format string vulnerabilities inside fields
- Buffer overflow vulnerabilities inside fields
- Bugs with missing or extra control characters
- Bugs parsing non-printable or ascii characters
- Bugs parsing large files
- Integer overflows
- Integer underflows

## XML

- Format string vulnerabilities inside of tags
- Format string vulnerabilities between tags
- Format string vulnerabilities inside of properties inside tags
- Buffer overflow vulnerabilities inside of tags
- Buffer overflow vulnerabilities between tags
- Buffer overflow vulnerabilities inside of properties inside tags
- Bugs with missing or extra control characters
- Bugs parsing non-printable or ascii characters
- Bugs parsing large files
- Integer overflows
- Integer underflows
- Nesting control structures (e.g. nesting tags)

## Json

- Format string vulnerabilities inside braces
- Buffer overflow vulnerabilities inside braces
- Bugs with missing or extra control characters
- Bugs parsing non-printable or ascii characters
- Bugs parsing large files
- Integer overflows
- Integer underflows
- Nesting control structures (e.g. nesting braces and brackets)

## Plain Text

- Format string vulnerabilites
- Buffer overflow vulnerabilities 
- Integer overflows
- Integer underflows
- Bugs parsing non-printable characters 
- Bugs parsing unicode symbols
- Bugs parsing ascii characters/punctuation

# Fuzzing Strategies

- __Plaintext__: 
  In `fuzz_plaintext.c` , the `fuzz_handle_plaintext` first attempts a large
  buffer overflow a random number of times, then reads a the input file 
  into memory to be manipulated.
  In the first loop, as denoted in comments, fuzzing methods only requiring
  one execution such as trying typical problematic injection strings or 
  numbers. The lists `bad_strings` and `bad_nums` are called from `utils.h`
  in the fuzzing methods `replace_numbers` and `replace_strings`.
  The second loop infinitely loops over every line in the input file, fuzzing
  each line with functions from `mutation_functions.c` such as `bit_flip_in_range`
  `bit_shift_in_range`. 

- __JSON__: In `fuzz_json.c`, the fuzzer performs a few different types of strategies. 
It first runs the functions that are deterministic, such as buffer overflow and format 
strings. These will perform a function either on each entry, or on each value in the entry. 
`fuzz_empty` will remove names and values from the entry. `fuzz_extra_entries`, 
`fuzz_extra_objects` and `fuzz_append_objects` all perform overflow of some kind, with 
`fuzz_append_objects` being invalid JSON.

| Function               | Output 																		|
|------------------------|------------------------------------------------------------------------------|
| `fuzz_extra_entries`   | `{ "extra_name" : " "extra_value",  "extra_name" : " "extra_value" }` 		|
| `fuzz_extra_objects`   | `[{ "extra_name" : " "extra_value" }, { "extra_name" : " "extra_value" }]`   |
| `fuzz_extra_objects`	 | `{ "extra_name" : " "extra_value" }`<br> `{ "extra_name" : " "extra_value" }`|

- __CSV__: In `fuzz_csv.c`, the fuzzer initially runs 4 payloads that attempt the same technquies without any randomisation. 
These include some buffer overflow and format string attacks as well as swapping some values with known problematic input such as swapping `1` with `-1`, `-999999`, `0` etc. 
These functions run the same checks for each invokation so there is no added value in running them a second time on a binary. After this set of functions are executed the fuzzer progresses into an infinite loop which mutates the input in random ways in random locations. The loop will exit when a crash occurs. 
Some techniques include bit shifts (`bit_shift_in_range`), bit flips (`bit_flip_in_range`), increasing the number of cells (`fuzz_populate_width`), increasing the number of rows (`fuzz_populate_length`) and creating numerous empty cells (`fuzz_empty_cells`). 
These strategies continue to execute with different outputs due to varying execution based on a pseudo random number generator `rand()`.


# Possible Improvements

- __Multi-threading__: This would provide a large performance boost. Currently
  the fuzzer only uses a single cpu while most other cpu's are idle. Only a few
  data structures would need to be shared which would result in little
  synchronisation overhead.
- __Chaining fuzzing strategies__: Most fuzzing strategies within the fuzzer
  test for a single bug at a time. A possible improvement would be testing a
  few possible bug classes together, for example:
  - Buffer overflow in one field and integer overflow in another
  - Buffer overflow in two separate fields
- __Modifying control data__: This fuzzer excels at mutating data inside
  certain parts of the payload, since the libraries used to parse and modify
  the data structures provide a simple to use API. However, only a few
  strategies focus on fuzzing control data (e.g. `<` and `>` for XML, `{` and
  `}` for Json). One possible improvement is to mutate pairs of control
  characters in the input, for example: `<` and `>` characters for a XML tag.
- __Optimising disk IO__: There is the possibility of strategically calling
  functions so that the amount of time writing to disk is reduced. It could
  also  possible to more closely keep track of the changes made to the
  `/tmp/testdata.bin-XXXXXX` so that `lseek` could be employed to simply revert
  those specific bytes instead of the whole file. This would reduce disk IO.

# Bonus Marks

## Fork Server

For the bonus component we implement a fork server which is used to create
processes instead of using the `execve` system call. This was inspired by
[this](https://lcamtuf.blogspot.com/2014/10/fuzzing-binaries-without-execve.html)
article written by lcamtuf (the author of the AFL fuzzer).

Before the fuzzer is started, two shared objects are compiled (when invoking
the `install.sh` script): "shared32.so" and "shared64.so". These shared objects
are used for 32bit and 64bit executables respectively. Using the `LD_PRELOAD`
environment variable we can load in our shared object into the target. To
figure out if the target is a 32bits or 64bits, the elf header is checked.
The process of checking the elf file can be seen below:

```{c}
#include <elf.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

unsigned char get_class(void)
{
    unsigned char e_ident[EI_NIDENT];
    int fd = open('./target', O_RDONLY);
    read(fd, e_ident, sizeof(e_ident));
    close(fd);
    return e_ident[EI_CLASS];
}
```

By using `__attribute__ ((constructor))` we can set a constructor inside our
shared library to to run before the target is run. The constructor is used to
receive commands from the fuzzer and send data to the fuzzer. There are three
commands:

- `Q` is used to exit out of the process for when we are finished.
- `T` is used to test the connection between the fuzzer and the target process.
  This is used so that the fuzzer can check if we have been successful in
  loading in our shared library (since we can't do this when we have static
  binaries).
- `R` is used to run the target process by calling the `fork` system call. The
  process of running the fork server can be seen in the pseudo code below:

```{c}
void
run(void)
{
    int wstatus;

    pid_t pid = fork();

    case (pid) {
    switch 0: /* child */
        return; /* continue execution */

    default:
        /* Wait for child process */
        waitpid(pid, &wstatus, 0);

        /* Send results back to the fuzzer */
        send(DATA_FD, &wstatus, sizeof(wstatus));

        break;
    }
}
```

The shared object will wait for commands from the parent process (i.e. the
fuzzer). Commands are sent and received on file descriptors `198` and `199`.

Using the `LD_BIND_NOW` environment variable we can force the dynamic linker to
resolve all symbols that referenced by the target on startup, this saves time
during the execution of the process since only `fork` after symbols have been
resolved.
