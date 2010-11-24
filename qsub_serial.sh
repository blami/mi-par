#!/bin/sh

#$ -S /bin/sh
#$ -cwd
#$ -e ./stderr/
#$ -o ./stdout/
#$ -pe ompi 1
#$ -q serial.q

./srpnompi inst/${1}.srp > logs/serial_${1}.log
