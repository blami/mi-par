all: plot report

report:

plot: speed.ps speed_inf.ps speed_eth.ps time.ps time_inf.ps time_eth.ps

speed.ps: time_inf.dat time_eth.dat
	gnuplot ./plot-speed.gnuplot

speed_inf.ps: time_inf.dat
	gnuplot ./plot-speed_inf.gnuplot

speed_eth.ps: time_eth.dat
	gnuplot ./plot-speed_eth.gnuplot

time.ps: time_inf.dat time_eth.dat
	gnuplot ./plot-time.gnuplot

time_inf.ps: time_inf.dat
	gnuplot ./plot-time_inf.gnuplot

time_eth.ps: time_eth.dat
	gnuplot ./plot-time_eth.gnuplot

time_inf.dat: time.dat
	./prepare_plot.pl -n inf > time_inf.dat

time_eth.dat: time.dat
	./prepare_plot.pl -n eth > time_eth.dat

clean:
	rm -f *.ps
	rm -f time_eth.dat time_inf.dat
