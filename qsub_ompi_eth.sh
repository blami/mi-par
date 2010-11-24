#!/bin/sh

#$ -S /bin/sh
#$ -cwd
#$ -e ./stderr/
#$ -o ./stdout/
#$ -V

mpirun --mca btl tcp,self -np ${NSLOTS} ./srpmpi ${1} > ./logs/ompi${2}-eth_${1}.log
