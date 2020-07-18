# COMP6447 - Midpoint Fuzzer Submission

---
    Jarrod Cameron (z5210220)
    Shantanu Kulkarni (z5205339)
    Michelle Qiu (z5163295)
    Brandan Nyholm (z5206679)
---

# Fuzzing Strategies

- **Bit flip and bit shifts**: This strategy is less focused and works to
  simply change bit within a specific range. This has the effect of randomising
  characters. This is particularly useful for changing control characters of a
  file such as `,` for CSV `{}` for JSON and `<>` for XML among others.

- **Writing numbers**: This strategy is tailored to changing numbers within an
  input. This aims to overwrite sensitive numbers in the input. Swapping
  integers with floats, negative numbers, numbers that would overflow default
  data types and 0s.

- **Writing strings**: This strategy replace sections of the input with
  functions that aim to cause crashes such as format strings, buffer overflows,
  numbers and control characters such as newlines and null terminators.

# File Detection

In order to make our fuzzer more efficient, it is essential that it knows the
type of the file so it can fuzz type-specific characters.  Currently, it scans
through the given text file and counts the number of special characters -
`$,\{\}<>$` and new lines.  XML files have an even number of `<` and `>`; JSON
files have an even number of `{` and `}` and CSVs have the same number of
commas per line.

# External Libraries

- [csv\_parser](https://github.com/semitrivial/csv_parser): For converting a
  CSV file into a list of records and fields.
- [json\_parser](https://github.com/udp/json-parser): For converting a JSON
  file into a tree-like data structure.
  - [json\_builder](https://github.com/udp/json-builder): For converting the
    tree-like data structure into a file.

For the XML format the [libxml2](http://www.xmlsoft.org/index.html) library
will be used, however it is not available for the midpoint submission.

# Possible Improvements

## Performance

- Currently potentially a large section of the input file is rewritten to
  disk with each mutation. If the mutations could be organised more closely
  it would be possible to reduce writes to disk and thus improve fuzzer
  performance.
- Threading could also be included to further improve performance.
- Using a fork server for spawning a new process, instead of using the `execve`
  system call.

## Fuzzing Strategies

- Adding further mutations strategies.
- Combining mutation strategies. E.g. bit flip in section X and mutate numbers
  in section Y

