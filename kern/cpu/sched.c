#include "sched.h"

#include <inc/assert.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/trap.h>
#include <kern/mem/kheap.h>
#include <kern/mem/memory_manager.h>
#include <kern/tests/utilities.h>
#include <kern/cmd/command_prompt.h>


uint32 isSchedMethodRR(){if(scheduler_method == SCH_RR) return 1; return 0;}
uint32 isSchedMethodMLFQ(){if(scheduler_method == SCH_MLFQ) return 1; return 0;}
uint32 isSchedMethodBSD(){if(scheduler_method == SCH_BSD) return 1; return 0;}

//===================================================================================//
//============================ SCHEDULER FUNCTIONS ==================================//
//===================================================================================//

//===================================
// [1] Default Scheduler Initializer:
//===================================
void sched_init()
{
	old_pf_counter = 0;

	sched_init_RR(INIT_QUANTUM_IN_MS);

	init_queue(&env_new_queue);
	init_queue(&env_exit_queue);
	scheduler_status = SCH_STOPPED;
}

//=========================
// [2] Main FOS Scheduler:
//=========================
void
fos_scheduler(void)
{
	//	cprintf("inside scheduler\n");

	chk1();
	scheduler_status = SCH_STARTED;

	//This variable should be set to the next environment to be run (if any)
	struct Env* next_env = NULL;

	if (scheduler_method == SCH_RR)
	{
		// Implement simple round-robin scheduling.
		// Pick next environment from the ready queue,
		// and switch to such environment if found.
		// It's OK to choose the previously running env if no other env
		// is runnable.

		//If the curenv is still exist, then insert it again in the ready queue
		if (curenv != NULL)
		{
			enqueue(&(env_ready_queues[0]), curenv);
		}

		//Pick the next environment from the ready queue
		next_env = dequeue(&(env_ready_queues[0]));

		//Reset the quantum
		//2017: Reset the value of CNT0 for the next clock interval
		kclock_set_quantum(quantums[0]);
		//uint16 cnt0 = kclock_read_cnt0_latch() ;
		//cprintf("CLOCK INTERRUPT AFTER RESET: Counter0 Value = %d\n", cnt0 );

	}
	else if (scheduler_method == SCH_MLFQ)
	{
		next_env = fos_scheduler_MLFQ();
	}
	else if (scheduler_method == SCH_BSD)
	{
		next_env = fos_scheduler_BSD();
	}
	//temporarily set the curenv by the next env JUST for checking the scheduler
	//Then: reset it again
	struct Env* old_curenv = curenv;
	curenv = next_env ;
	chk2(next_env) ;
	curenv = old_curenv;

	//sched_print_all();

	if(next_env != NULL)
	{
		//		cprintf("\nScheduler select program '%s' [%d]... counter = %d\n", next_env->prog_name, next_env->env_id, kclock_read_cnt0());
		//		cprintf("Q0 = %d, Q1 = %d, Q2 = %d, Q3 = %d\n", queue_size(&(env_ready_queues[0])), queue_size(&(env_ready_queues[1])), queue_size(&(env_ready_queues[2])), queue_size(&(env_ready_queues[3])));
		env_run(next_env);
	}
	else
	{
		/*2015*///No more envs... curenv doesn't exist any more! return back to command prompt
		curenv = NULL;
		//lcr3(K_PHYSICAL_ADDRESS(ptr_page_directory));
		lcr3(phys_page_directory);

		//cprintf("SP = %x\n", read_esp());

		scheduler_status = SCH_STOPPED;
		//cprintf("[sched] no envs - nothing more to do!\n");
		while (1)
			run_command_prompt(NULL);

	}
}

