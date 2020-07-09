#!/usr/bin/env python3
# Author: Jarrod Cameron (z5210220)
# Date:   09/07/20 18:33

import signal
import subprocess
import sys
import mmap
import os

class Fuzzer():

    def handle(self, signum, frame):
        print(self.deploys)
        sys.exit(signum)

    def __init__(self, fname, b):

        # For graceful exit and some stats
        signal.signal(signal.SIGINT, self.handle) # control-c
        signal.signal(signal.SIGTERM, self.handle) # timeout -v fuzzer.py

        with open(fname, 'r+b') as f:
            self.mem = mmap.mmap(
                f.fileno(),
                0, # map whole file
                prot=mmap.PROT_WRITE|mmap.PROT_READ
            )

        # Number of times we have run
        self.deploys = 0

        self.bin = b

    # Send self.mem to the application
    def deploy(self):

        self.deploys += 1

        with open('testdata.bin', 'wb') as f:
            f.write(self.mem.read())

        self.deploys

        subprocess.run(
            self.bin,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            input=b'testdata.bin'
        )

def main():

    if len(sys.argv) != 3:
        print(f'Usage: {sys.argv[0]} /path/to/data /path/to/bin')
        sys.exit(1)

    f = Fuzzer(sys.argv[1], sys.argv[2])
    while True:
        f.deploy()

if __name__ == '__main__':
    main()
