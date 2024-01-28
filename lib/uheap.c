#include <inc/lib.h>

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
uint32 arr[1000];
int i=0;
int FirstTimeFlag = 1;
void InitializeUHeap()
{
	if(FirstTimeFlag)
	{
#if UHP_USE_BUDDY
		initialize_buddy();
		cprintf("BUDDY SYSTEM IS INITIALIZED\n");
#endif
		FirstTimeFlag = 0;
	}
}
struct VAS{
	void* address;
	uint32 numberOfFrames;
	int startIndex;
};
//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment)
{
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
//const int Arraysize=ROUNDUP(USER_HEAP_MAX-(getHardLimt+PAGE_SIZE),PAGE_SIZE)/PAGE_SIZE;
bool heapArray[132000]={0};
struct VAS freeArray[8000];
int counterForFreeArray=0;
void* malloc(uint32 size)
{

	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0) return NULL ;
	//==============================================================
	// char*usermax="0xA0000000";
	// uint32 max=strtol(usermax,NULL,16);
	//cprintf("calling malloc\n 1\n");
	if(size <= DYN_ALLOC_MAX_BLOCK_SIZE){
	//	cprintf("block allocator case 1\n");//block allocator
		return alloc_block_FF(size);
	 }
	else{
		int numOfFrames=ROUNDUP(size,PAGE_SIZE);
	    int counter=0;
	    int Firstindex=0;
		bool flag=0;
		int counterForHEAPArray=0;
		numOfFrames/=PAGE_SIZE;
		uint32 allocAddress=0;
		uint32 loopStart=getHardLimt() + PAGE_SIZE ;
       // cprintf("\nthe hard limt passed to malloc is %x\n",loopStart-PAGE_SIZE);
		for(uint32 i= loopStart ; i<USER_HEAP_MAX;i+=PAGE_SIZE){
		    if(heapArray[counterForHEAPArray]==0){
		    	counter++;
		    	if(!flag){
		    		Firstindex=counterForHEAPArray;
		    		flag=1;
		    	}
		    	}
		    else{
		    	counter=0;
		    	flag=0;
		   	}
		  	if(counter==numOfFrames){
		    allocAddress = i-((counter-1)*PAGE_SIZE);
		    freeArray[counterForFreeArray].address=(void*)allocAddress;
		    freeArray[counterForFreeArray].numberOfFrames=numOfFrames;
		    freeArray[counterForFreeArray].startIndex=Firstindex;
		    counterForFreeArray++;
		    break;
		 	}
		  	counterForHEAPArray++;
		}
		if(counter>=numOfFrames)
		{
		for(int i=0;i<numOfFrames;i++){
			heapArray[Firstindex]=1;
			Firstindex++;
		 }
		 sys_allocate_user_mem(allocAddress,size);
		   return (void*) allocAddress;
		}
		else{
		  return NULL;
		}

	}
}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #11] [2] USER HEAP - free() [User Side]
	//cprintf("calling free user side\n 1\n");
	uint32 va=(uint32)virtual_address;
	uint32 userHardLimt=getHardLimt();
//assuming the that the block allocator starts from USER_HEAP_START if not you will need to get the environment block allocator start address
	if(va<=userHardLimt&&va>=USER_HEAP_START){
		free_block(virtual_address);
	}
	else if(va>=userHardLimt+PAGE_SIZE && va<USER_HEAP_MAX){
	 int i=0;
	 bool flag=0;
	 int index=0;
	  for( i=0;i<=counterForFreeArray;i++){
		 if(virtual_address==freeArray[i].address){
			 index=freeArray[i].startIndex;
			 flag=1;
			 break;
		  }
	  }
     if(flag){
	  for(int k=0;k<freeArray[i].numberOfFrames;k++){
		  heapArray[index]=0;
		  index++;
	  }
	  uint32 sizeToDelete=freeArray[i].numberOfFrames*PAGE_SIZE;
	  freeArray[i].numberOfFrames=0;
	  freeArray[i].address=0;
	  freeArray[i].startIndex=0;
	  sys_free_user_mem(va,sizeToDelete);

	}}
	else{
		panic("invalid address");
	}



}


//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0) return NULL ;
	//==============================================================
	panic("smalloc() is not implemented yet...!!");
	return NULL;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================
	// Write your code here, remove the panic and write your code
	panic("sget() is not implemented yet...!!");
	return NULL;
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================

	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}


//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}


//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");

}
