#!/usr/bin/env bash

while [[ ! -f .beacon ]]; do
	cd ..
done;

echo $PWD
