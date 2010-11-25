#!/bin/sh

#$ -S /bin/sh
#$ -cwd
#$ -e ./stderr/
#$ -o ./stdout/
#$ -V

mpirun --mca btl tcp,self -np ${NSLOTS} ./srpmpi ./inst/${1} > ./logs/$(date +%y%m%d%H%M%S)-ompi${NSLOTS}-eth_${1}.log
