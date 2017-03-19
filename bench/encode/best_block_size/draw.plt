set xlabel "Block size"
# set logscale x 2
# set format x "%.0f"
set ylabel "Time/s"
set title "Encode 1GB data with ECO"
# set grid
set term jpeg
set output 'verbs.jpg'

plot "verbs.data" using 1:3 w l  title "Verbs"
