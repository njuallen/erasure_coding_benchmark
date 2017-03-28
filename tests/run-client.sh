export HRD_REGISTRY_IP="114.212.85.170"

./ibv_ec_sw_encode_send -i mlx5_0 -p 2 -g 0 -F 42949672960 -s 163840 -k 10 -m 4 -w 8 -d -v

# ./ibv_client -i mlx5_0 -p 0 -g 0 -s 163840 -k 10 -m 4 -d -v

sleep 1
