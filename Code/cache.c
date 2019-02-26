#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
 
#ifndef data_ways 
#define data_ways 4
#endif

#ifndef ins_ways
#define ins_ways 2
#endif

#ifndef data_sets
#define data_sets 16*1024
#endif

#ifndef ins_sets
#define ins_sets 16*1024
#endif

#ifndef Dbytes
#define Dbytes 64
#endif

#ifndef Ibytes
#define Ibytes 64
#endif

#ifndef addr_bits
#define addr_bits 32
#endif

uint32_t address;		//32 bit address value
int instruction;		//instruction - 0 1 2 3 4 8 9

int byte_select;		//byte select from data
int indexB;			//index value
int tag;			//tag value

int hit = 0;			//counter for data hits
int miss = 0;			//counter for data misses
int requests = 0;		//total data requests = hits + misses
int i_hit = 0;			//instruction hits
int i_miss = 0;			//instruction misses
int i_requests = 0;

int d_reads = 0;		//data cache read
int d_writes = 0;		//data cache writes
int i_reads = 0;		//ins cache reads


int data_hit = 0;		//used for if else conditions
int data_miss = 0;		//used for if else conditions

int mode = 0;
	
#define blockD log2(Dbytes)	//remove byte select bits while calculating index bits
#define blockI log2(Ibytes)
#define indexD log2(data_sets)	//20	- remove index and byte bits- calculate tag 
#define indexI log2(ins_sets)
 

//struct that holds bits found in a line
struct lineD {
	char state;
	int counter;
	int tag_bits;
	int data[Dbytes];

	int write;
};

struct lineI {
	char state;
	int counter;
	int tag_bits;
	int data[Ibytes];

};

//l1 data cache with 4 ways and 16k sets
struct l1_d {

	struct lineD way[data_ways];
}l1_data[data_sets];

struct l1_i {

	struct lineI way[ins_ways];
}l1_ins[ins_sets];


//functions
void print_cache();
void clear_cache();
int cache_miss();
void cacheAction();
int read_data();
void initialize();
void LRU(int );
int LRU_replace();
void LRU_ins(int );
int LRU_replace_ins();
int cache_miss_ins();
void invalidate();
void write_data();
int read_ins();
int l2_snoop();

int main(int argc, char *argv[])
{
	if(argv[1] == NULL || argv[2] == NULL)
	{
	    printf("\n**Enter trace and mode**\n\n");
	    return 0;
	}

	double ratio;
        
       initialize();

	FILE * traceFile;
	FILE * add;
	traceFile = fopen(argv[1], "r");

	if(!strcmp(argv[2], "1"))
	    mode = 1;

	else
	    mode = 0;

	add = fopen("add.txt", "w+");

	if(traceFile == NULL)
	{
		printf("Unable to open trace File\n");
		return 0;
	}

	else
	{
	     while(fscanf(traceFile, "%d", &instruction) && !feof(traceFile))
	     {
		if((instruction != 8) && (instruction != 9))
		{
		    fscanf(traceFile, "%x", &address);
		    int i, j, k, m;

		    if(instruction != 2)
		    {		
		        i = addr_bits - blockD;
		        j = addr_bits - (indexD + blockD);
		        k = addr_bits - indexD;
		        m = blockD + indexD;
		    }

		    else
		    {
		        i = addr_bits - blockI;
		        j = addr_bits - (indexI + blockI);
		        k = addr_bits - indexI;
		        m = blockI + indexI;
		    }

		    byte_select = (address << i) >> i;
		    indexB = (address << j) >> k;
		    tag = (address >> m);

	            fprintf(add, "Instruction = %d, Address = %x \n", instruction, address);
		    fprintf(add, "Byte = %d, index = %d , Tag = 0x%x \n\n", byte_select, indexB, tag);
		}		
		
		cacheAction();
	     }
	}
	
	fclose(traceFile);
	fclose(add);

	ratio = (double)hit/requests * 100;

	printf("\n\n\nNumber of Data cache reads: %d\n", d_reads);
	printf("Number of Data cache writes: %d\n", d_writes);
	printf("Number of Data Cache Hits: %d\n", hit);
	printf("Number of Data Cache Misses: %d\n", miss);
	printf("Total Data Cache requests: %d\n", requests);
	printf("Data Cache Hit ratio: %.2f  \n\n", ratio);

	ratio = (double)i_hit/i_requests * 100;

	printf("\n");
	printf("Number of Instruction Cache reads: %d\n", i_reads);
	printf("Number of Instruction Cache Hits: %d\n", i_hit);
	printf("Number of Instruction Cache Misses: %d\n", i_miss);
	printf("Instruction Cache Hit ratio: %.2f \n\n", ratio);
	
	ratio = (double)(hit + i_hit)/(requests + i_requests) * 100;

	printf("\n");
	printf("Total Cache Hits: %d\n", (hit + i_hit));
	printf("Total Cache Misses: %d\n", (miss + i_miss));
	printf("Total Cache requests: %d\n", (requests + i_requests));
	printf("Total Cache Hit ratio: %.2f  \n\n", ratio);

	return 0;
}

