export HRD_REGISTRY_IP="114.212.85.170"

./ibv_client -i mlx5_0 -p 2 -g 0 -s 4096 -k 10 -m 4 -d -v

sleep 1
