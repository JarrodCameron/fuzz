#!/bin/sh

# Author: Jarrod Cameron (z5210220)
# Date:   09/07/20 19:23

# Exit on non-zero return status
set -e

rm -f testdata.bin bad.txt
make

clear

if [ "$#" -lt '1' ]; then
       echo 'Usage: ./run.sh <prog>'
       exit 1
fi

f="$1"

if [ ! -e 'examples/'"$f" ]; then
       echo 'examples/'"$f"' does not exist!'
       exit 2
fi

timeout --foreground -v 180 ./fuzzer examples/"$f".txt examples/"$f"