void cacheAction()
{
//function that checks instruction number and calls the needed function

	switch(instruction)
	{
		case 0 : read_data();
		break;

		case 1 : write_data();
		break;
		
		case 2 : read_ins();
		break;

		case 3 : invalidate();
		break;

		case 4 : l2_snoop();
		break;

		case 8 : clear_cache();
		break;

		case 9 : print_cache();
		break;

		default : (printf("\n \n **Invalid Instruction: %d** \n\n",instruction));
		break;	
	}
}

int read_data()
{
//function that performs a read request to l1 data cache
//if valid and tag bits match, return data
//if not, performs cache miss - calls another function

	requests++;
	d_reads++;

	for(int i = 0; i < data_ways; i++)
	{
	    if(l1_data[indexB].way[i].state != 'I' && l1_data[indexB].way[i].tag_bits == tag)
	    {
		data_hit = 1;
		data_miss = 0;
		hit++;
		
		LRU(i);

		return l1_data[indexB].way[i].data[byte_select];
	    }

	    else
		data_miss = 1;
	}
		
	if(data_miss)
	{
	    miss++;
	    cache_miss();
 	}
}

void write_data()
{
	requests++;
	d_writes++;

	for(int i = 0; i < data_ways; i++)
	{
	    if(l1_data[indexB].way[i].state != 'I' && l1_data[indexB].way[i].tag_bits == tag)
	    {
		data_hit = 1;
		data_miss = 0;
		hit++;

		if(l1_data[indexB].way[i].write)  	//if not the first write	
		{
		    l1_data[indexB].way[i].state = 'M';			   
		}

		else		//if first write
		{
		    if(mode)
		        printf("\nWrite to L2                <0x%08x>", address - byte_select);

		    l1_data[indexB].way[i].write = 1;
		    l1_data[indexB].way[i].state = 'E';	
		}		

		LRU(i);

		break;
	    }

	    else
		data_miss = 1;
	}
		
	if(data_miss)
	{
	    miss++;
	    cache_miss();
 	}
}

void LRU(int way)
{
	for(int i = 0; i < data_ways; i++)
	{
	    if(l1_data[indexB].way[i].counter != 0 && l1_data[indexB].way[way].counter == 0)
	    {
		l1_data[indexB].way[i].counter++;
	    }

  	    else if(l1_data[indexB].way[i].counter != 0 && (l1_data[indexB].way[i].counter < l1_data[indexB].way[way].counter))
		l1_data[indexB].way[i].counter++;
	}
	
	l1_data[indexB].way[way].counter = 1;
}

int cache_miss()
{
	for(int i = 0; i < data_ways; i++)
	{
	    if(l1_data[indexB].way[i].state == 'I')
	    {
		if(instruction == 0)
		{
		    l1_data[indexB].way[i].state = 'S';
	            l1_data[indexB].way[i].tag_bits = tag;

	    	    data_hit = 1;
		    data_miss = 0;

		    if(mode)
		        printf("\nRead from L2               <0x%08x>", address - byte_select);

		    LRU(i);

		    return l1_data[indexB].way[i].data[byte_select];
		}

		else if(instruction == 1)
		{
		    l1_data[indexB].way[i].write = 1;
		    l1_data[indexB].way[i].state = 'E';
	            l1_data[indexB].way[i].tag_bits = tag;

	    	    data_hit = 1;
		    data_miss = 0;

		    if(mode)
		    {
		        printf("\nRead for Ownership from L2 <0x%08x>", address - byte_select);
			printf("\nWrite to L2                <0x%08x>", address - byte_select);
		    }

		    LRU(i);
	
		    return 0;
		}		
	    }

	    else
		data_miss = 1;
	}

	if(data_miss)
	    return LRU_replace();
}
	    
