set encoding iso_8859_2
set term postscript enhanced color "Times" 16
set output "time.ps"

set title "Èasy bìhu pro jednotlivé instance\n(infiband a ethernet)"
set xlabel "Poèet procesorù [-]"
set ylabel "Èas [s]"
set logscale y
set xtics (1, 2, 4, 8, 16, 24, 32)
# nezobrazovat klic - je jasny z predchozich obrazku
unset key

set style line 1 lt 3 lc 1 pt 2
set style line 2 lt 3 lc 2 pt 2
set style line 3 lt 3 lc 3 pt 2
set style line 4 lt 3 lc 4 pt 2
set style line 5 lt 1 lc 1 pt 2
set style line 6 lt 1 lc 2 pt 2
set style line 7 lt 1 lc 3 pt 2
set style line 8 lt 1 lc 4 pt 2

plot \
	"./time_eth.dat" using 1:2 ls 1 title 'inst1-5-5-5' with linespoints, \
	"./time_eth.dat" using 1:3 ls 2 title 'inst1-6-21-6' with linespoints, \
	"./time_eth.dat" using 1:4 ls 3 title 'inst1-7-7-7' with linespoints, \
	"./time_eth.dat" using 1:5 ls 4 title 'inst2-7-8-7' with linespoints, \
	"./time_inf.dat" using 1:2 ls 5 title 'inst1-5-5-5' with linespoints, \
	"./time_inf.dat" using 1:3 ls 6 title 'inst1-6-21-6' with linespoints, \
	"./time_inf.dat" using 1:4 ls 7 title 'inst1-7-7-7' with linespoints, \
	"./time_inf.dat" using 1:5 ls 8 title 'inst2-7-8-7' with linespoints
