# Erasure Coding NIC Offload benchmark
Erasure coding (EC) is a method of data protection in which data is broken into fragments,  expanded and encoded with redundant data pieces and stored across a set of different locations or storage media.  

Data encoding/decoding is very CPU intensive and can be a major overhead when using Erasure coding.  

This is my benchmark test on Mellanox ConnectX-5 EN adapter cards' ECO(Erasure Coding Offloading) capabilities.

The code in this repository is modified from the sample test code in Mellaxnox's EC repository(https://github.com/Mellanox/EC.git).

### Prerequisites
1. Mellanox ConnectX-4 or ConnectX-4 Lx.
2. MLNX_OFED_LINUX-3.3-1.0.0.0 (or later).
2. Firmware - v12.16.1006 for ConnectX-4 (or later), v14.16.1006 for ConnectX-4 Lx  (or later).
3. Jerasure library v2.0. (https://github.com/tsuraan/Jerasure).

### Recommendations
1. install Jerasure & GF-Complete using the following parameters:  
    
        ./configure --prefix=/usr/ --libdir=/usr/lib64/
2. It is highly recommended to install ConnectX庐-4 on PCIe3.0 x16, and ConnectX庐-4 Lx on PCIe3.0 x8 slot for better performance.
3. Use block size aligned to 64 bytes to avoid copying to remainder to internal buffers.

### Tests

Test code resides in `tests` folder.

**Build**  
1. cd tests  
2. make

**ibv_ec_capability_test**

Checking EC offload capabilities for IB devices.

*Usage*  

        ./ibv_ec_capability_test
        Usage = ./ec_capability_test <device_name>
        Available devices : <List of available IB devices>

**ibv_ec_encoder**

Perform encode operations on input files using Erasure Coding NIC Offload library, Erasure Coding Offload Experimental Verbs API and Jerasure library.  
Galois field GF(2^w) must be equal to GF(2^4).  
The test will encode the input file three times (all the results should be equal):  
1. using Erasure Coding NIC Offload library - the results will be written into <inputFile>.encode.code.eco  
2. using Experimental Verbs API - the results will be written into <inputFile>.encode.code.verbs  
3. using Jerasure Library - the results will be written into <inputFile>.encode.code.sw

*Usage*  

    Usage:
    ./ibv_ec_encoder            start EC encoder
    
    Options:
      -i, --ib-dev=<dev>         use IB device <dev> (default first device found)
      -k, --data_blocks=<blocks> Number of data blocks
      -m, --code_blocks=<blocks> Number of code blocks
      -w, --gf=<gf>              Galois field GF(2^w)
      -D, --datafile=<name>      Name of input file to encode
      -s, --frame_size=<size>    size of EC frame
      -d, --debug                print debug messages
      -v, --verbose              add verbosity
      -h, --help                 display this output

**ibv_ec_encoder_mem**

Perform encode operations on data blocks generated in memory using Erasure Coding Offload Experimental Verbs API and Jerasure library.  
Galois field GF(2^w) must be equal to GF(2^4). 
The test will encode the data blocks generated in memory with multiple threads and output the elapsed time(wall time) for encode operations.
1. The size of the "file" to be encoded should be specified in command line arguments. 
2. The total number of encode operations E = file_size / frame_size. All threads share a single counter E, each time a thread removes a batch of work from E until E reaches zero.
3. Since Erasure Coding NIC offload library is not thread safe, we can only choose between Experimental Verbs API and Jerasure Library.

*Usage*  

    Usage:
    ./ibv_ec_encoder_mem            start EC encoder
    
    Options:
	  -i, --ib-dev=<dev>           use IB device <dev> (default first device found)
	  -k, --data_blocks=<blocks>   Number of data blocks
	  -m, --code_blocks=<blocks>   Number of code blocks
	  -w, --gf=<gf>                Galois field GF(2^w)
	  -F, --file_size=<size>       size of file in bytes
	  -s, --frame_size=<size>      size of EC frame
	  -t, --thread_number=<number> number of threads used
	  -V, --verbs                  use verbs api
	  -S, --software_ec            use software EC(Jerasure library)
	  -L, --eco_lib                use mellanox ECO library(mlx_lib)
	  -d, --debug                  print debug messages
	  -v, --verbose                add verbosity
	  -h, --help                   display this output


**ibv_ec_decoder**

Perform decode operations on input files using Erasure Coding NIC Offload library and Erasure Coding Offload
Experimental Verbs API.  
Galois field GF(2^w) must be equal to GF(2^4).  
The test will decode the input file two times (The code result should be equal to the input code file, data results should be equal to the input file):  
1. using  Erasure Coding NIC Offload library - the data results will be written into <inputFile>.decode.data.eco  
                                               the code results will be written into <inputFile>.decode.code.eco  
2. using Experimental Verbs API - the data results will be written into <inputFile>.decode.data.verbs

*Usage*  

    Usage:
      ./ibv_ec_decoder            start EC decoder
    
    Options:
      -i, --ib-dev=<dev>         use IB device <dev> (default first device found)
      -k, --data_blocks=<blocks> Number of data blocks
      -m, --code_blocks=<blocks> Number of code blocks
      -w, --gf=<gf>              Galois field GF(2^w)
      -D, --datafile=<name>      Name of input data file
      -C, --codefile=<name>      Name of input code file
      -E, --erasures=<erasures>  Comma separated failed blocks
      -s, --frame_size=<size>    size of EC frame
      -d, --debug                print debug messages
      -v, --verbose              add verbosity
      -h, --help                 display this output

### Benchmark

Benchmark scripts resides in `bench` folder.

**best_block_size**
Determine the best block size for ECO and Jerasure library.
Use program ibv_ec_encoder_mem.
The encoder is single-threaded.
For ECO, we use sync verbs.

**max_throughput**
Determine the maximum throughput for ECO and Jerasure library.
Use program ibv_ec_encoder_mem.
The encoder is multi-threaded.
For ECO, we use sync verbs.

