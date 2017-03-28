# Define a common prefix where binaries and docs install
PREFIX = /usr
sbindir = bin

CC = gcc
CFLAGS += -g -ggdb -Wall -W -D_GNU_SOURCE -I /usr/include/jerasure -I ../libhrd/
LDFLAGS = -libverbs -lgf_complete -lJerasure -lpthread -lrdmacm -lecOffload -lmemcached -lnuma


COMMON_OBJS = common.o csapp.o adt.o utils.o ../libhrd/hrd_conn.o ../libhrd/hrd_util.o

EC_COMMON_OBJS = $(COMMON_OBJS) ec_common.o

OBJECTS_LAT = $(EC_COMMON_OBJS) \
			  ec_capability_test.o \
			  ec_encoder_sync_verbs.o ec_encoder_async_verbs.o ec_encoder_sw.o \
			  ec_sw_encode_send.o \
			  ec_decoder_verbs.o ec_decoder_sw.o \
			  main.o ppc.o server.o client.o

TARGETS = ibv_ec_capability_test \
		  ibv_ec_encoder_sync_verbs ibv_ec_encoder_async_verbs ibv_ec_encoder_sw \
		  ibv_ec_sw_encode_send \
		  ibv_ec_decoder_verbs ibv_ec_decoder_sw \
		  ibv_main ibv_ppc ibv_pcchecker ibv_server ibv_client


all: $(TARGETS)

ibv_main: main.o $(COMMON_OBJS) ec_common.o 
	$(CC) $(CFLAGS) $(LDFLAGS) main.o $(COMMON_OBJS) -o $@

ibv_ppc: ppc.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) ppc.o $(COMMON_OBJS) -o $@

ibv_server: server.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -libverbs -lpthread -lrdmacm -lmemcached -lnuma server.o $(COMMON_OBJS) -o $@

ibv_client: client.o $(COMMON_OBJS)
	$(CC) $(CFLAGS)  -libverbs -lpthread -lrdmacm -lmemcached -lnuma client.o $(COMMON_OBJS) -o $@

ibv_pcchecker:
	g++ -Wall -std=c++11 pcchecker.cpp -o ibv_pcchecker

ibv_ec_capability_test: ec_capability_test.o
	$(CC) $(CFLAGS) -libverbs ec_capability_test.o -o $@

ibv_ec_encoder_sync_verbs: ec_encoder_sync_verbs.o $(EC_COMMON_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) ec_encoder_sync_verbs.o $(EC_COMMON_OBJS) -o $@

ibv_ec_encoder_async_verbs: ec_encoder_async_verbs.o $(EC_COMMON_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) ec_encoder_async_verbs.o $(EC_COMMON_OBJS) -o $@

ibv_ec_encoder_sw: ec_encoder_sw.o $(EC_COMMON_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) ec_encoder_sw.o $(EC_COMMON_OBJS) -o $@

ibv_ec_sw_encode_send: ec_sw_encode_send.o $(EC_COMMON_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) ec_sw_encode_send.o $(EC_COMMON_OBJS) -o $@

ibv_ec_decoder_verbs: ec_decoder_verbs.o $(EC_COMMON_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) ec_decoder_verbs.o $(EC_COMMON_OBJS) -o $@

ibv_ec_decoder_sw: ec_decoder_sw.o $(EC_COMMON_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) ec_decoder_sw.o $(EC_COMMON_OBJS) -o $@

install:
	install -d -m 755 $(PREFIX)/$(sbindir)
	install -m 755 $(TARGETS) $(PREFIX)/$(sbindir)
clean:
	rm -f $(OBJECTS_LAT) $(TARGETS)