//=============================
// [3] Initialize RR Scheduler:
//=============================
void sched_init_RR(uint8 quantum)
{
	//cprintf("\nbe carefull RR initialization has been called\n");
	// Create 1 ready queue for the RR
	num_of_ready_queues = 1;
#if USE_KHEAP
	sched_delete_ready_queues();
	env_ready_queues = kmalloc(sizeof(struct Env_Queue));
	quantums = kmalloc(num_of_ready_queues * sizeof(uint8)) ;
#endif
	quantums[0] = quantum;
	kclock_set_quantum(quantums[0]);
	init_queue(&(env_ready_queues[0]));

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_RR;
	//=========================================
	//=========================================
}

//===============================
// [4] Initialize MLFQ Scheduler:
//===============================
void sched_init_MLFQ(uint8 numOfLevels, uint8 *quantumOfEachLevel)
{
#if USE_KHEAP
	//=========================================
	//DON'T CHANGE THESE LINES=================
	sched_delete_ready_queues();
	//=========================================
	//=========================================

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_MLFQ;
	//=========================================
	//=========================================
#endif
}

//===============================
// [5] Initialize BSD Scheduler:
//===============================
void sched_init_BSD(uint8 numOfLevels, uint8 quantum)
{
#if USE_KHEAP
	//TODO: [PROJECT'23.MS3 - #4] [2] BSD SCHEDULER - sched_init_BSD
	//Your code is here
	//Comment the following line
	loadAVG=fix_int(0);
	//curenv->nice=0;                    //edited by me
	//curenv->recentCPU=fix_int(0);      //edited by me
	//curenv->priority=PRI_MAX;          //edited by me
	num_of_ready_queues = numOfLevels;
	sched_delete_ready_queues();
	env_ready_queues = kmalloc(num_of_ready_queues *sizeof(struct Env_Queue));
	quantums = kmalloc(num_of_ready_queues *sizeof(uint8)) ;
/*	for(int i = 0; i < numOfLevels; i++){
		quantums[i]=quantum;
	}*/

  //  cprintf("\nbe carefull BSD initialization has been called\n");
	for(int i = 0; i <num_of_ready_queues ; i++){
		quantums[i]=quantum;
		init_queue(&(env_ready_queues[i]));

	}
	kclock_set_quantum(quantums[0]);
	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_BSD;
	//=========================================
	//=========================================
#endif
}


//=========================
// [6] MLFQ Scheduler:
//=========================
struct Env* fos_scheduler_MLFQ()
{
	panic("not implemented");
	return NULL;
}

//=========================
// [7] BSD Scheduler:
//=========================
struct Env* fos_scheduler_BSD()
{
	//TODO: [PROJECT'23.MS3 - #5] [2] BSD SCHEDULER - fos_scheduler_BSD
	//Your code is here
	//Comment the following line
	if (curenv != NULL)
	{
		int queueIndex=PRI_MAX-curenv->priority;
		enqueue(&(env_ready_queues[queueIndex]), curenv);
	}
	for(int i = 0; i <num_of_ready_queues ; i++){
		if(LIST_SIZE(&(env_ready_queues[i]))!=0){
			kclock_set_quantum(quantums[0]);
			return dequeue(&(env_ready_queues[i]));
		}
	}
	loadAVG=fix_int(0);
	return NULL;
}

