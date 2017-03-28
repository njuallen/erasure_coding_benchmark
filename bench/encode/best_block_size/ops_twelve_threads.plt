set xlabel "Block size(Bytes)"
set xtics font ", 10"
set logscale x 2
set format x "%.0f"
set format y "%.0f"
set ylabel "Encode OPS"
set title "Encode 10GB data with twelve threads"
set grid
set term jpeg
set output 'encode_ops_twelve_threads.jpg'

plot "nic_sync_ops.data" using 1:(10737418240.0/($1 * 10 * $2)) w lp pt 8 title "sync_verbs", \
     "nic_async_ops.data" using 1:(10737418240.0/($1 * 10 * $2)) w lp pt 12 title "async_verbs", \
	 "sw_ops.data" using 1:(10737418240.0/($1 * 10 * $2)) w lp pt 5 title "jerasure"
