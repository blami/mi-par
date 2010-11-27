set encoding iso_8859_2
set term postscript enhanced color "Times" 16
set output "speed.ps"

set title "Zrychlení pro jednotlivé instance\n(infiband a ethernet)"
set xlabel "Poèet procesorù [-]"
set ylabel "Èas [s]"
set xtics (1, 2, 4, 8, 16, 24, 32)

set style line 1 lt 3 lc 1 pt 2
set style line 2 lt 3 lc 2 pt 2
set style line 3 lt 3 lc 3 pt 2
set style line 4 lt 3 lc 4 pt 2
set style line 5 lt 1 lc 1 pt 2
set style line 6 lt 1 lc 2 pt 2
set style line 7 lt 1 lc 3 pt 2
set style line 8 lt 1 lc 4 pt 2

plot \
	"./time_inf.dat" using 1:(0.110000/$2) ls 5 title 'infiband' with linespoints, \
	"./time_inf.dat" using 1:(194.980000/$3) ls 6 notitle with linespoints, \
	"./time_inf.dat" using 1:(432.440000/$4) ls 7 notitle with linespoints, \
	"./time_inf.dat" using 1:(691.190000/$5) ls 8 notitle with linespoints, \
	"./time_eth.dat" using 1:(0.110000/$2) ls 1 title 'ethernet' with linespoints, \
	"./time_eth.dat" using 1:(194.980000/$3) ls 2 notitle with linespoints, \
	"./time_eth.dat" using 1:(432.440000/$4) ls 3 notitle with linespoints, \
	"./time_eth.dat" using 1:(691.190000/$5) ls 4 notitle with linespoints

