# COMP6447 - Final Fuzzer Submission

---
    Jarrod Cameron (z5210220)
    Shantanu Kulkarni (z5205339)
    Michelle Qiu (z5163295)
    Brandan Nyholm (z5206679)
---

# How The Fuzzer Works

1. Detect the input file type (it should be csv, xml, json, or plain text).
2. The fuzzer is split into four components (one for each file type). Invoke
   the part of the fuzzer responsible for fuzzing the given file type.
3. The fuzzer will mutate the given input
4. Feed the mutated input into the target binary
5. If the target did not crash because of a SIGSEGV signal, then continue from
   step 3.
6. Store the payload in the corrent working directory in a file called
   `bad.txt`
7. Prevent memory leaks by freeing all used memory and temporary files

# What kind of bugs the fuzzer can find

The fuzzer is split into four different components, one for each file type and
is optimised to look for particular bugs depending on the four file types.

## CSV

TODO

## Json

TODO

## XML

- Buffer overflow in the tag names
- Buffer overflow in the tag properties
- Buffer overflow in the data between two tags
- Format strings in tag names
- Format strings in tag properties
- Integer overflow/underflow in tag properties
- Integer overflow/underflow in data between two tags
- Memory corruption from non-printable characters
- General errors from mishandling tags that are in the wrong order

## Plain Text

TODO

# Possible Improvements

- __Multi-threading__: This would provide a large performance boost. Currently
  the fuzzer only uses a single cpu while most other cpu's are idle. Only a few
  data structures would need to be shared which would result in little
  synchronization overhead.
- __Chaining fuzzing stratagies__: Most fuzzing stratagies within the fuzzer
  test for a single bug at a time. A possible improvement would be testing a
  few possible bug classes together, for example:
  - Buffer overflow in one field and integer overflow in another
  - Buffer overflow in two seperate fields
- __Modifying control data__: This fuzzer excels at mutating data inside
  certain parts of the payload, since the libraries used to parse and modify
  the data structures provide a simple to use API. However, only a few
  stratagies focus on fuzzing control data (e.g. `<` and `>` for XML, `{` and
  `}` for Json). One possible improvement is to mutate pairs of control
  characters in the input, for example: `<` and `>` characters for a XML tag.

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
