/*
 * Driver code for airballoon problem
 */
#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

#define N_LORD_FLOWERKILLER 8
#define NROPES 16
static int ropes_left = NROPES;

/* Data structures for rope mappings */

//We dont need lock for hook because only dandelion will access it.
struct hook{
    volatile int stake_num; //The stake attached to the hook
    volatile bool severed; //Indicate if the rope is severed
};

struct ground_stake{
    volatile int hook_num; // The hook attached to the stake
    volatile bool severed; //Indicate if the rope is severed
    struct lock *stake_lk; //lock for stake
};
/* Implement this! */

/* Synchronization primitives */
struct semaphore *main_exit;// semaphore for blocking the main thread
struct semaphore *balloon_exit; //semaphore for blocking the ballon thread
struct lock *ropes_left_lk;// lock for ropes_left

/* struct for holding nropes of hooks and stakes*/
struct hook hooks[NROPES];
struct ground_stake ground_stakes[NROPES];

/* Implement this! */

/*
 * Describe your design and any invariants or locking protocols
 * that must be maintained. Explain the exit conditions. How
 * do all threads know when they are done?
 */

static
void
dandelion(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Dandelion thread starting\n");

    while(true){
        
        //check if all ropes are severed
        lock_acquire(ropes_left_lk);
        if (ropes_left <= 0) {
            lock_release(ropes_left_lk);
            break;
        }
        lock_release(ropes_left_lk);

        
        int sever_num = random() % NROPES;//generate random number to cut rope
        if(hooks[sever_num].severed == false){
            int opposite_num = hooks[sever_num].stake_num;//get the number of stakes attached.
            lock_acquire(ground_stakes[opposite_num].stake_lk);
            //release the lock if info not match
            if (ground_stakes[opposite_num].hook_num != sever_num) {
                lock_release(ground_stakes[opposite_num].stake_lk);
                continue;
            }
            //check if both ends are attached
            if(ground_stakes[opposite_num].severed == false && hooks[sever_num].severed == false){
                //cutting
                hooks[sever_num].severed = true;
                ground_stakes[opposite_num].severed = true;
                //reduce the ropes left
                lock_acquire(ropes_left_lk);
                ropes_left --;
                lock_release(ropes_left_lk);
                kprintf("Dandelion severed rope %d.\n",sever_num);
                lock_release(ground_stakes[opposite_num].stake_lk);
                thread_yield();
                continue;
            }
            else{
                lock_release(ground_stakes[opposite_num].stake_lk);
                continue;
            }
        }else{
            continue;
        }
    }
    kprintf("Dandelion thread done\n");
    V(balloon_exit);
    V(main_exit);
}

static
void
marigold(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Marigold thread starting\n");

    while(true){
        
        //check if all ropes are severed
        lock_acquire(ropes_left_lk);
        if (ropes_left <= 0) {
            lock_release(ropes_left_lk);
            break;
        }
        lock_release(ropes_left_lk);

        
        int sever_num = random() % NROPES;//generate random number to cut rope
        lock_acquire(ground_stakes[sever_num].stake_lk);
        if(ground_stakes[sever_num].severed == false){
            int opposite_num = ground_stakes[sever_num].hook_num;//get the number of stakes attached.
            //release the lock if info not match
            if (hooks[opposite_num].stake_num != sever_num) {
                lock_release(ground_stakes[sever_num].stake_lk);
                continue;
            }
            //check if both ends are attached
            if(hooks[opposite_num].severed == false && ground_stakes[sever_num].severed == false){
                //cutting
                ground_stakes[sever_num].severed = true;
                hooks[opposite_num].severed = true;
                //reduce the ropes left
                lock_acquire(ropes_left_lk);
                ropes_left --;
                lock_release(ropes_left_lk);
                kprintf("Marigold severed rope %d from stake %d\n",opposite_num,sever_num);
                lock_release(ground_stakes[sever_num].stake_lk);
                thread_yield();
                continue;
            }
            else{
                lock_release(ground_stakes[sever_num].stake_lk);
                continue;
            }
        }else{
            lock_release(ground_stakes[sever_num].stake_lk);
            continue;
        }
    }
    kprintf("Marigold thread done\n");
    V(balloon_exit);
    V(main_exit);
}

