/* Author: Wenlan Tian
 * Date: 02/22/2015
 * Lab02
 */

/* process.h - add the following code to struct procent{}
 * umsg32 buffer[10];
 * uint32 tail;
 * uint32 size;
 *
 * create.c- initialize the tail, size by adding the following code in create() function
 * prptr->tail = 0;
 * prptr->size = 0;
 */

/*  main.c  - main */
#include <xinu.h>
#include <stdio.h>

sid32 mutex;

syscall sendMsg (pid32 pid,umsg32 msg){
	intmask	mask;			/* saved interrupt mask		*/
	struct	procent *prptr;		/* ptr to process' table entry	*/

	mask = disable();
	if (isbadpid(pid)) {
		restore(mask);
		wait(mutex);
		kprintf("!!!bad pid:%d\n",pid);
		signal(mutex);
		return SYSERR;
	}	//check the recipient process exists

	prptr = &proctab[pid];

	//check if the recipient has a msg outstanding
	if ((prptr->prstate == PR_FREE) || prptr->size == 10) {
		restore(mask);
		wait(mutex);
		kprintf("Error!Unable to send message.\n");
		signal(mutex);

		return SYSERR;
	}

	//if no msg outstanding -> send the msg
	prptr->buffer[prptr->tail] = msg;
	wait(mutex);
	kprintf("Process[%d] has sent message '%d' to processes[%d]\n",currpid,msg,pid);
	signal(mutex);
	prptr->tail++;
	prptr->size++;

	/* If recipient waiting or in timed-wait make it ready */
	if (prptr->prstate == PR_RECV) {
		ready(pid, RESCHED_YES);
	} else if (prptr->prstate == PR_RECTIM) {
		unsleep(pid);
		ready(pid, RESCHED_YES);
	}
	restore(mask);		/* restore interrupts */
	return OK;
}

umsg32 receiveMsg (void){
	intmask	mask;			/* saved interrupt mask		*/
	struct	procent *prptr;		/* ptr to process' table entry	*/
	umsg32	msg;			/* message to return		*/

	mask = disable();
	prptr = &proctab[currpid];

	//if no msg has arrived, receive changes to PR_RECV and calls resched
	if (prptr->size == 0) {
		prptr->prstate = PR_RECV;
		wait(mutex);
		kprintf("Process[%d] has no message available to be received.\n", currpid);
		signal(mutex);
		resched();		/* block until message arrives	*/
	}

	//Once execution passes if statement, receive extracts the msg, sets prhasmsg to FALSE
	msg = prptr->buffer[0];		/* retrieve message		*/

	umsg32 i = 0;
	for (i = 0; i < prptr->size; i++){
		prptr->buffer[i] = prptr->buffer[i+1];
	}

	prptr->tail--;
	prptr->size--;

	wait(mutex);
	kprintf("Process[%d] received message: %d\n", currpid, msg);
	signal(mutex);


	restore(mask);
	return msg;
}

uint32 sendMsgs (pid32 pid,umsg32* msgs,uint32 msg_count){
	intmask	mask;			/* saved interrupt mask		*/
	struct	procent *prptr;		/* ptr to process' table entry	*/

	mask = disable();

	if (isbadpid(pid)) {
		restore(mask);
		wait(mutex);
		kprintf("!!!bad pid:%d\n",pid);
		signal(mutex);
		return SYSERR;
	}	//check the recipient process exists

	prptr = &proctab[pid];

	//check if the recipient has a msg outstanding
	if ((prptr->prstate == PR_FREE) || prptr->size == 10 || msg_count > 10 || msg_count <= 0) {
		restore(mask);
		wait(mutex);
		kprintf("Error!Unable to send message.\n");
		signal(mutex);
		return SYSERR;
	}

	//if no msg outstanding -> send the msg
	uint32 free = 10 - prptr->size;

	uint32 sent = 0;

	while (sent != free && sent != msg_count){
		prptr->buffer[prptr->tail] = msgs[sent];
		wait(mutex);
		kprintf("Process[%d] has sent message '%d' to processes[%d]\n",currpid,msgs[sent],pid);
		signal(mutex);
		prptr->tail++;
		prptr->size++;
		sent++;
	}

	wait(mutex);
	kprintf("Process[%d] has sent %d message(s) to processes[%d] in total.\n",currpid,sent,pid);
	signal(mutex);

	/* If recipient waiting or in timed-wait make it ready */
	if (prptr->prstate == PR_RECV) {
		ready(pid, RESCHED_YES);
	} else if (prptr->prstate == PR_RECTIM) {
		unsleep(pid);
		ready(pid, RESCHED_YES);
	}
	restore(mask);		/* restore interrupts */
	return sent;
}

