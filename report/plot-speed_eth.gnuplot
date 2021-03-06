set encoding iso_8859_2
set term postscript enhanced color "Times" 16
set output "speed_eth.ps"

set title "Zrychlen� pro jednotliv� instance\n(ethernet)"
set xlabel "Po�et procesor� [-]"
set ylabel "Zrychlen� [-]"
set xtics (1, 2, 4, 8, 16, 24, 32)
set key top left

set style line 1 lt 3 lc 1 pt 2
set style line 2 lt 3 lc 2 pt 2
set style line 3 lt 3 lc 3 pt 2
set style line 4 lt 3 lc 4 pt 2

plot \
	"./time_eth.dat" using 1:(0.110000/$2) ls 1 title 'inst1-5-5-5' with linespoints, \
	"./time_eth.dat" using 1:(194.980000/$3) ls 2 title 'inst1-6-21-6' with linespoints, \
	"./time_eth.dat" using 1:(432.440000/$4) ls 3 title 'inst1-7-7-7' with linespoints, \
	"./time_eth.dat" using 1:(691.190000/$5) ls 4 title 'inst2-7-8-7' with linespoints
