#!/bin/sh

# This script are steps on how to search for files with
# LG Copyright and BSD-style license

git grep -i "Copyright" * | grep LG > all-files-with-lg-license
for f in `cat all-files-with-lg-license`; do
	grep -L "BSD-style" $f;
done

# Above should print files with an issue
