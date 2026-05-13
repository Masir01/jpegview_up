#!/usr/bin/env bash
cd projects/vstudio/../..
find . -name "*.c" -exec sed -i 's/\r$//' {} \;
find . -name "*.h" -exec sed -i 's/\r$//' {} \;
find . -name "*.txt" -exec sed -i 's/\r$//' {} \;
find . -name "*.def" -exec sed -i 's/\r$//' {} \;
