#!/bin/sh

# Author: Jarrod Cameron (z5210220)
# Date:   09/07/20 19:23

# Exit on non-zero return status
set -e

make

timeout -v 180 ./fuzzer examples/dummy.txt examples/dummy



