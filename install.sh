#!/bin/sh

# This was tested on Ubuntu 20.04 LTS virtual machine

sudo apt install -y gcc gcc-multilib libc6-dev:i386 libxml2 libxml2-dev make

make clean all
