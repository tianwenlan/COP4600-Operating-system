/* Author: Wenlan Tian
 * Date: 02/07/2015
 * Lab01 C
 
 * Why deadlock free?
 * Deadlock happens if all five philosophers take their left/right fork at the same time.
 * To avoid this problem, in my code, when a philosopher wants to eat, he checks both forks. 
 * If both are free, he eats; if not, he waits on a condition variable.
 * When a philosopher finishes eating, he checks to see if his neighbors are waiting. 
 * If so, he calls signal on their condition variables so that they can recheck the chopsticks and eat if possible.
 */

/*  main.c  - main */
#include <xinu.h>
#include <stdio.h>

#define NUM 5 // number of philosophers
#define LEFT (i+NUM-1)%NUM // number of i's left
#define RIGHT (i+1)%NUM // number of i's right
#define THINKING 0
#define HUNGRY 1
#define EATING 2

int state[NUM]; // keep track of state
sid32 mutex;
sid32 s[NUM]; // semaphore per philosopher

void thinking(int i){
	kprintf("Thinking %d\n",i);
}

void eating(int i){
	kprintf("Eating %d\n",i);
}

void test(int i){
	if(state[i]== HUNGRY && state[LEFT]!=EATING && state[RIGHT]!=EATING){
		signal(s[i]);
	}
}

void take_fork(int i){
	wait(mutex);
	state[i] = HUNGRY;
	test(i); // try getting 2 forks
	signal(mutex);
	wait(s[i]); //block if no forks aquired
}

void put_fork(int i){
	wait(mutex);
	state[i]= THINKING;
	test(LEFT);
	test(RIGHT);
	signal(mutex);
}

// each philosophier is a loop of thinking, pick fork, eating, and place down fork
void philosopher(int i){	
	while(1){
		thinking(i);  // thinking
		take_fork(i); // get two forks, block
		eating(i); // eating
		put_fork(i); // place down fork
	}
}

int main(int argc, char **argv){

	mutex = semcreate(1);
	s[NUM] = semcreate(0);
	
	sched_cntl(DEFER_START);
	resume(create(philosopher, 4096, 20, "phil2", 1, 2));
	resume(create(philosopher, 4096, 20, "phil1", 1, 1));
	resume(create(philosopher, 4096, 20, "phil3", 1, 3));
	resume(create(philosopher, 4096, 20, "phil4", 1, 4));
	resume(create(philosopher, 4096, 20, "phil5", 1, 5));
	sched_cntl(DEFER_STOP);

	return OK;
}


