#!/bin/sh

# Author: Jarrod Cameron (z5210220)
# Date:   09/07/20 19:23

# Exit on non-zero return status
set -e

rm -f testdata.bin bad.txt
make

if [ "$#" -lt '1' ]; then
	echo 'Usage: ./run.sh <prog> [v|V]'
	exit 1
fi

f="$1"
shift

bin=''
txt=''

if [ -e 'examples/'"$f" ]; then
	bin='examples/'"$f"
	txt='examples/'"$f"'.txt'
elif [ -e 'dummy/'"$f" ]; then
	bin='dummy/'"$f"
	txt='dummy/dummy.txt'
else
	echo 'There is no '"$f"' in examples/ or dummy/'
	exit 2
fi

cmd='timeout --foreground -v 180'

if [ -n "$1" ]; then
	case "$1"
	in
		'v') cmd='valgrind --leak-check=full' ;;
		'V') cmd='valgrind --leak-check=full --show-leak-kinds=all' ;;
		*) echo '???' ; exit 1 ;;
	esac
fi

clear

$cmd ./fuzzer "$txt" "$bin"

