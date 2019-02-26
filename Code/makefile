data_sets = 16*1024
data_ways = 4
Dbytes = 64

ins_sets = 16*1024
ins_ways = 2
Ibytes = 64

addr_bits =32

CC = gcc ## GCC compiler
CFLAGS = -std=c99
FILELIST = cache.c
OUT_DEFINED = cache_1
OUT_NEW = cache_exe


cache_comp:
	$(CC) $(CFLAGS) $(FILELIST) -o $(OUT_DEFINED) -lm

cache :
	$(CC) $(CFLAGS) -Ddata_ways=$(data_ways) -Ddata_sets=$(data_sets) -Dins_sets=$(ins_sets) -Dins_ways=$(ins_ways) -DDbytes=$(Dbytes) -DIbytes=$(Ibytes) -Daddr_bits=$(addr_bits) -o $(OUT_NEW) $(FILELIST) -lm


.PHONY : clean
clean : 
	rm -rf cache_*

