/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"

struct BlockMetaData * add;
struct BlockMetaData * last;
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
uint32 get_block_size(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->size ;
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
int8 is_free_block(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->is_free ;
}

//===========================================
// 3) ALLOCATE BLOCK BASED ON GIVEN STRATEGY:
//===========================================
void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockMetaData* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", blk->size, blk->is_free) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
//struct MemBlock_LIST list;
bool is_initialized = 0;
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//=========================================
	//DON'T CHANGE THESE LINES=================
	if (initSizeOfAllocatedSpace == 0)
		return ;
	is_initialized=1;
    struct BlockMetaData* metaData;
    void *va =(void*)daStart;
    //add=va;
    metaData=va;
    LIST_INIT(&list);
    metaData->size=initSizeOfAllocatedSpace;
    metaData->is_free=1;
    LIST_INSERT_HEAD(&list,metaData);
	//TODO: [PROJECT'23.MS1 - #5] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator()
	//panic("initialize_dynamic_allocator is not implemented yet");
}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================

void *alloc_block_FF(uint32 size)
{
	if(size==0)
			return NULL;
		if (!is_initialized)
		{
		uint32 required_size = size + sizeOfMetaData();
		uint32 da_start = (uint32)sbrk(required_size);

		//get new break since it's page aligned! thus, the size can be more than the required one
		uint32 da_break = (uint32)sbrk(0);//get the current break
		initialize_dynamic_allocator(da_start, da_break - da_start);
		}

		struct BlockMetaData *metaDataIt=LIST_FIRST(&list);
		//cprintf("calling allocate_block ff\n");
		while(metaDataIt!=NULL)
		{
	        if(metaDataIt->size>=(2*sizeOfMetaData()))
	        {
	      if(metaDataIt->is_free==1&&(metaDataIt->size -(2*sizeOfMetaData()))>= size)//may create new metaData with size 0
	     {
	    	 struct BlockMetaData* metaData;//new meta data
	    	 metaData = (struct BlockMetaData*)((char*)metaDataIt + size + sizeOfMetaData());//the address of the new meta data
	    	 metaData->is_free=1;
	    	 metaData->size=(metaDataIt->size)-(size+sizeOfMetaData());
	    	 metaDataIt->size= size + sizeOfMetaData();
	    	 LIST_INSERT_AFTER(&list,metaDataIt,metaData);//what if we inserted metedata over metadata
	    	 metaDataIt->is_free=0;
		    // cprintf("first\n");
	    	 last=metaData;
	    	 return (void*)((char*)metaDataIt +sizeOfMetaData());
	     }
	      else if(metaDataIt->is_free==1&&(metaDataIt->size-sizeOfMetaData())>=size){//may waste a small space (sizeOfMetaData())it will be not used
	    	  metaDataIt->is_free=0;
	    	 // cprintf("second\n");
	    	  return (void*)((char*)metaDataIt + sizeOfMetaData());
	      }
	        }
	      else if(metaDataIt->is_free==1&&(metaDataIt->size -(1*sizeOfMetaData()))>= size){
	    	  metaDataIt->is_free=0;
	    	  return (void*)((char*)metaDataIt + sizeOfMetaData());
	      }
	     metaDataIt= metaDataIt->prev_next_info.le_next;
		}

		struct BlockMetaData* metaData=sbrk(size+sizeOfMetaData());
		if(metaData==(void*)-1){
			//cprintf("sbrk has been called : 1\n");
				return NULL;
		}

		else{

			  // cprintf("sbrk has been called : 2\n");

			    metaData->is_free=1;
			    metaData->size=(uint32)(sbrk(0)-(void*)metaData);
			    LIST_INSERT_TAIL(&list,metaData);
			    struct BlockMetaData *metaDataIt=LIST_FIRST(&list);
			    	while(metaDataIt!=NULL)
			    	{
			            if(metaDataIt->size>=(2*sizeOfMetaData()))
			            {
			          if(metaDataIt->is_free==1&&(metaDataIt->size -(2*sizeOfMetaData()))>= size)//may create new metaData with size 0
			         {
			        	 struct BlockMetaData* metaData;//new meta data
			        	 metaData = (struct BlockMetaData*)((char*)metaDataIt + size + sizeOfMetaData());//the address of the new meta data
			        	 metaData->is_free=1;
			        	 metaData->size=(metaDataIt->size)-(size+sizeOfMetaData());
			        	 metaDataIt->size= size + sizeOfMetaData();
			        	 LIST_INSERT_AFTER(&list,metaDataIt,metaData);//what if we inserted metedata over metadata
			        	 metaDataIt->is_free=0;
			    	    // cprintf("first\n");
			        	 last=metaData;
			        	 return (void*)((char*)metaDataIt +sizeOfMetaData());
			         }
			          else if(metaDataIt->is_free==1&&(metaDataIt->size-sizeOfMetaData())>=size){//may waste a small space (sizeOfMetaData())it will be not used
			        	  metaDataIt->is_free=0;
			        	 // cprintf("second\n");
			        	  return (void*)((char*)metaDataIt + sizeOfMetaData());
			          }
			            }
			          else if(metaDataIt->is_free==1&&(metaDataIt->size -(1*sizeOfMetaData()))>= size){
			        	  metaDataIt->is_free=0;
			        	  return (void*)((char*)metaDataIt + sizeOfMetaData());
			          }
			         metaDataIt= metaDataIt->prev_next_info.le_next;
			    	}

			   // return (struct BlockMetaData*)((char*)metaData + sizeOfMetaData());

		 }

	return NULL;

}
//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	     if(size==0)
			return NULL;
		struct BlockMetaData *metaDataIt=LIST_FIRST(&list);
		struct BlockMetaData *flag=LIST_FIRST(&list);
				int num=0;
				LIST_FOREACH (metaDataIt,&list)
			    {
					  if(metaDataIt->size > flag->size)
					  {
						  flag=metaDataIt;
					  }
			    }
				metaDataIt=LIST_FIRST(&list);
				LIST_FOREACH (metaDataIt,&list)
				{

			     if(metaDataIt->is_free==1 && (metaDataIt->size -(sizeOfMetaData()))>= size)//may create new metaData with size 0
			     {
			    	  if(metaDataIt->size <= flag->size)
			    	  {  flag=metaDataIt;
			    	     num=1;
			    	    // cprintf("%d\n",2);
			          }

			     }
				}
				//end of the loop
				if(!num){
					if((int)sbrk(size+sizeOfMetaData())==-1){

									return NULL;
					}
                    else{

		         struct BlockMetaData* metaData;
	              metaData=sbrk(size+sizeOfMetaData());
		          metaData->is_free=0;
		          metaData->size=size+sizeOfMetaData();
                  LIST_INSERT_TAIL(&list,metaData);
		      return (struct BlockMetaData*)((char*)metaData + sizeOfMetaData());

                    }
				}
				 if(flag->size>=(2*sizeOfMetaData()))
				    {
				      if((flag->size -(2*sizeOfMetaData()))>= size)//may create new metaData with size 0
				     {
				    	 struct BlockMetaData* metaData;//new meta data
				    	 metaData = (struct BlockMetaData*)((char*)flag + size + sizeOfMetaData());//the address of the new meta data
				    	 metaData->is_free=1;
				    	 metaData->size=(flag->size)-(size+sizeOfMetaData());
				    	 flag->size= size + sizeOfMetaData();
				    	 LIST_INSERT_AFTER(&list,flag,metaData);//what if we inserted metedata over metadata
				    	 flag->is_free=0;
					    // cprintf("first\n");
				    	 last=metaData;
				    	 return (struct BlockMetaData*)((char*)flag +sizeOfMetaData());
				     }

				   }
				 if((flag->size-sizeOfMetaData())>=size){//may waste a small space (sizeOfMetaData())it will be not used
						 flag->is_free=0;
						 // cprintf("second\n");
						 return (struct BlockMetaData*)((char*)flag + sizeOfMetaData());
				 }



	return NULL;
}

