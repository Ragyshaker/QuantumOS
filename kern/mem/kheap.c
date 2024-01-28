#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"
struct vaSize{
	void* address;
	uint32 numberOfFrames;
};
int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
//	cprintf("calling initialize dynamic allocator\n");
	if(daStart+ROUNDUP(initSizeToAllocate,PAGE_SIZE) >= daLimit||free_frame_list.size==0){
		return E_NO_MEM;
	}
    uint32 frameAddress=daStart;
	start=daStart;
	segmentBreak=daStart+ROUNDUP(initSizeToAllocate,PAGE_SIZE);
	hardLimt=daLimit;
	//cprintf("the initialsize is %d\n",initSizeToAllocate);
	int counter = ROUNDUP(initSizeToAllocate,PAGE_SIZE)/PAGE_SIZE;;
	struct FrameInfo *ptr;
	//cprintf("counter: %d\n",counter);
	//cprintf("first %d\n",free_frame_list.size);
    for(int i=0 ; i<counter ; i++){
      int ret= allocate_frame(&ptr);
      if(ret==E_NO_MEM){
    	  return  E_NO_MEM;
      }
	  map_frame(ptr_page_directory,ptr,frameAddress,PERM_WRITEABLE|PERM_PRESENT);
	  frameAddress+=PAGE_SIZE;
    }
    initialize_dynamic_allocator(daStart,ROUNDUP(initSizeToAllocate,PAGE_SIZE));
	return 0;
}

void* sbrk(int increment)
{
	//TODO: [PROJECT'23.MS2 - #02] [1] KERNEL HEAP - sbrk()
  // cprintf("\ncalling sbrk\n");
     if(increment == 0)
    	 return (void*) segmentBreak;

     else if(increment>0){
    	 uint32 nextBoundry=ROUNDUP(segmentBreak,PAGE_SIZE) ;
    	 if(nextBoundry==segmentBreak){
    	 if((segmentBreak+ROUNDUP(increment,PAGE_SIZE)) > hardLimt || free_frame_list.size==0){
    		 panic("NO enough memory\n");
    	 }
    	 int numberOfPages= ROUNDUP(increment,PAGE_SIZE);
    	 numberOfPages/=PAGE_SIZE;
    	 uint32 breakseg=segmentBreak;
    	 struct FrameInfo *ptr;
    	 for(int i=0; i < numberOfPages ; i++){
    		 int ret= allocate_frame(&ptr);
    		 if(ret==E_NO_MEM){
    		   panic("NO enough memory\n");
    		 }
    		// allocate_frame(&ptr);//the bugy line in MS2 elhmdllah
    		 map_frame(ptr_page_directory,ptr,(segmentBreak),PERM_WRITEABLE|PERM_PRESENT);
    		 segmentBreak+=PAGE_SIZE;
    	 }
    	 return (void*)breakseg;
    	 }
    	 else{
    		 if((segmentBreak+increment) > hardLimt ){
    		    	 panic("NO enough memory\n");
    		    }
    		 uint32 c= nextBoundry - segmentBreak;
    		  if(increment>c){
    		 if((segmentBreak+ROUNDUP(increment,PAGE_SIZE)) > hardLimt || free_frame_list.size==0){
    			     		 panic("NO enough memory\n");
    		 }
    			 uint32 breakseg=segmentBreak;
    			  increment-=c;
    			  segmentBreak+=c;
    			 int numberOfPages= ROUNDUP(increment,PAGE_SIZE);
    		  	 numberOfPages/=PAGE_SIZE;
    		    struct FrameInfo *ptr;
    			 for(int i=0; i < numberOfPages ; i++){
    			 allocate_frame(&ptr);//take care to handle the case if no free frames in the FFL
    			 map_frame(ptr_page_directory,ptr,segmentBreak,PERM_WRITEABLE|PERM_PRESENT);
    			 segmentBreak+=PAGE_SIZE;
    			 }
    			 return (void*)breakseg;
    		  }
    		  else{
    			  uint32 breakseg=segmentBreak;
    			  segmentBreak+=c;
    			  return (void*)breakseg;
    		  }
    	 }

     }
     else {
    	 increment*=-1;
    	 uint32 nextBoundry=ROUNDDOWN(segmentBreak,PAGE_SIZE)  ;
    	  if(nextBoundry==segmentBreak){//case 1 the brk  is aligned
    		  int numberOfPages= increment/PAGE_SIZE ;
    		  for(int i=0;i<numberOfPages;i++){
    		    unmap_frame(ptr_page_directory,segmentBreak-1);
    		    segmentBreak-=PAGE_SIZE;
    		  }
    		 segmentBreak-=(increment%PAGE_SIZE);
    	  }
    	  else{//case 2 the brk  is not aligned
    		  uint32 c= segmentBreak-nextBoundry;
    		  if(increment>c){
    			  unmap_frame(ptr_page_directory,segmentBreak-1);
    			  increment-=c;
    			  segmentBreak-=c;
    			  int numberOfPages= increment/PAGE_SIZE ;
    	      for(int i=0;i<numberOfPages;i++){
    			    unmap_frame(ptr_page_directory,segmentBreak-1);
    			    segmentBreak-=PAGE_SIZE;
    			     		     	    }
    		 segmentBreak-=(increment%PAGE_SIZE);
    		  }
    		  else{
    			  segmentBreak-=increment;
    		  }
    	  }

    	return (void*)segmentBreak;

     }

	//return (void*)-1;
}

