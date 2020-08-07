# COMP6447 - Final Fuzzer Submission

---
    Jarrod Cameron (z5210220)
    Shantanu Kulkarni (z5205339)
    Michelle Qiu (z5163295)
    Brandan Nyholm (z5206679)
---

# How The Fuzzer Works

# What kind of bugs the fuzzer can find

# Possible Improvements

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