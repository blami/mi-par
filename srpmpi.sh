#!/bin/bash

CPU=4
mpirun --output-filename output --np $CPU srpmpi $@
