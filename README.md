# fuzz
Fuzzer for comp6447

# Benchmark

| What                                                 | # iters |
|------------------------------------------------------|---------|
| Passing `csv1.txt` to `csv1` in python3              | 214920  |
| Passing `csv1.txt` to `csv1` in C                    | 375907  |

# Shared libs

- [csv\_parser](https://github.com/semitrivial/csv_parser)
- [json\_parser](https://github.com/udp/json-parser)
  - [json\_builder](https://github.com/udp/json-builder)

# Notes from chat

- [ ] Divide mut = 1/ orig
- [ ] multiply mut = orig * orig
- [ ] negative mut = - orig
- [ ] absolute mut = |orig|
- [ ] swap integers with fractions
- [ ] swap numbers with characterws
- [ ] change full fields to empty fields
- [ ] change fields to large input
- [ ] change fields to include control characters
- [ ] change strings to include non printable characters
- [ ] add extra fields at end
- [ ] insert extra fields at beginning
- [ ] insert extra fields in between other fields
- [ ] format strings = `%?$s`
- [ ] rolling bit flip

# Paper from shan

https://arxiv.org/pdf/1812.00140.pdf

# todo

jarrod -
- finding library of parsing file
- checking return value of victim
- remind group of group meeting (3:45pm Monday 13/07/2020)

michelle
- identify file format

brandon
- generic functions for modifying the testdatab.bin file

shan
- Given a parsed file, figure out what/where to fuzz