int i=0;
struct vaSize array[1500];
void* kmalloc(unsigned int size)
{
	//cprintf("calling kmalloc\n");
	if(isKHeapPlacementStrategyFIRSTFIT())
	{
   // char*kernmax="0xFFFFF000";
   // uint32 max=strtol(kernmax,NULL,16);
   // cprintf("\nkernel heap max address %u \n",max);
   // cprintf("the address to be compared %u \n",(hardLimt+PAGE_SIZE+size));
	//cprintf("el call rakm %d \n",i);
	if(size <= DYN_ALLOC_MAX_BLOCK_SIZE){                  //block allocator
	 //  cprintf("\nI am at the case of block allocator from the kmalloc\n");
	   return alloc_block_FF(size);
	}
    else if(size>DYN_ALLOC_MAX_SIZE/*32*1024*1024*/)
    {
	   return NULL;
    }
    else{//page allocator

    	int ourFr=ROUNDUP(size,PAGE_SIZE);
    	int counter=0;
    	bool flag=0;
    	ourFr/=PAGE_SIZE;
    	uint32 allocAddress=0;
    	if(ourFr>free_frame_list.size){
    		//cprintf("\n\nno free frames from the Kmalloc\n\n");
    		return NULL;
    	}
    //	cprintf("number of pages%d\n",numOfFrames);
    	uint32 *ptr_page=NULL;
    	for(uint32 i=hardLimt+PAGE_SIZE ; i<KERNEL_HEAP_MAX;i+=PAGE_SIZE){
        struct  FrameInfo *p= get_frame_info(ptr_page_directory,(i),&ptr_page);
    	if(p==(void*)0){
    		counter++;
    	}
    	else{
    		counter=0;
    	}
    	if(counter==ourFr){
    		allocAddress= i-((counter-1)*PAGE_SIZE);
    			break;
    	}
    	}
    	if(counter>=ourFr)
    	{
    		uint32 temp=allocAddress;
    		struct FrameInfo *ptr;
    		for(int i=0;i<ourFr;i++){
    		allocate_frame(&ptr);
            map_frame(ptr_page_directory,ptr,allocAddress,PERM_WRITEABLE|PERM_PRESENT);
            //  cprintf("\nI am at the case of pagee allocator from the kmalloc %x\n",allocAddress);
            allocAddress+=PAGE_SIZE;
    		}
    		array[i].address=(void*) temp;
    		array[i].numberOfFrames=ourFr;
    		i++;
    		return (void*) temp;
    		}
    	    else{
    			return NULL;
    			}
    	//	}
    	}
	}
 return NULL;
}

void kfree(void* virtual_address)
{
	//cprintf("\ncalling Kfree\n");
	//uint32*ptr_page_table;
	uint32 va=(uint32)virtual_address;

	//char*kernmax="0xFFFFF000";
	//uint32 max=strtol(kernmax,NULL,16);
	if(va>=start&&va<=segmentBreak){
		free_block(virtual_address);
	}
	else if(va>=hardLimt+PAGE_SIZE && va<=KERNEL_HEAP_MAX){
		virtual_address=ROUNDDOWN(virtual_address,PAGE_SIZE);
		//  struct  FrameInfo *ptr= get_frame_info(ptr_page_directory,va,&ptr_page_table);
		  uint32 add=va;
		  int j;
		  for(j=0;j<=i;j++){
			  if(virtual_address==array[j].address){
				  break;
			  }
		  }
		for(int k=0;k<array[j].numberOfFrames;k++){
			unmap_frame(ptr_page_directory,add);
			add+=PAGE_SIZE;
		}
		array[j].numberOfFrames=0;
		array[j].address=0;
	}
	else{
		panic("invalid address\n");
	}

}
unsigned int kheap_virtual_address(unsigned int physical_address) {
	//to_frame_info(physical_address);
	//cprintf("number of refernces %d\n",fr->references);
	if(PPN(physical_address)>=number_of_frames){
		// cprintf("\nkheap_virtual_address:: %x\n",0);
		return 0;
	}
	else if(frames_info[PPN(physical_address)].references==0){
		// cprintf("\nkheap_virtual_address:: %x\n",0);
		 return 0;
	 }
	 else {
	//	 cprintf("\nkheap_virtual_address:: %x\n",frames_info[PPN(physical_address)].va +(physical_address&0xFFF));
		return  frames_info[PPN(physical_address)].va +(physical_address&0xFFF);
	 }
	 return 0;
}

