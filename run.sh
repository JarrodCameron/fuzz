#!/bin/sh

# Author: Jarrod Cameron (z5210220)
# Date:   09/07/20 19:23

# Exit on non-zero return status
#set -e

stats () {
	cmd="$1"
	txt="$2"
	bin="$3"

	niters=50
	nbads=0

	rm -rf bad.txt*

	for i in `seq "$niters"`
	do

		rm -rf testdata.bin-*

		$cmd ./fuzzer "$txt" "$bin"

		if [ -e 'bad.txt' ]; then
			nbads=$((nbads+1))
			mv bad.txt bad.txt-"$nbads"
		fi

		# sleep to ensure uniq seed in fuzzer
		sleep 1

		echo '||' $cmd ./fuzzer "$txt" "$bin"
		echo '|| nth iteration:' "$i"
		echo '|| nth success:  ' "$nbads"
		echo '|| total iters:  ' "$niters"
	done

	echo '============================'
	echo $cmd ./fuzzer "$txt" "$bin"
	echo 'Number of iterations:' "$niters"
	echo 'Number of successes: ' "$nbads"
	echo '============================'
}

rm -f testdata.bin bad.txt
make

if [ "$#" -lt '1' ]; then
	echo 'Usage: ./run.sh <prog> [v|V|s]'
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
		'V') cmd='valgrind -s --leak-check=full --show-leak-kinds=all' ;;
		's') stats "$cmd" "$txt" "$bin" ; exit 0 ;;
		*) echo '???' ; exit 1 ;;
	esac
fi

clear

$cmd ./fuzzer "$txt" "$bin"

