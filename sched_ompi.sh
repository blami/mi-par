#!/bin/sh
#sched_serial.sh: schedule all instances for OpenMPI run

QSUB=qsub
INST="inst1-5-5-5.srp"
#	inst1-6-21-6.srp
#	inst1-7-7-7.srp"

CPU="2 4" # 8 16 24"

for N in $CPU
do
	for I in $INST
	do
		#qsub -pe ompi $N -q default.q -l pdq ./qsub_ompi_eth.sh "$I"
		qsub -pe ompi $N -q default.q -l pdq ./qsub_ompi_inf.sh "$I"
	done
done
