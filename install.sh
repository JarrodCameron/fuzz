#!/bin/sh

# Author: Jarrod Cameron (z5210220)
# Date:   30/07/20 20:38

# Exit on non-zero return status
set -e

# TODO Check this on an ubuntu box
sudo apt-get install -y libxml2
sudo apt-get install -y libxml2-dev

make shared



