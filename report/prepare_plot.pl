#!/usr/bin/perl -w
# Prepare MI-PAR time.dat for plotting
use strict;
use Getopt::Long;

our $NET = "inf";

# parse options
GetOptions(
	"network=s" => \$NET,
);

open(FILE, "time.dat") || die "Could not open `time.dat'";
our @lines = <FILE>;
close(FILE);

our $line;
foreach $line (@lines) {
	chomp($line);

	# skip comments
	next if(substr($line, 0, 1) eq ';');

	my $cpu;
	my $net;
	my $i1;
	my $i2;
	my $i3;
	my $i4;

	($cpu, $net, $i1, $i2, $i3, $i4) = split(/[ \t]+/, $line);

	if($net eq $NET) {
		print "$cpu $i1 $i2 $i3 $i4\n";
	}
}
