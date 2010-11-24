#!/bin/sh
#sched_serial.sh: schedule all instances for serial run

QSUB=qsub
INST="inst/inst1-5-5-5.srp"
#	inst/inst1-6-21-6.srp
#	inst/inst1-7-7-7.srp"

for I in $INST
do
	qsub ./qsub_serial.sh "$I"
done