static
void
flowerkiller(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Lord FlowerKiller thread starting\n");
    
    int tmp;
    while(ropes_left>1){
        //generate switch number of ropes
        int switch_idx_1 = random()%NROPES;
        int switch_idx_2 = random()%NROPES;
        if (switch_idx_1 != switch_idx_2){
            //always locking the lower index first to avoid deadlock by swapping the index
            if (switch_idx_1 > switch_idx_2){
                tmp =switch_idx_1;
                switch_idx_1 = switch_idx_2;
                switch_idx_2 = tmp;
            }
            //aquire locks
            lock_acquire(ground_stakes[switch_idx_1].stake_lk);
            lock_acquire(ground_stakes[switch_idx_2].stake_lk);
            
            //check if both ends are attached
            if (ground_stakes[switch_idx_1].severed == false && ground_stakes[switch_idx_2].severed == false){
                
                //swap both the hooks and the stakes
                hooks[ground_stakes[switch_idx_1].hook_num].stake_num = switch_idx_2;
                hooks[ground_stakes[switch_idx_2].hook_num].stake_num = switch_idx_1;
                tmp = ground_stakes[switch_idx_1].hook_num;
                ground_stakes[switch_idx_1].hook_num = ground_stakes[switch_idx_2].hook_num;
                ground_stakes[switch_idx_2].hook_num = tmp;
                kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d\n",ground_stakes[switch_idx_2].hook_num,switch_idx_1,switch_idx_2);
                kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d\n",ground_stakes[switch_idx_1].hook_num,switch_idx_2,switch_idx_1);
            }
            lock_release(ground_stakes[switch_idx_1].stake_lk);
            lock_release(ground_stakes[switch_idx_2].stake_lk);
            thread_yield();
        }else{
            continue;//do another random
        }
    }
    kprintf("Lord FlowerKiller thread done\n");
    V(balloon_exit);
    V(main_exit);
}

static
void
balloon(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;

	kprintf("Balloon thread starting\n");
    
    //wait till lordflowerkiller threads and dandelion and marigold thread to finish
    for(int k = 0; k < N_LORD_FLOWERKILLER+2; k++){//blocking ballon thread
        P(balloon_exit);
    }
    kprintf("Balloon freed and Prince Dandelion escapes!\n");
    kprintf("Balloon thread done\n");
    V(main_exit);
}


// Change this function as necessary
int
airballoon(int nargs, char **args)
{

	int err = 0, i;

	(void)nargs;
	(void)args;
	(void)ropes_left;

    
    
    //init
    
    for (int j = 0;j<NROPES; j++){
        hooks[j].stake_num = j;
        hooks[j].severed = false;
        ground_stakes[j].hook_num = j;
        ground_stakes[j].severed = false;
        ground_stakes[j].stake_lk = lock_create("stakes");
    }
    ropes_left_lk = lock_create("ropes_left");
    balloon_exit = sem_create("ballon_exit", 0);
    main_exit = sem_create("main_exit",0);
    
    
	err = thread_fork("Marigold Thread",
			  NULL, marigold, NULL, 0);
	if(err)
		goto panic;

	err = thread_fork("Dandelion Thread",
			  NULL, dandelion, NULL, 0);
	if(err)
		goto panic;

	for (i = 0; i < N_LORD_FLOWERKILLER; i++) {
		err = thread_fork("Lord FlowerKiller Thread",
				  NULL, flowerkiller, NULL, 0);
		if(err)
			goto panic;
	}

	err = thread_fork("Air Balloon",
			  NULL, balloon, NULL, 0);
	if(err)
		goto panic;

	goto done;
panic:
	panic("airballoon: thread_fork failed: %s)\n",
	      strerror(err));

done:
    for(int k = 0; k < N_LORD_FLOWERKILLER+3; k++){ //blocking main thread
        P(main_exit);
    }
    //cleaning up
    for (int j = 0;j<NROPES; j++){
        lock_destroy(ground_stakes[j].stake_lk);
    }
    lock_destroy(ropes_left_lk);
    sem_destroy(balloon_exit);
    sem_destroy(main_exit);
    ropes_left = NROPES;
    kprintf("Main thread done\n");
	return 0;
}
