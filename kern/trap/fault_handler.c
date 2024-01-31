/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"
//#include "kheap.h"
//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault

void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
#if USE_KHEAP
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
#else
		int iWS =curenv->page_last_WS_index;
		uint32 wsSize = env_page_ws_get_size(curenv);
#endif
	//	env_page_ws_get_size(curenv);
 if(isPageReplacmentAlgorithmFIFO()){

	 // fault_va=ROUNDDOWN( fault_va,PAGE_SIZE);
	 if(wsSize < (curenv->page_WS_max_size))
	 	{

	 		//cprintf("placement fifo :: %x\n",fault_va);
	 		struct FrameInfo *ptr;
	 		int r= allocate_frame(&ptr);
	 		if(r==E_NO_MEM)
	 		 return;
	 		map_frame(curenv->env_page_directory,ptr,fault_va,PERM_PRESENT|PERM_USER|PERM_WRITEABLE);
	 		struct WorkingSetElement*newE = env_page_ws_list_create_element(curenv,fault_va);
            if(pf_read_env_page(curenv,(void*) fault_va)!=E_PAGE_NOT_EXIST_IN_PF){
            	//cprintf("\nthe fualted page is found on the disk \n");
            	//uint32 *page_table;
            	/*if(get_page_table(curenv->env_page_directory,fault_va,&page_table)==TABLE_NOT_EXIST){
				// cprintf("\nfrom tia the page table is not found we will create it \n");
				 create_page_table(curenv->env_page_directory,curenv->segmentBreak);
							}*/
            	//	cprintf("\n the size before is %d\n",LIST_SIZE(&(curenv->page_WS_list)));
            	LIST_INSERT_TAIL(&curenv->page_WS_list,newE);
            	curenv->page_last_WS_element=NULL;
            	curenv->page_last_WS_index ++ ;
            	curenv->page_last_WS_index = curenv->page_last_WS_index %  curenv->page_WS_max_size ;

            	wsSize++;
            	if(wsSize >= (curenv->page_WS_max_size)){
            	  curenv->page_last_WS_element=LIST_FIRST(&curenv->page_WS_list);
            	}
            	//	cprintf("\n the size after is %d\n",LIST_SIZE(&(curenv->page_WS_list)));
            }
            else if(fault_va < USTACKTOP && fault_va >= USTACKBOTTOM){
            	//cprintf("\nthe fualted page is not on the disk but found in the stack\n");
            	//uint32 *page_table;
				/*if(get_page_table(curenv->env_page_directory,fault_va,&page_table)==TABLE_NOT_EXIST){
				// cprintf("\nfrom tia the page table is not found we will create it \n");
				 create_page_table(curenv->env_page_directory,curenv->segmentBreak);
							}*/
            	//	cprintf("\n the size before is %d\n",LIST_SIZE(&(curenv->page_WS_list)));


            	LIST_INSERT_TAIL(&curenv->page_WS_list,newE);
            	curenv->page_last_WS_element=NULL;
            	curenv->page_last_WS_index ++ ;
            	curenv->page_last_WS_index = curenv->page_last_WS_index %  curenv->page_WS_max_size ;

            	wsSize++;
            	if(wsSize >= (curenv->page_WS_max_size)){
            	curenv->page_last_WS_element=LIST_FIRST(&curenv->page_WS_list);
            	}
            	//	cprintf("\n the size after is %d\n",LIST_SIZE(&(curenv->page_WS_list)));
            }
            else if(fault_va >=USER_HEAP_START&&fault_va<USER_HEAP_MAX){
            	//cprintf("\nthe fualted page is not on the disk but found in the heap\n");
            	//uint32 *page_table;
				/*if(get_page_table(curenv->env_page_directory,fault_va,&page_table)==TABLE_NOT_EXIST){
				// cprintf("\nfrom tia the page table is not found we will create it \n");
				 create_page_table(curenv->env_page_directory,curenv->segmentBreak);
							}*/
            	//	cprintf("\n the size before is %d\n",LIST_SIZE(&(curenv->page_WS_list)));
            	LIST_INSERT_TAIL(&curenv->page_WS_list,newE);
            	curenv->page_last_WS_element=NULL;
            	curenv->page_last_WS_index ++ ;
            	curenv->page_last_WS_index = curenv->page_last_WS_index %  curenv->page_WS_max_size ;
            	wsSize++;
            	if(wsSize >= (curenv->page_WS_max_size)){
            	  curenv->page_last_WS_element=LIST_FIRST(&curenv->page_WS_list);
            	}
            	//	cprintf("\n the size after is %d\n",LIST_SIZE(&(curenv->page_WS_list)));
            }
            else{
            //	cprintf("\nthe fualted page is not on the disk and not on the stack or heap must be killed %x \n",fault_va);
            	sched_kill_env(curenv->env_id);
            }
            // env_page_ws_print(curenv);

	}
	else //replacement replacement replacement replacement replacement replacement replacement replacement replacement replacement replacement
	{
		//cprintf("the fualted address is : %x\n",fault_va);
		struct FrameInfo *ptr;
		int r= allocate_frame(&ptr);
		    if(r==E_NO_MEM)
				return;
		map_frame(curenv->env_page_directory,ptr,fault_va,PERM_PRESENT|PERM_USER|PERM_WRITEABLE);
		struct WorkingSetElement*newE = env_page_ws_list_create_element(curenv,fault_va);
		struct WorkingSetElement*lastOne=LIST_NEXT(curenv->page_last_WS_element);
		LIST_INSERT_AFTER(&curenv->page_WS_list,curenv->page_last_WS_element,newE);

		if(pf_read_env_page(curenv,(void*) fault_va)!=E_PAGE_NOT_EXIST_IN_PF)
		{
			uint32 *page_table;
			victimWSElement=curenv->page_last_WS_element;
			unsigned int victimVirtualAddress=victimWSElement->virtual_address;
			struct  FrameInfo *p= get_frame_info(curenv->env_page_directory,victimVirtualAddress,&page_table);
			int modifiedBit = (page_table[PTX(victimVirtualAddress)] >> 6) & 1;
			//cprintf("modified bit is %d\n",modifiedBit);
			if(modifiedBit==1){
			 pf_update_env_page(curenv,victimVirtualAddress,p);
			}
			unmap_frame(curenv->env_page_directory,victimVirtualAddress);
			LIST_REMOVE(&(curenv->page_WS_list),victimWSElement);
			env_page_ws_list_delete_element(victimWSElement);
			curenv->page_last_WS_element=lastOne;
			if(lastOne==NULL){
			 curenv->page_last_WS_element=LIST_FIRST(&curenv->page_WS_list);
			}

		}
		else{
		 if (fault_va < USTACKTOP && fault_va >= USTACKBOTTOM)
		 {
			 //cprintf("replacement nstack page\n");


			 uint32 *page_table;
			 victimWSElement=curenv->page_last_WS_element;
			 unsigned int victimVirtualAddress=victimWSElement->virtual_address;
			 struct  FrameInfo *p= get_frame_info(curenv->env_page_directory,victimVirtualAddress,&page_table);
			 int modifiedBit = (page_table[PTX(victimVirtualAddress)] >> 6) & 1;
			 //cprintf("modified bit is %d\n",modifiedBit);
			 if(modifiedBit==1){
			 	 pf_update_env_page(curenv,victimVirtualAddress,p);
			 }
			 unmap_frame(curenv->env_page_directory,victimVirtualAddress);
			 LIST_REMOVE(&(curenv->page_WS_list),victimWSElement);
			 env_page_ws_list_delete_element(victimWSElement);
			 curenv->page_last_WS_element=lastOne;
			 if(lastOne==NULL){
			 	 curenv->page_last_WS_element=LIST_FIRST(&curenv->page_WS_list);
			 }
			//get_page_table(curenv->env_page_directory,fault_va,&page_table);
			//page_table[PTX(fault_va)] |= (1 << 6);

		 }
		 else if(fault_va >=USER_HEAP_START&&fault_va<USER_HEAP_MAX)
		 {
			uint32 *page_table;
			victimWSElement=curenv->page_last_WS_element;
			unsigned int victimVirtualAddress=victimWSElement->virtual_address;
			struct  FrameInfo *p= get_frame_info(curenv->env_page_directory,victimVirtualAddress,&page_table);
			 //get_page_table(curenv->env_page_directory,victimVA,&page_table);
			int modifiedBit = (page_table[PTX(victimVirtualAddress)] >> 6) & 1;
			if(modifiedBit==1){
			 	 pf_update_env_page(curenv,victimVirtualAddress,p);
			}
			unmap_frame(curenv->env_page_directory,victimVirtualAddress);
		 	curenv->page_last_WS_element=lastOne;
			LIST_REMOVE(&(curenv->page_WS_list),victimWSElement);
			env_page_ws_list_delete_element(victimWSElement);
			 if(lastOne==NULL){
			   curenv->page_last_WS_element=LIST_FIRST(&curenv->page_WS_list);
			 }
			//get_page_table(curenv->env_page_directory,fault_va,&page_table);
			//page_table[PTX(fault_va)] |= (1 << 6);
			}
		 else{
		  //cprintf("\nthe fualted page is not on the disk and not on the stack or heap must be killed %x \n",fault_va);
		 sched_kill_env(curenv->env_id);
		 }
		}
		//env_page_ws_print(curenv);

	}
 }
	else if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX))
		{
			//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
			// Write your code here, remove the panic and write your code
		uint32 fisrtListSize = LIST_SIZE(&(curenv->ActiveList));
		uint32 secondListSize = LIST_SIZE(&(curenv->SecondList));
        if((fisrtListSize + secondListSize )<(curenv->page_WS_max_size) )//placement
        {
        	//cprintf("\n\ntake care LRU placement the fualted address is %x\n\n",fault_va);
        	bool isFound=0;
        	struct WorkingSetElement * it = NULL;
        	LIST_FOREACH(it, &(curenv->SecondList))
        	{
              if(ROUNDDOWN(it->virtual_address,PAGE_SIZE)==ROUNDDOWN(fault_va,PAGE_SIZE)){
            	  isFound=1;
            	 if(LIST_SIZE(&(curenv->ActiveList)) < curenv->ActiveListSize)//first list is not full yet we will insert inside it
            	 {
            		uint32 *page_table;
            		get_page_table(curenv->env_page_directory,fault_va,&page_table);
            		page_table[PTX(it->virtual_address)] |= 1;
            	    LIST_INSERT_HEAD(&curenv->ActiveList,it);
            	    LIST_REMOVE(&(curenv->SecondList),it);
            	 }
            	 else //fisrt list is full
            	 {
            		 struct WorkingSetElement*elementOfFisrtList=LIST_LAST(&curenv->ActiveList);
            		 struct WorkingSetElement*elementOfSecondList=it;
            		 uint32 *page_table;
            		 get_page_table(curenv->env_page_directory,elementOfFisrtList->virtual_address,&page_table);
            		 page_table[PTX(elementOfFisrtList->virtual_address)] &= ~(1);
            		 LIST_REMOVE(&(curenv->ActiveList),elementOfFisrtList);
            		 get_page_table(curenv->env_page_directory,fault_va,&page_table);
            		 page_table[PTX(it->virtual_address)] |= 1;
            		 LIST_REMOVE(&(curenv->SecondList),it);
            		 LIST_INSERT_HEAD(&curenv->ActiveList,it);
            		 LIST_INSERT_HEAD(&curenv->SecondList,elementOfFisrtList);
            	 }
            	 break;
              }
        	}
        	if(!isFound){
        		struct FrameInfo *ptr;
        		int r= allocate_frame(&ptr);
        		if(r==E_NO_MEM)
        		    return;
        		 map_frame(curenv->env_page_directory,ptr,fault_va,PERM_PRESENT|PERM_USER|PERM_WRITEABLE);
        		 struct WorkingSetElement*newE = env_page_ws_list_create_element(curenv,fault_va);
        	  if(pf_read_env_page(curenv,(void*) fault_va)!=E_PAGE_NOT_EXIST_IN_PF)
        	  {
                if(LIST_SIZE(&(curenv->ActiveList)) < curenv->ActiveListSize)//first list is not full yet we will insert inside it
                 {
                  LIST_INSERT_HEAD(&curenv->ActiveList,newE);
                 }
                else //first list is full
                {
                struct WorkingSetElement*elementOfFisrtList=LIST_LAST(&curenv->ActiveList);
                struct WorkingSetElement*elementOfSecondList=it;
                uint32 *page_table;
                get_page_table(curenv->env_page_directory,elementOfFisrtList->virtual_address,&page_table);
                page_table[PTX(elementOfFisrtList->virtual_address)] &= ~(1);
                LIST_REMOVE(&(curenv->ActiveList),elementOfFisrtList);
                LIST_INSERT_HEAD(&curenv->SecondList,elementOfFisrtList);
               // map_frame(curenv->env_page_directory,ptr,fault_va,PERM_PRESENT|PERM_USER|PERM_WRITEABLE);
                LIST_INSERT_HEAD(&curenv->ActiveList,newE);
                }
        	  }
        	  else if(fault_va < USTACKTOP && fault_va >= USTACKBOTTOM){
        		  if(LIST_SIZE(&(curenv->ActiveList)) < curenv->ActiveListSize)//first list is not full yet we will insert inside it
        		   {
        		     //  map_frame(curenv->env_page_directory,ptr,fault_va,PERM_PRESENT|PERM_USER|PERM_WRITEABLE);
        		       LIST_INSERT_HEAD(&curenv->ActiveList,newE);
        		   }
        		  else //first list is full
        		  {
        		     struct WorkingSetElement*elementOfFisrtList=LIST_LAST(&curenv->ActiveList);
        		     struct WorkingSetElement*elementOfSecondList=it;
        		     uint32 *page_table;
        		     get_page_table(curenv->env_page_directory,elementOfFisrtList->virtual_address,&page_table);
        		     page_table[PTX(elementOfFisrtList->virtual_address)] &= ~(1);
        		     LIST_REMOVE(&(curenv->ActiveList),elementOfFisrtList);
        		     LIST_INSERT_HEAD(&curenv->SecondList,elementOfFisrtList);
        		     // map_frame(curenv->env_page_directory,ptr,fault_va,PERM_PRESENT|PERM_USER|PERM_WRITEABLE);
        		     LIST_INSERT_HEAD(&curenv->ActiveList,newE);
        		  }
        	  }
        	  else if(fault_va >=USER_HEAP_START&&fault_va<USER_HEAP_MAX){

        		  if(LIST_SIZE(&(curenv->ActiveList)) < curenv->ActiveListSize)//first list is not full yet we will insert inside it
        		  {
        		     //  map_frame(curenv->env_page_directory,ptr,fault_va,PERM_PRESENT|PERM_USER|PERM_WRITEABLE);
        		     LIST_INSERT_HEAD(&curenv->ActiveList,newE);
        		  }
        		  else //first list is full
        		  {
        		     struct WorkingSetElement*elementOfFisrtList=LIST_LAST(&curenv->ActiveList);
        		     struct WorkingSetElement*elementOfSecondList=it;
        		     uint32 *page_table;
        		     get_page_table(curenv->env_page_directory,elementOfFisrtList->virtual_address,&page_table);
        		     page_table[PTX(elementOfFisrtList->virtual_address)] &= ~(1);
        		     LIST_REMOVE(&(curenv->ActiveList),elementOfFisrtList);
        		     LIST_INSERT_HEAD(&curenv->SecondList,elementOfFisrtList);
        		     // map_frame(curenv->env_page_directory,ptr,fault_va,PERM_PRESENT|PERM_USER|PERM_WRITEABLE);
        		     LIST_INSERT_HEAD(&curenv->ActiveList,newE);
        		  }

        	  }
        	  else{
        		  sched_kill_env(curenv->env_id);
        	  }

        	}

        }
        else //replacementLRU replacementLRU replacementLRU replacementLRU replacementLRU replacementLRU replacementLRU replacementLRU
        {
        	//cprintf("LRU replacement the fualted address is %x\n\n",fault_va);
        	//cprintf("the lists BEFORE\n");
        	//env_page_ws_print(curenv);
        	bool isFound=0;
        	struct WorkingSetElement * it = NULL;
        	LIST_FOREACH(it, &(curenv->SecondList))
        	{
        	  if(ROUNDDOWN(it->virtual_address,PAGE_SIZE)==ROUNDDOWN(fault_va,PAGE_SIZE)){
        		//cprintf("case1 \n");
        	  isFound=1;
        	  struct WorkingSetElement*elementOfFisrtList=LIST_LAST(&curenv->ActiveList);
        	  struct WorkingSetElement*elementOfSecondList=it;
        	  uint32 *page_table;
        	  get_page_table(curenv->env_page_directory,elementOfFisrtList->virtual_address,&page_table);
        	  page_table[PTX(elementOfFisrtList->virtual_address)] &= ~(1);
        	  LIST_REMOVE(&(curenv->SecondList),it);
        	  LIST_REMOVE(&(curenv->ActiveList),elementOfFisrtList);
        	  LIST_INSERT_HEAD(&curenv->SecondList,elementOfFisrtList);

        	  get_page_table(curenv->env_page_directory,fault_va,&page_table);
        	  page_table[PTX(it->virtual_address)] |= 1;
        	  LIST_INSERT_HEAD(&curenv->ActiveList,it);

        	  break;
        	  }
        	}
        	if(!isFound){
         //   cprintf("case2 \n");
            struct FrameInfo *ptr;
            int r= allocate_frame(&ptr);
             if(r==E_NO_MEM)
                return;
            map_frame(curenv->env_page_directory,ptr,fault_va,PERM_PRESENT|PERM_USER|PERM_WRITEABLE);
            if(pf_read_env_page(curenv,(void*) fault_va)!=E_PAGE_NOT_EXIST_IN_PF)
            {
            uint32 *page_table;
            struct WorkingSetElement*theVictim=LIST_LAST(&curenv->SecondList);

            struct  FrameInfo *p= get_frame_info(curenv->env_page_directory,theVictim->virtual_address,&page_table);
            int modifiedBit = (page_table[PTX(theVictim->virtual_address)] >> 6) & 1;

           	if(modifiedBit==1){
             pf_update_env_page(curenv,theVictim->virtual_address,p);
           	}
        	unmap_frame(curenv->env_page_directory, theVictim->virtual_address);
           	LIST_REMOVE(&(curenv->SecondList), theVictim);
        	env_page_ws_list_delete_element(theVictim);
        	struct WorkingSetElement*elementOfFisrtList=LIST_LAST(&curenv->ActiveList);
        	LIST_REMOVE(&(curenv->ActiveList),elementOfFisrtList);
           	struct WorkingSetElement*newE = env_page_ws_list_create_element(curenv,fault_va);
           	get_page_table(curenv->env_page_directory,elementOfFisrtList->virtual_address,&page_table);
            page_table[PTX(elementOfFisrtList->virtual_address)] &= ~(1);
        	LIST_INSERT_HEAD(&curenv->SecondList,elementOfFisrtList);
           	LIST_INSERT_HEAD(&curenv->ActiveList,newE);

        	}
            else{
            	if(fault_va < USTACKTOP && fault_va >= USTACKBOTTOM){

            		uint32 *page_table;
            		struct WorkingSetElement*theVictim=LIST_LAST(&curenv->SecondList);
            		struct  FrameInfo *p= get_frame_info(curenv->env_page_directory,theVictim->virtual_address,&page_table);
            		int modifiedBit = (page_table[PTX(theVictim->virtual_address)] >> 6) & 1;
            		if(modifiedBit==1){
            		   pf_update_env_page(curenv,theVictim->virtual_address,p);
            		}
            		unmap_frame(curenv->env_page_directory, theVictim->virtual_address);
            		LIST_REMOVE(&(curenv->SecondList), theVictim);

            		env_page_ws_list_delete_element(theVictim);
            		struct WorkingSetElement*elementOfFisrtList=LIST_LAST(&curenv->ActiveList);
            		LIST_REMOVE(&(curenv->ActiveList),elementOfFisrtList);
            		struct WorkingSetElement*newE = env_page_ws_list_create_element(curenv,fault_va);
            		get_page_table(curenv->env_page_directory,elementOfFisrtList->virtual_address,&page_table);
            		page_table[PTX(elementOfFisrtList->virtual_address)] &= ~(1);
            		LIST_INSERT_HEAD(&curenv->SecondList,elementOfFisrtList);
            		LIST_INSERT_HEAD(&curenv->ActiveList,newE);

            	//	env_page_ws_invalidate(curenv,theVictim->virtual_address);

            	}
            	else if(fault_va >=USER_HEAP_START&&fault_va<USER_HEAP_MAX){
            		uint32 *page_table;
            		struct WorkingSetElement*theVictim=LIST_LAST(&curenv->SecondList);

            		struct  FrameInfo *p= get_frame_info(curenv->env_page_directory,theVictim->virtual_address,&page_table);
            		int modifiedBit = (page_table[PTX(theVictim->virtual_address)] >> 6) & 1;
            		if(modifiedBit==1){
            		  pf_update_env_page(curenv,theVictim->virtual_address,p);
            		}
            		unmap_frame(curenv->env_page_directory, theVictim->virtual_address);
            		LIST_REMOVE(&(curenv->SecondList), theVictim);

            		env_page_ws_list_delete_element(theVictim);
            		struct WorkingSetElement*elementOfFisrtList=LIST_LAST(&curenv->ActiveList);
            		LIST_REMOVE(&(curenv->ActiveList),elementOfFisrtList);
					struct WorkingSetElement*newE = env_page_ws_list_create_element(curenv,fault_va);
					get_page_table(curenv->env_page_directory,elementOfFisrtList->virtual_address,&page_table);
					page_table[PTX(elementOfFisrtList->virtual_address)] &= ~(1);
					LIST_INSERT_HEAD(&curenv->SecondList,elementOfFisrtList);
				    LIST_INSERT_HEAD(&curenv->ActiveList,newE);

            	}
            	else{
            	//	cprintf("kill him nowwwww\n");
            		sched_kill_env(curenv->env_id);
            	}
            }
            //cprintf("\nthe lists after\n");
           // env_page_ws_print(curenv);
        	}
        }

     //   cprintf("222222222222222222222222222222222222222222222\n");	//TODO: [PROJECT'23.MS3 - BONUS] [1] PAGE FAULT HANDLER - O(1) implementation of LRU replacement
		}

}
void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}



