#!/bin/bash

# this assumes you already have Valgrind installed (https://valgrind.org/) with the
# Callgrind plugin.
# The KCacheGrind tool is also useful for analyzing Valgrind outputs (https://kcachegrind.github.io/html/Home.html)


valgrind --tool=callgrind "$@"