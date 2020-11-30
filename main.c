
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <alloca.h>
#include <math.h>


#define FRAME_SIZE 256          //FRAME SIZE is 256 bytes 2^8
#define FRAMES 256              //Number of frames
#define PAGE_SIZE 256           // PAGE SIZE is 256 bytes 2^8
#define TLB_SIZE 16             //Number of TLB Entries
#define PAGE_MASKER 0xFFFF
#define OFFSET_MASKER 0xFF
#define ADDRESS_SIZE 10
#define CHUNK 256

// input files
FILE *address_file;
FILE *backing_store;

// struct to store TLB and page table
struct page_frame
{
    int page_number;
    int frame_number;
};

// initial global variables
int Physical_Memory [FRAMES][FRAME_SIZE];

struct page_frame TLB[TLB_SIZE];
struct page_frame PAGE_TABLE[FRAMES]; 

int translated_addresses = 0;
char address [ADDRESS_SIZE]; // to store the address coming from the input file
int TLBHits = 0;
int page_faults = 0;
signed char buffer[CHUNK];
int firstAvailableFrame = 0;
int firstAvailablePageTableIndex = 0;
signed char value;
int TLB_Full_Entries = 0;

// functions placeholders
void get_page(int logical_address);
int read_from_store(int pageNumber);
void insert_into_TLB(int pageNumber, int frameNumber);

// get a page for a given virtual address
void get_page(int logical_address){
    
    int pageNumber = ((logical_address & PAGE_MASKER)>>8);
    int offset = (logical_address & OFFSET_MASKER);
    
    // first try to get page from TLB
    int frameNumber = -1; // initialized to -1 to tell if it's valid in the conditionals below
    
    int i;  // look through TLB for a match
    for(i = 0; i < TLB_SIZE; i++){
        if(TLB[i].page_number == pageNumber){
            frameNumber = TLB[i].frame_number;
            TLBHits++;
            
        }
    }
    
    // if the frameNumber was not int the TLB:
    if(frameNumber == -1){
        int i;
        for(i = 0; i < firstAvailablePageTableIndex; i++){
            if(PAGE_TABLE[i].page_number == pageNumber){
                frameNumber = PAGE_TABLE[i].frame_number;
            }
        }
        
        // if the page is not found in those contents
        if(frameNumber == -1){
            frameNumber = read_from_store(pageNumber);
            page_faults++;
        }
    }
    
    insert_into_TLB(pageNumber, frameNumber);
    value = Physical_Memory[frameNumber][offset];
    
    
    // output final values to a file named results
    FILE *res = fopen("correct.txt","a");
    fprintf(res,"Virtual address: %d Physical address: %d Value: %d\n", logical_address, (frameNumber << 8) | offset, value);
    fclose(res);
}

int read_from_store(int pageNumber){
    
    if (fseek(backing_store, pageNumber * CHUNK, SEEK_SET) != 0) {
        fprintf(stderr, "Error seeking in backing store\n");
    }
    
    if (fread(buffer, sizeof(signed char), CHUNK, backing_store) == 0) {
        fprintf(stderr, "Error reading from backing store\n");
    }
    
    // load the bits into the first available frame in the physical memory 2D array
    int i;
    for(i = 0; i < CHUNK; i++){
        Physical_Memory[firstAvailableFrame][i] = buffer[i];
    }
    
    // and then load the frame number into the page table in the first available frame
    PAGE_TABLE[firstAvailablePageTableIndex].page_number = pageNumber;
    PAGE_TABLE[firstAvailablePageTableIndex].frame_number = firstAvailableFrame;
    firstAvailableFrame++;
    firstAvailablePageTableIndex++;
    return PAGE_TABLE[firstAvailablePageTableIndex-1].frame_number;
}

void insert_into_TLB(int pageNumber, int frameNumber){

    
    for(int i = 0; i < TLB_Full_Entries; i++){
        if(TLB[i].page_number == pageNumber){
            for(i = i; i < TLB_Full_Entries - 1; i++)
                TLB[i] = TLB[i + 1];
            break;
        }
    }
    
    // if the previous loop did not break, then the page is not in the TLB. We need to insert it.
    int i = 0;
    if( i == TLB_Full_Entries){
        int j;
        for (j=0; j<i; j++)
            TLB[j] = TLB[j+1];
        
    }
    TLB[i].page_number = pageNumber;        // and insert the page and frame on the end
    TLB[i].frame_number = frameNumber;
    
    if(TLB_Full_Entries < TLB_SIZE-1){     // if there is still room in the arrays, increment the number of entries
        TLB_Full_Entries++;
    }
}

int main(int argc, char *argv[]) {
 
    printf("results are printed to the \"correct.txt\" file.\n");
   
    

    address_file = fopen(argv[1], "r");
    
    backing_store = fopen("BACKING_STORE.bin", "rb");
    
    if (address_file == NULL) {
        fprintf(stderr, "Error opening addresses.txt %s\n",argv[1]);
        return -1;
    }
    
    if (backing_store == NULL) {
        fprintf(stderr, "Error opening BACKING_STORE.bin %s\n","BACKING_STORE.bin");
        return -1;
    }
    int translated_addresses = 0;
    int logical_address;
  
    while (fgets(address, ADDRESS_SIZE, address_file) != NULL) {
        logical_address = atoi(address); // this function converts numeric strings to int datatype.
        
        // get the physical address and value stored at that address
        get_page(logical_address);
        translated_addresses++;
    }
    
    // calculate and print out the stats
    printf("Number of translated addresses = %d\n", translated_addresses);
    double pfRate = page_faults / (double)translated_addresses;
    double TLBRate = TLBHits / (double)translated_addresses;
    
    printf("Page Faults = %d\n", page_faults);
    printf("Page Fault Rate = %.3f\n",pfRate);
    printf("TLB Hits = %d\n", TLBHits);
    printf("TLB Hit Rate = %.3f\n", TLBRate);
    
    // close the input file and backing store
    fclose(address_file);
    fclose(backing_store);
    
    return 0;
}
