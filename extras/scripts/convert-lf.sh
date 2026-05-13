#!/usr/bin/env bash
cd projects/vstudio
find . -name "*.vcxproj" -exec sed -i 's/\r$//' {} \;
find . -name "*.sln" -exec sed -i 's/\r$//' {} \;
find . -name "*.vcxproj.filters" -exec sed -i 's/\r$//' {} \;
