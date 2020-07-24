# fuzz
Fuzzer for comp6447

```
# Creat tar archive
tar -cf fuzzer.tar fuzzer

# Open tar archive
tar -xf fuzzer.tar
```

# Benchmark

| What                                                 | # iters |
|------------------------------------------------------|---------|
| Passing `csv1.txt` to `csv1` in python3              | 214920  |
| Passing `csv1.txt` to `csv1` in C                    | 375907  |
| Fuzzing `csv1` without fork server                   | 218217  |
| Fuzzing `csv1` with fork server                      | 426584  |
| Fuzzing `json1` without fork server                  | 11415   |
| Fuzzing `json1` with fork server                     | 16537   |

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