unsigned int kheap_physical_address(unsigned int virtual_address) {
	//cprintf("\nkheap_physical_address\n");
	//uint32*	ptr_page_table;
   // char*kernmax="0xFFFFF000";
    //uint32 max=strtol(kernmax,NULL,16);
	uint32 *ptr_page=NULL;
	struct  FrameInfo *p= get_frame_info(ptr_page_directory,virtual_address,&ptr_page);
	if(p==NULL){
		return 0;
	}
    if(virtual_address>=hardLimt&&virtual_address<hardLimt+PAGE_SIZE){
    	return 0;
    }
    if (virtual_address>=start && virtual_address<KERNEL_HEAP_MAX) {
     uint32* page_table ;
     get_page_table(ptr_page_directory,virtual_address, &page_table);
     uint32 entry = page_table[PTX(virtual_address)] >> 12;
   //  cprintf("frame number->%d\n",entry);
     unsigned int PA = (entry << 12) + ((virtual_address)& 0xFFF );
     return PA;
    }
    else{
    return 0;
    }
}

void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'23.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc()
	// Write your code here, remove the panic and write your code
	if(virtual_address==NULL&&new_size!=0&&new_size>DYN_ALLOC_MAX_BLOCK_SIZE){
		return kmalloc(new_size);
	}
	else if(new_size==0 && virtual_address!=NULL&& (uint32)virtual_address<=KERNEL_HEAP_MAX&&(uint32)virtual_address>=hardLimt+PAGE_SIZE){
		kfree(virtual_address);
	}
	else if(new_size==0&&virtual_address!=NULL&&(uint32)virtual_address<=segmentBreak&&(uint32)virtual_address>=start){
		free_block(virtual_address);
		}
	else if(virtual_address==NULL&&new_size!=0&&new_size<=DYN_ALLOC_MAX_BLOCK_SIZE){
		return alloc_block_FF(new_size);
	}
	else if(new_size<=0&&virtual_address==NULL){
		return NULL;
	}
	 int j;

	 if((uint32)virtual_address<=KERNEL_HEAP_MAX&&(uint32)virtual_address>=hardLimt+PAGE_SIZE){
	    if(new_size<= DYN_ALLOC_MAX_BLOCK_SIZE){
	    kfree(virtual_address);
	    return	alloc_block_FF(new_size);
	    }

	 for(j=0;j<=i;j++){
	   if(virtual_address==array[j].address){
		  break;
	   }


	int numOfFramesOfNewSize=ROUNDUP(new_size,PAGE_SIZE);
    if(numOfFramesOfNewSize>(array[j].numberOfFrames)){   //increment
    	bool flag=0;
    	int counter=0;
    	int diffrence=numOfFramesOfNewSize-array[j].numberOfFrames;
    	for(uint32 i=(uint32)virtual_address+PAGE_SIZE ; i<=KERNEL_HEAP_MAX;i+=PAGE_SIZE){
    	uint32 *ptr_page=NULL;
        struct  FrameInfo *p= get_frame_info(ptr_page_directory,i,&ptr_page);
    	if(p==NULL){
    	 	counter++;
    	 }
    	 else
    	 {
    	  	counter=0;
    	  	break;
    	 }
    	 if(counter==diffrence){
    		struct FrameInfo *ptr;
    		int  allocAddress;
    	    allocAddress= i-((counter-1)*PAGE_SIZE);
    	    for(int k=0;k<diffrence;k++){
    	    allocate_frame(&ptr);//take care to handle the case if no free frames in the FFL
    	    map_frame(ptr_page_directory,ptr, allocAddress,PERM_WRITEABLE|PERM_PRESENT);
    	    allocAddress+=PAGE_SIZE;
    	    }
    	   	break;
    	 }
    	}


    }
	 }

	 }
	 else if((uint32)virtual_address<=segmentBreak&&(uint32)virtual_address>=start){
		if(new_size>DYN_ALLOC_MAX_BLOCK_SIZE) {
			free_block(virtual_address);
		   return kmalloc(new_size);
		}
		return realloc_block_FF(virtual_address, new_size);
	 }


	return NULL;
}
