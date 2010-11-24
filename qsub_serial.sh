#!/bin/sh

#$ -S /bin/sh
#$ -cwd
#$ -e ./stderr/
#$ -o ./stdout/
#$ -pe ompi 1
#$ -q serial.q

./srpnompi ./inst/${1} > ./logs/$(date +%y%m%d%H%M%S)-serial_${1}.log