syscall receiveMsgs (umsg32* msgs,uint32 msg_count){
	intmask	mask;			/* saved interrupt mask		*/
	struct	procent *prptr;		/* ptr to process' table entry	*/
	umsg32	msg;			/* message to return		*/

	mask = disable();
	prptr = &proctab[currpid];

	wait(mutex);
	kprintf("current process has %d elements.\n", prptr->size);
	signal(mutex);

	//if current queue do not have enough msgs to receive
	if (prptr->size < msg_count) {
		prptr->prstate = PR_RECV;
		wait(mutex);
		kprintf("Process[%d] has not enough messages available to be received.\n", currpid);
		signal(mutex);
		resched();		/* block until message arrives	*/
	}else{

	//Once execution passes if statement, receive extracts the msg, sets prhasmsg to FALSE
	uint32 i = 0;

	for (i=0; i < msg_count; i++){
		msg = prptr->buffer[0];		/* retrieve message		*/

		umsg32 j = 0;

		for (j = 0; j < prptr->size; j++){
			prptr->buffer[j] = prptr->buffer[j+1];
		}

		prptr->tail--;
		prptr->size--;
		wait(mutex);
		kprintf("Process[%d] received message: %d\n", currpid, msg);
		signal(mutex);
		msgs[i] = msg;
	}

	}

	restore(mask);
	return OK;
}

uint32 sendnMsg (uint32 pid_count,pid32* pids,umsg32 msg){
	intmask	mask;			/* saved interrupt mask		*/
	struct	procent *prptr;		/* ptr to process' table entry	*/

	mask = disable();

	if(pid_count > 3 || pid_count <= 0){
		wait(mutex);
		kprintf("Error!Cannot only send to 1,2 or 3 receivers.\n");
		signal(mutex);

		return SYSERR;
	}

	uint32 success = 0;
	int32 i = -1;
	while (i != pid_count-1){
		i++;
		pid32 pid = pids[i];

		if (isbadpid(pid)) {
			restore(mask);
			wait(mutex);
			kprintf("!!!bad pid:%d\n",pid);
			signal(mutex);
			continue;
		//	return SYSERR;
		}//check the recipient process exists
		
		prptr = &proctab[pid];

		//check if the recipient has a msg outstanding
		//prptr->prhasmsg == Ture -> indicates a msg is waiting
		if ((prptr->prstate == PR_FREE) ||prptr->size == 10) {
			restore(mask);
			wait(mutex);
			kprintf("Error!Unable to send message to process[%d].\n",pid);
			signal(mutex);
			continue;
		//	return SYSERR;
		}

		//if no msg outstanding -> send the msg
		prptr->buffer[prptr->tail] = msg;
		wait(mutex);
		kprintf("Process[%d] has sent message '%d' to processes[%d]\n",currpid,msg,pid);
		signal(mutex);
		prptr->tail++;
		prptr->size++;
		success++;

		/* If recipient waiting or in timed-wait make it ready */
		if (prptr->prstate == PR_RECV) {
			ready(pid, RESCHED_YES);
		} else if (prptr->prstate == PR_RECTIM) {
			unsleep(pid);
			ready(pid, RESCHED_YES);
		}
		restore(mask);		/* restore interrupts */
	}

	wait(mutex);
	kprintf("Process[%d] has sent message to %d processe(s) in total.\n",currpid,success);
	signal(mutex);

	return success;
}