//========================================
// [8] Clock Interrupt Handler
//	  (Automatically Called Every Quantum)
//========================================
int curruntTime=0;
bool flag =1;
void clock_interrupt_handler()
{
	//TODO: [PROJECT'23.MS3 - #5] [2] BSD SCHEDULER - Your code is here
	if(scheduler_method == SCH_BSD)
	{

	 // cprintf("\nThe currunt time is %d\n",curruntTime);
	  int numOfReadyProcess=0;
	  if(curenv != NULL)
	  {
	  numOfReadyProcess=1;
	  fixed_point_t num = fix_int(1);
	  curenv->recentCPU =fix_add (curenv->recentCPU, num);
	  }
	  curruntTime+=quantums[0];
      if(curruntTime>= 1000|| flag )
      {
    	  flag =0;
    	  //cprintf("\nthe time is %d\n",quantums[0] * timer_ticks());
    	  for(int i = 0; i < num_of_ready_queues; i++){
    		  int sizeOfQueue=LIST_SIZE(&(env_ready_queues[i]));
    	  		if(sizeOfQueue!=0){
    	  			numOfReadyProcess+=sizeOfQueue;
    	  		}
    	  }
    	 // cprintf("\nnumber of ready processes %d\n",numOfReadyProcess);
    	  //load AVG equation
    	  fixed_point_t fixedNumOfReadyProcess=fix_int(numOfReadyProcess);
    	  fixed_point_t frac1=fix_frac(59,60);
    	  fixed_point_t frac2=fix_frac(1,60);
    	  fixed_point_t temp1=fix_mul (frac1, loadAVG);
    	  fixed_point_t temp2=fix_mul (frac2, fixedNumOfReadyProcess);
    	  loadAVG=fix_add(temp1, temp2);
    	 // cprintf("\nload averege %u\n",loadAVG);
    	  for(int i = 0; i <num_of_ready_queues ; i++){
	    	if (!LIST_EMPTY(&(env_ready_queues[i])))
			{
	    		struct Env* ptr_env ;
				LIST_FOREACH(ptr_env, &(env_ready_queues[i]))
				{
				   //recent CPU equation  //recent CPU equation //recent CPU equation
				   fixed_point_t num1=fix_int(1);
				   fixed_point_t num2=fix_int(2);
				   fixed_point_t temp3=fix_mul (num2,loadAVG);
				   fixed_point_t temp4=fix_add(temp3, num1);
				   fixed_point_t frac= fix_div (temp3, temp4);
				   fixed_point_t fixed_nice=fix_int(ptr_env->nice);
				   fixed_point_t temp5=fix_mul(frac,ptr_env->recentCPU);
				   ptr_env->recentCPU=fix_add(temp5,fixed_nice);
				  // cprintf("\nrecent cpu %u\n",ptr_env->recentCPU);
				}
			}

	      }
	      if(curenv!=NULL){
	    	  //recent CPU equation  //recent CPU equation //recent CPU equation
	    	   fixed_point_t num1=fix_int(1);
			   fixed_point_t num2=fix_int(2);
			   fixed_point_t temp3=fix_mul (num2, loadAVG);
			   fixed_point_t temp4=fix_add(temp3, num1);
			   fixed_point_t frac= fix_div (temp3, temp4);
			   fixed_point_t fixed_nice=fix_int(curenv->nice);
			   fixed_point_t temp5=fix_mul(frac,curenv->recentCPU);
			   curenv->recentCPU=fix_add(temp5,fixed_nice);
	      }
      }
      if(timer_ticks() % 4 == 0)
      {
    	  //lets update the priority for all ready processes
    	  //sched_print_all();
    	  for(int i = 0; i <num_of_ready_queues ; i++)
    	  {
			if (!LIST_EMPTY(&(env_ready_queues[i])))
			{
				struct Env* ptr_env ;
				struct Env* movedEnv;
				LIST_FOREACH(ptr_env, &(env_ready_queues[i]))
				{
				   //priority equation  //priority equation //priority equation
				   int oldPriority=ptr_env->priority;
				   fixed_point_t FIXED_PRI_MAX=fix_int(PRI_MAX);
				   fixed_point_t fixed_nice=fix_int(ptr_env->nice * 2);
				   fixed_point_t temp1 = fix_unscale (ptr_env->recentCPU, 4);
				   fixed_point_t temp2 = fix_sub(FIXED_PRI_MAX,temp1);
				   fixed_point_t temp3 =fix_sub(temp2 ,fixed_nice);
				   ptr_env->priority =fix_trunc(temp3);
				   if(ptr_env->priority>PRI_MAX){
					   ptr_env->priority=PRI_MAX;
				   }
				   else if (ptr_env->priority<PRI_MIN){
					   ptr_env->priority=PRI_MIN;
				   }
				  // cprintf("\nreadypirority%u\n",ptr_env->priority);
				 if(oldPriority!=ptr_env->priority){//causes a problem when running the round roubin
				   movedEnv=ptr_env;
				   remove_from_queue(&(env_ready_queues[i]), ptr_env);
				   int queueIndex=PRI_MAX-ptr_env->priority;
				   enqueue(&(env_ready_queues[queueIndex]), movedEnv);
				   }
				}
			}
    	  }

    	  //lets update the priority for the running process also
    	  if(curenv!=NULL){
    		  //priority equation //priority equation //priority equation
    		   fixed_point_t FIXED_PRI_MAX=fix_int(PRI_MAX);
			   fixed_point_t fixed_nice=fix_int(curenv->nice*2);
			   fixed_point_t temp1 = fix_unscale (curenv->recentCPU, 4);
			   fixed_point_t temp2 = fix_sub(FIXED_PRI_MAX,temp1);
			   fixed_point_t temp3 =fix_sub(temp2 ,fixed_nice);
			   curenv->priority =fix_trunc(temp3);
			   if(curenv->priority>PRI_MAX){
				   curenv->priority=PRI_MAX;
			   }
			   else if (curenv->priority<PRI_MIN){
				   curenv->priority=PRI_MIN;
			   }
			  // cprintf("\ncurrnt pirority%u\n",curenv->priority);
    	  }
    	  //sched_print_all();
      }

      if(curruntTime>=1000){
    	  curruntTime-=1000;
      }
	}

	//sched_print_all();
	/********DON'T CHANGE THIS LINE***********/
	ticks++ ;
	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_TIME_APPROX))
	{
		update_WS_time_stamps();
	}
	//cprintf("Clock Handler\n") ;
	fos_scheduler();
	/*****************************************/
}

