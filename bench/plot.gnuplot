#!/usr/bin/gnuplot --persist

set datafile separator ","
set yrange [0:100]
plot  'bench/data.csv' every ::2::23 using 11:($3/$11) with lines, \
      'bench/data.csv' every ::24::45 using 11:($3/$11) with lines, \
      'bench/data.csv' every ::46::67 using 11:($3/$11) with lines,