int LRU_replace()
{
	for(int i = 0; i < data_ways; i++)
	{
	    if(l1_data[indexB].way[i].counter == data_ways)
	    {

		if(l1_data[indexB].way[i].state == 'M')
		{
		    //generate address to write back all data bytes from index and tag
		    //byte select bits not needed since writing all data back
         	    int m = blockD + indexD;
		    int n = blockD;
		    int k = (l1_data[indexB].way[i].tag_bits << m);
		    int l = (indexB << n);
                    uint32_t back = k + l;

		    if(mode)
		        printf("\nWrite to L2                <0x%08x>", back); 
		}

		if(instruction == 1)
		{	     	    
		    l1_data[indexB].way[i].state = 'E';
	            l1_data[indexB].way[i].tag_bits = tag;
		    l1_data[indexB].way[i].write = 1;

		    if(mode)
		    {
		        printf("\nRead for Ownership from L2 <0x%08x>", address - byte_select);
			printf("\nWrite to L2                <0x%08x>", address - byte_select);
		    } 
	
		    LRU(i);
		    return 0;
		
		}

		else if(instruction == 0)
		{
		    l1_data[indexB].way[i].state = 'S';
	            l1_data[indexB].way[i].tag_bits = tag;
	
		    LRU(i);

		    return l1_data[indexB].way[i].data[byte_select];
		}   		
	    }
	}
}

int read_ins()
{
	i_requests++;
	i_reads++;

	for(int i = 0; i < ins_ways; i++)
	{
	    if(l1_ins[indexB].way[i].state != 'I' && l1_ins[indexB].way[i].tag_bits == tag)
	    {
		data_hit = 1;
		data_miss = 0;
		i_hit++;

		LRU_ins(i);

		return l1_ins[indexB].way[i].data[byte_select];
	    }

	    else
		data_miss = 1;
	}
		
	if(data_miss)
	{
	    i_miss++;
	    return cache_miss_ins();
 	}
}

void LRU_ins(int way)
{
	for(int i = 0; i < ins_ways; i++)
	{
	    if(l1_ins[indexB].way[i].counter != 0 && l1_ins[indexB].way[way].counter == 0)
	    {
		l1_ins[indexB].way[i].counter++;
	    }

  	    else if(l1_ins[indexB].way[i].counter != 0 && (l1_ins[indexB].way[i].counter < l1_ins[indexB].way[way].counter))
		l1_ins[indexB].way[i].counter++;
	}
	
	l1_ins[indexB].way[way].counter = 1;
}

int cache_miss_ins()
{
//sees if there an empty spot
//if there is stores tag bits and sets valid to 1
//if not, need to perform LRU eviction
//and then retur_insn data

	for(int i = 0; i < ins_ways; i++)
	{
	    if(l1_ins[indexB].way[i].state == 'I')
	    {
		l1_ins[indexB].way[i].state = 'S';
	        l1_ins[indexB].way[i].tag_bits = tag;

	    	data_hit = 1;
		data_miss = 0;

		if(mode)
		        printf("\nRead from L2               <0x%08x>", address - byte_select);

		LRU_ins(i);

		return l1_ins[indexB].way[i].data[byte_select];
	    }

	    else
		data_miss = 1;
	}

	if(data_miss)
	    LRU_replace_ins();
}
	    
int LRU_replace_ins()
{
	for(int i = 0; i < ins_ways; i++)
	{
	    if(l1_ins[indexB].way[i].counter == ins_ways)
	    {
		l1_ins[indexB].way[i].state = 'S';
	        l1_ins[indexB].way[i].tag_bits = tag;

		LRU_ins(i);

		return l1_ins[indexB].way[i].data[byte_select];
	    }
	}
}


int l2_snoop()
{	
	//requests++;
	for(int i = 0; i < data_ways; i++)
	{
	    if(l1_data[indexB].way[i].state != 'I' && l1_data[indexB].way[i].tag_bits == tag)
	    {
		l1_data[indexB].way[i].state = 'S';

		//LRU(i);

		if(mode && l1_data[indexB].way[i].state == 'M')
		    printf("\nReturn data to L2          <0x%08x>", address - byte_select);

		return l1_data[indexB].way[i].data[0];
	    }
	}

	return 0;
}