//===================================================================
// [9] Update LRU Timestamp of WS Elements
//	  (Automatically Called Every Quantum in case of LRU Time Approx)
//===================================================================
void update_WS_time_stamps()
{
	struct Env *curr_env_ptr = curenv;

	if(curr_env_ptr != NULL)
	{
		struct WorkingSetElement* wse ;
		{
			int i ;
#if USE_KHEAP
			LIST_FOREACH(wse, &(curr_env_ptr->page_WS_list))
			{
#else
			for (i = 0 ; i < (curr_env_ptr->page_WS_max_size); i++)
			{
				wse = &(curr_env_ptr->ptr_pageWorkingSet[i]);
				if( wse->empty == 1)
					continue;
#endif
				//update the time if the page was referenced
				uint32 page_va = wse->virtual_address ;
				uint32 perm = pt_get_page_permissions(curr_env_ptr->env_page_directory, page_va) ;
				uint32 oldTimeStamp = wse->time_stamp;

				if (perm & PERM_USED)
				{
					wse->time_stamp = (oldTimeStamp>>2) | 0x80000000;
					pt_set_page_permissions(curr_env_ptr->env_page_directory, page_va, 0 , PERM_USED) ;
				}
				else
				{
					wse->time_stamp = (oldTimeStamp>>2);
				}
			}
		}

		{
			int t ;
			for (t = 0 ; t < __TWS_MAX_SIZE; t++)
			{
				if( curr_env_ptr->__ptr_tws[t].empty != 1)
				{
					//update the time if the page was referenced
					uint32 table_va = curr_env_ptr->__ptr_tws[t].virtual_address;
					uint32 oldTimeStamp = curr_env_ptr->__ptr_tws[t].time_stamp;

					if (pd_is_table_used(curr_env_ptr->env_page_directory, table_va))
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2) | 0x80000000;
						pd_set_table_unused(curr_env_ptr->env_page_directory, table_va);
					}
					else
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2);
					}
				}
			}
		}
	}
}