//=========================================
// [6] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

//===================================================
// [8] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//cprintf("calling free block\n");
	if(va==NULL)
		return;

		struct BlockMetaData *prev;
		struct BlockMetaData *next;
	    struct BlockMetaData *metaDataIt;
		metaDataIt=((struct BlockMetaData *)va - 1);
		prev=LIST_PREV(metaDataIt);
		next=LIST_NEXT(metaDataIt);

		if(LIST_FIRST(&list)==metaDataIt && next==NULL)//first block and only
		{
			metaDataIt->is_free=1;
		}

		else if(LIST_LAST(&list) ==metaDataIt)// last block
		{
			if(prev->is_free==0 )//not free
			{
			  metaDataIt->is_free=1;

			}
			else{
			  prev->size = prev->size + metaDataIt->size;
			  prev->is_free=1;
			  metaDataIt->size=0;
			  metaDataIt->is_free=0;
              LIST_REMOVE(&list,metaDataIt);
			}
		}
		else if(LIST_FIRST(&list)==metaDataIt)//first block
		{

			  if(next->is_free==0 )//next not free
			  {
			    metaDataIt->is_free=1;
			  }
		      else{
			    metaDataIt->size=metaDataIt->size+next->size;
			    metaDataIt->is_free=1;
			    next->size=0;
			    next->is_free=0;
                LIST_REMOVE(&list,next);
			  }
		}
			    //finish first and last
		else if(next->is_free==0&&prev->is_free==0)// both are busy
		{
			metaDataIt->is_free=1;
		}
		else if(next->is_free==0&&prev->is_free==1) //next full prev free
		{
			prev->size=prev->size + metaDataIt->size;
			metaDataIt->size=0;
			metaDataIt->is_free=0;
			LIST_REMOVE(&list,metaDataIt);
		}
		else if(next->is_free==1&&prev->is_free==0) //next free prev full i will edit here
		{
			metaDataIt->size = next->size + metaDataIt->size;
			next->size=0;
			next->is_free=0;
			metaDataIt->is_free=1;
			LIST_REMOVE(&list,next);
		}
		else {
			prev->size=prev->size + metaDataIt->size + next->size;
			next->size=0;
			metaDataIt->is_free=0;
			next->is_free=0;
			metaDataIt->size=0;
			prev->is_free=1;
	        LIST_REMOVE(&list,metaDataIt);
	        LIST_REMOVE(&list,next);
		}
			return ;
}

