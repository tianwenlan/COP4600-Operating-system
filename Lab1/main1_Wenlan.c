/* Auther: Wenlan Tian
 * Date: 02/07/2015
 * Lab01: part A
 * Possible problems in consumer-producer problem:
 * 1.Because the consumer and producer share the buffer, once the buffer is full, producer cannot input items any more, and once the buffer is empty, the consumers can neither take items from it.
 * 2.After the producer input the item, or the consumer take the item, it should tell the other know that they could use the buffer again
 * 3.When the consumer or producer is using the buffer, the other cannot use the buffer at the same time

 *
 * Lab01: part B
 */

/*  main.c  - main */

#include <xinu.h>
#include <stdio.h>

#define BUFFER_SIZE  8 //total numer of slots

int in = 0;
int out = 0;
int buffer[BUFFER_SIZE];
int produceItem = 0;

int produce_item(){
	int item = ++produceItem;
	kprintf("Producing %d ...\n", item);
	return item;
}

void input(int item){
	buffer[in] = item;
	in = (in+1)%BUFFER_SIZE;
}

int remove(){
	out = (out+1)%BUFFER_SIZE;
	return buffer[out];
}

void consume_item(int item){
	kprintf("Consuming  %d ...\n", item);
}

void producer(sid32,sid32,sid32);
void consumer(sid32,sid32,sid32);

int main(int argc, char **argv){

	//define three semaphores: full, empty, and mutex
	
	sid32 full; //keep track of the number of full spots
	sid32 empty; //keep track of the number of empty spots
	sid32 mutex; //enforce mutual exculsion to shared data

	full = semcreate(BUFFER_SIZE); // initialized to full size
	empty = semcreate(0); // initialized to 0
	mutex = semcreate(1); // initialized to 1

	resume(create(producer, 1024, 20, "producer", 3, full, empty, mutex));
	resume(create(consumer, 1024, 20, "consumer", 3, full, empty, mutex));

	return OK;
}

void producer (sid32 full, sid32 empty, sid32 mutex){
	while(1){
		produce_item();
		wait(empty);	// if it's empty, wait
		wait(mutex);	// if another process is using the buffer, wait
		input(produce_item());
		signal(mutex); // release the buffer
		signal(full); // increment the number of full slots
	}
}

void consumer(sid32 full, sid32 empty, sid32 mutex){
	while(1){
		wait(full);
		wait(mutex);
		remove();
		signal(mutex);
		signal(empty);
		consume_item(remove());
	}
}

