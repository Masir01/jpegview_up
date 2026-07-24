#!/usr/bin/env bash

# apng support — check if already applied
echo "--- Patching libpng with apng support ---"
patch -d libpng -p0 --forward --binary < libpng-apng/libpng-1.6.58-apng.patch 2>/dev/null
rc=$?
if [ $rc -eq 0 ]; then
	echo apng patch applied successfully
elif [ $rc -eq 1 ]; then
	# patch returned 1 = some hunks rejected or already applied
	# check if it was "already applied" (reversed)
	if patch -d libpng -p0 --dry-run --reverse --binary < libpng-apng/libpng-1.6.58-apng.patch 2>/dev/null; then
		echo apng patch already applied, skipping
	else
		echo apng patch failed — not already applied and can't apply
		exit 1
	fi
else
	echo apng patch error
	exit 1
fi


# 64-bit support — check if already applied
echo "--- Patching libpng with 64-bit support ---"
patch -d libpng -p1 --forward --binary < libpng-x64.patch 2>/dev/null
rc=$?
if [ $rc -eq 0 ]; then
	echo x64 patch applied successfully
elif [ $rc -eq 1 ]; then
	if patch -d libpng -p1 --dry-run --reverse --binary < libpng-x64.patch 2>/dev/null; then
		echo x64 patch already applied, skipping
	else
		echo x64 patch failed — not already applied and can't apply
		exit 1
	fi
else
	echo x64 patch error
	exit 1
fi
