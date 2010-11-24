#!/bin/sh

#$ -S /bin/sh
#$ -cwd
#$ -e ./stderr/
#$ -o ./stdout/
#$ -V

mpirun -np ${NSLOTS} ./srpmpi ${1} > ./logs/ompi${NSLOTS}-inf_${1}.log