void invalidate()
{
	for(int i = 0; i < data_ways; i++)
	{
	    if((l1_data[indexB].way[i].state == 'E' || l1_data[indexB].way[i].state == 'S') && l1_data[indexB].way[i].tag_bits == tag)
	    {

		l1_data[indexB].way[i].state = 'I';

		for(int j = 0; j < data_ways; j++)
		{
		    if((i != j) && (l1_data[indexB].way[j].counter > l1_data[indexB].way[i].counter))
			l1_data[indexB].way[j].counter--;
		}

		l1_data[indexB].way[i].counter = 0;
		break;	
	    }	    
	}	
}	

void print_cache()
{
//prints contents of cache

	int valid = 0;

	printf("\n\n_______________________________________________________________________________");

	printf("\n\nData Cache \n\n");

	for(int i = 0; i < data_sets; i++)
	{
	    for(int j = 0; j < data_ways; j++)
	    {
		if(l1_data[i].way[j].state != 'I')
		    valid = 1;
	    }

	    if(valid)
		printf("ind:0x%05x  ", i);

	    for(int j = 0; j < data_ways; j++)
	    {
		if(valid)
		{
	    	   if(l1_data[i].way[j].state != 'I')
		   {
		       printf("LRU:%02d  ", l1_data[i].way[j].counter - 1);
     	               printf("State: %c  ", l1_data[i].way[j].state);
	               printf("tag: 0x%08x    ", l1_data[i].way[j].tag_bits); 
		    
	    	   }
		
	 	else
		    printf("                                     ");
		}

		if(data_ways > 4 && (((j + 1) % 4) == 0))
	            printf("\n             ");

 	    }

   
	    if(valid)
	    {
	        printf("\n");
		valid = 0;
	    }
	}

	printf("\n\nInstruction Cache \n\n");

	for(int i = 0; i < ins_sets; i++)
	{
	    for(int j = 0; j < ins_ways; j++)
	    {
		if(l1_ins[i].way[j].state != 'I')
		    valid = 1;
	    }

	    if(valid)
		printf("ind:0x%05x  ", i);

	    for(int j = 0; j < ins_ways; j++)
	    {
		if(valid)
		{
	    	   if(l1_ins[i].way[j].state != 'I')
		   {
		       printf("LRU:%02d  ", l1_ins[i].way[j].counter - 1);
     	               printf("State: %c  ", l1_ins[i].way[j].state);
	               printf("tag: 0x%08x    ", l1_ins[i].way[j].tag_bits); 
		    
	    	   }
		
	 	else
		    printf("                                     ");
		}

		if(ins_ways > 4 && (((j + 1) % 4) == 0))
	            printf("\n             ");

 	    }
   
	    if(valid)
	    {
	        printf("\n");
		valid = 0;
	    }
	}

	printf("_______________________________________________________________________________\n");
}

void clear_cache()
{
//sets all valid bits to 0
//clears statistics

	for(int i = 0; i < data_sets; i++)
	{
	    for(int j = 0; j < data_ways; j++)	
	    {    
 	        l1_data[i].way[j].state = 'I';
		l1_data[i].way[j].counter = 0;
		l1_data[i].way[j].write = 0;
	    }
	}
	
	for(int i = 0; i < ins_sets; i++)
	{
	    for(int j = 0; j < ins_ways; j++)	
	    {    
		l1_ins[i].way[j].state = 'I';
		l1_ins[i].way[j].counter = 0;
	    }
	}
	        
	d_reads = 0;
	d_writes = 0;
	i_reads = 0;

	hit = 0;
	miss = 0;
	requests = 0;

	i_hit = 0;
	i_miss = 0;
	i_requests = 0;
	
}

void initialize()
{
//dummy function to put in random data into data array
//sets valid bits to 0 and tag bits to 0 - so while printing no issues arise
//called at the start of the program


	for(int i = 0; i < data_sets; i++)
	{
	    for(int j = 0; j < data_ways; j++)
	    {
		for(int k = 0; k < Dbytes; k++)
		{
		    l1_data[i].way[j].data[k] = k;
		}

		l1_data[i].way[j].state = 'I';
	    	l1_data[i].way[j].counter = 0; 
		l1_data[i].way[j].write = 0;
	    }
	}   

	for(int i = 0; i < ins_sets; i++)
	{
	    for(int j = 0; j < ins_ways; j++)
	    {
		for(int k = 0; k < Ibytes; k++)
		{
		    l1_ins[i].way[j].data[k] = k;
		}

		l1_ins[i].way[j].state = 'I';
	    	l1_ins[i].way[j].counter = 0; 
	    }
	}   



	
}
	