int main(int argc, char **argv){
	mutex = semcreate(1);

	pid32 p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19;

	////////////////Test Case 1///////////////////
	//p2 and p1 send '1' and '2' to p1
	
	p1 = create(receiveMsg, 4096, 20, "receiver1",0);
	p2 = create(sendMsg, 4096, 20, "sender2", 2, p1, 1);
	p3 = create(sendMsg, 4096, 20, "sender3", 2, p1, 2);

	resume(p2);
	resume(p3);
	resume(p1);

	////////////////Test Case 2///////////////////
	//p4 sends '3' to p5
	//p5 waits to receive msg until sent by p4

	p5 = create(receiveMsg, 4096, 20, "receiver5", 0, 0);
	p4 = create(sendMsg, 4096, 20, "sender4", 2, p5, 3);

	resume(p5);
	resume(p4);

	////////////////Test Case 3///////////////////
	//p6 sends '88' to badpid
	p6 = create(sendMsg, 4096, 20, "sender6", 2, 121, 88);
	resume(p6);

	////////////////Test Case 4///////////////////
	//p7 sens '5,6,7,8,9' to p8
	//p8 receives '5,6,7,8,9'

	umsg32 msgs1[5] = {5, 6, 7, 8, 9 };
	umsg32 msgsRec1[5];

	p8 = create(receiveMsgs, 4096, 20, "receiver8", 2, &msgsRec1, 5);
	p7 = create(sendMsgs, 4096, 20, "sender7", 3, p8, &msgs1, 5);

	resume(p7);
	resume(p8);

	////////////////Test Case 5///////////////////
	//p9 sends '10,11,12,13,14' to p11 first
	//p11 needs to receive 8 msgs, however,there is not enough, so it has to wait
	//p10 sends '15,16,17,18,19' to p11
	//p11 receives 8 msgs

	umsg32 msgs2[6] = {10, 11, 12, 13, 14, 15};
	umsg32 msgs3[5] = {16, 17, 18, 19, 20};
	umsg32 msgRec2[10];

	p11 = create(receiveMsgs, 4096, 20, "Receiver11", 2, &msgRec2, 8);
	p9 = create(sendMsgs, 4096, 20, "Sender9", 3, p11, &msgs2, 6);
	p10 = create(sendMsgs, 4096, 20, "Sender10", 3, p11, &msgs3, 5);

	resume(p9);
	resume(p11);
	resume(p10);

	////////////////Test Case 6///////////////////
	//p13 first send 10 msgs to p12
	//p14 send 5 msgs to p12 -> failed, since there is no avilable queue
	//p15 send 1 msg to p12 -> failed , since there is no avilable queue

	umsg32 msgs4[10] = { 20, 21, 22, 23, 24, 25, 26, 27, 28, 29 };
	umsg32 msgs5[5] = { 30, 31, 32, 33, 34 };

	p12 = create(receiveMsg, 4096, 20, "Receiver12", 0);

	p13 = create(sendMsgs, 4096, 20, "Sender13", 3, p12, &msgs4, 10);
	p14 = create(sendMsgs, 4096, 20, "Sender14", 3, p12, &msgs5, 5);
	p15 = create(sendMsg, 4096, 20, "Sender15", 2, p12, 35);

	resume(p13);
	resume(p14);
	resume(p15);

	////////////////Test Case 7///////////////////
	//test sendnMsg
	p16 = create(receiveMsg, 4096, 20, "receiver16",0);
	p17 = create(receiveMsg, 4096, 20, "receiver17",0);
//	p18 = create(receiveMsg, 4096, 20, "receiver18",0);

	pid32 pid[3] = {p16, p17, p18};
	p19 = create(sendnMsg, 4096, 20, "sendnMsg1", 3, 2,pid,8);

	resume(p19);
	resume(p16);
	resume(p17);
//	resume(p18);

	return OK;

}

