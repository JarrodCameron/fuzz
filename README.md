# fuzzer

`fuzzer` is a black box fuzzer designed to find memory corruption bugs in csv,
json, xml and plain text parses.

Given an example input and a target binary, the fuzzer will mutate the input
and pass it to the target via standard input, then save the mutated input if
the program is killed from a segmentation fault.

A fork server is used to spawn processes instead of the `execve` system call.
This is used to cut down on the time it takes to create new processes. More
information about the fork server and how the fuzzer works can be found in
"writeup.md".

## Usage

Firstly, install dependencies (tested on Ubuntu 20.04 LTS)...

```{sh}
./install.sh
```

Secondly, compile...

```{sh}
make
```

Thirdly, run...
```{sh}
./fuzz <path/to/input.txt> <path/to/target>
```

# External libraries

- [csv\_parser](https://github.com/semitrivial/csv_parser)
- [json\_parser](https://github.com/udp/json-parser)
- [json\_builder](https://github.com/udp/json-builder)

# Authors

```
Jarrod Cameron (z5210220)
Shantanu Kulkarni (z5205339)
Michelle Qiu (z5163295)
Brandan Nyholm (z5206679)
```