//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	if(va != NULL && new_size==0)
		{
			free_block(va);
			return NULL;
		}
    if(va==NULL)
		{
			if(new_size==0)
			{
				return NULL;
			}
			else
			{
			  return alloc_block_FF(new_size);
			}
		}

		struct BlockMetaData *metaDataIt;
		metaDataIt=((struct BlockMetaData *)va - 1);
		struct BlockMetaData *next=LIST_NEXT(metaDataIt);
		struct BlockMetaData *prev=LIST_PREV(metaDataIt);

		if(new_size > (metaDataIt->size- sizeOfMetaData()))//case 1 increment
		{
			if(next->is_free==1)//what if next is a NULL pointer (last block)
				{
					if(next->size > (new_size - (metaDataIt->size - sizeOfMetaData())))
					{
						struct BlockMetaData *metaData;
						metaData=	(struct BlockMetaData*)(va+new_size);
						metaData->is_free=1;
						next->is_free=0;
						next->size=0;
						metaData->size=next->size - (new_size - (metaDataIt->size - sizeOfMetaData()));
						LIST_REMOVE(&list,next);
						LIST_INSERT_AFTER(&list,metaDataIt,metaData);

						metaDataIt->size = metaDataIt->size +(new_size - (metaDataIt->size - sizeOfMetaData()));
						return va;
					}
					else if(next->size ==(new_size - (metaDataIt->size - sizeOfMetaData())))
					{
						metaDataIt->size = metaDataIt->size + next->size;
						metaDataIt->is_free=0;
						next->is_free=0;
						next->size=0;
						LIST_REMOVE(&list,next);
						return va;
					}
					else
					{
						void* address=alloc_block_FF(new_size);
						if(address!=NULL)
						{
							free_block(va);
							return address;
						}
						else
							{return NULL;}
					}
				}

			else if(next->is_free==0)
				{
				void* address=alloc_block_FF(new_size);
					if(address!=NULL)
					{
						free_block(va);
						return address;
					}
					else
						return NULL;
				}
		}
		else if((new_size == (metaDataIt->size- sizeOfMetaData())))//case 2 nothing
		{
			return va;
		}
		else // case 3 decrement
		{
			if(next->is_free==1)
			{
				next->size=next->size+(metaDataIt->size-sizeOfMetaData()-new_size);
				metaDataIt->size=new_size+sizeOfMetaData();
				next = (struct BlockMetaData*)((char*)next -(metaDataIt->size-sizeOfMetaData()-new_size) );
				return va;
			}
			else
			{
				uint32 diff_size = (metaDataIt->size-sizeOfMetaData()-new_size);
				metaDataIt->size=new_size+sizeOfMetaData();
				struct BlockMetaData *metaData;
				metaData=(struct BlockMetaData*)(va+new_size);
				metaData->size=diff_size;
			    metaData->is_free=1;

			    LIST_INSERT_AFTER(&list,metaDataIt,metaData);

				return va;

			}
		}
	return NULL;
}
