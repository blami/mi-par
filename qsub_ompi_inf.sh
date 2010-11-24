#!/bin/sh

#$ -S /bin/sh
#$ -cwd
#$ -e ./stderr/
#$ -o ./stdout/
#$ -V

mpirun -np ${NSLOTS} ./srpmpi ./inst/${1} > ./logs/$(date +%y%m%d%H%M%S)-ompi${NSLOTS}-inf_${1}.log
