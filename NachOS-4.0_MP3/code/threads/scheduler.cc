// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    readyList = new SortedList< Thread *>(Thread::compare_by_priority); // OAO
    readyRRList = new List< Thread *>(); // OAO work item 2(1)
    readySJFList = new SortedList< Thread *>(Thread::compare_by_burst);// OAO 2-2
    toBeDestroyed = NULL;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    delete readyList; 
    delete readyRRList;
    delete readySJFList;
}

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    thread->setStatus(READY);
    thread->setReadyTime(kernel->stats->totalTicks);//OAO work item 1(3)
    
    cout << "Thread " <<  thread->getID() << "\tProcessReady\t" << kernel->stats->totalTicks << endl;
    // 0 ~ 59 : priority queue
    if(thread->getPriority()<60){// OAO priority queue
        cout<<"Tick "<<kernel->stats->totalTicks<<" Thread "<<thread->getID()<<" move to Priority queue"<<endl;
        readyList->Insert(thread);// OAO Append is private
    }
    // 60 ~ 99 : RR queue
    else{ // OAO RR queue
        cout<<"Tick "<<kernel->stats->totalTicks<<" Thread "<<thread->getID()<<" move to RR queue"<<endl;
        readyRRList->Append(thread);
    }
    // 100 ~ 149 : SJF queue
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    aging(readyList);//OAO work item 1(3)
    aging(readyRRList);// OAO work item 2(1)
    // aging(readySJFList);// OAO ?
    moveBetweenQueues();//OAO check priority
    if(readySJFList->IsEmpty()){// 2-2
        if(readyRRList->IsEmpty()){// work item 2(1)
            if (readyList->IsEmpty()) {
                return NULL;
            } else {
                return readyList->RemoveFront();
            }    
        }
        else{
            return readyRRList->RemoveFront();
        }    
    }
    else{
        return readySJFList->RemoveFront();
    }
    
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    //OAO old thread == new thread => don't do context switch(?)
    // if(oldThread->getID()==nextThread->getID())return;
    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow
    // OAO 2-2?
    nextThread->setBurstTime();//OAO 2-2?
    nextThread->setStartBurstTime(kernel->stats->totalTicks);// OAO 2-2
    
    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    cout << "Thread " << kernel->currentThread->getID() << "\tProcessRunning\t" << kernel->stats->totalTicks << endl;
    
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    readyList->Apply(ThreadPrint);
}

void
Scheduler::aging(SortedList<Thread*>* list)// OAO
{// list = SJF/RR/priority queues
    ListIterator<Thread *> *iter = new ListIterator<Thread *>((List<Thread *>*)list);
    for (; !iter->IsDone(); iter->Next()) {
        Thread* thread = iter->Item();
        if(kernel->stats->totalTicks - thread->getReadyTime() >= 1500){
            list->Remove(thread);
            thread->setReadyTime(kernel->stats->totalTicks);
            thread->setPriority(thread->getPriority()+10);
            list->Insert(thread);
        }
    }
}
void
Scheduler::aging(List<Thread*>* list)// OAO
{// list = SJF/RR/priority queues
    ListIterator<Thread *> *iter = new ListIterator<Thread *>((List<Thread *>*)list);
    for (; !iter->IsDone(); iter->Next()) {
        Thread* thread = iter->Item();
        if(kernel->stats->totalTicks - thread->getReadyTime() >= 1500){
            list->Remove(thread);
            thread->setReadyTime(kernel->stats->totalTicks);
            thread->setPriority(thread->getPriority()+10);
            list->Append(thread);
        }
    }
}
void
Scheduler::moveBetweenQueues()// check priority OAO
{
    // after aging
    // 0 ~ 59 : priority queue
    // 60 ~ 99 : RR queue
    // 100 ~ 149 : SJF queue
    ListIterator<Thread *> *iter = new ListIterator<Thread *>((List<Thread *>*)readyList);
    for (; !iter->IsDone(); iter->Next()) {
        Thread* thread = iter->Item();
        if(60 <= thread->getPriority() && thread->getPriority() < 100){ 
            //to RR Q
            readyList->Remove(thread);
            readyRRList->Append(thread);
            cout<<"Tick "<<kernel->stats->totalTicks<<" Thread "<<thread->getID()<<" move to RR queue"<<endl;
        }
        else if(100 <= thread->getPriority()){
            //to SJF Q
            readyList->Remove(thread);
            readySJFList->Insert(thread);
            cout<<"Tick "<<kernel->stats->totalTicks<<" Thread "<<thread->getID()<<" move to SJF queue"<<endl;
        }
        else{// should I print this or not?
            cout<<"Tick "<<kernel->stats->totalTicks<<" Thread "<<thread->getID()<<" stay in Priority queue"<<endl;
        }
    }
    
    iter= new ListIterator<Thread *>((List<Thread *>*)readyRRList);
    for (; !iter->IsDone(); iter->Next()) {
        Thread* thread = iter->Item();
        if(thread->getPriority() < 60){
            // to priority Q
            readyRRList->Remove(thread);
            readyList->Insert(thread);
            cout<<"Tick "<<kernel->stats->totalTicks<<" Thread "<<thread->getID()<<" move to Priority queue"<<endl;
        }
        else if(100 <= thread->getPriority()){
            // to SJF Q
            readyRRList->Remove(thread);
            readySJFList->Insert(thread);
            cout<<"Tick "<<kernel->stats->totalTicks<<" Thread "<<thread->getID()<<" move to SJF queue"<<endl;
        }
        else{// should I print this or not?
            cout<<"Tick "<<kernel->stats->totalTicks<<" Thread "<<thread->getID()<<" stay in RR queue"<<endl;
        }
    }
    iter= new ListIterator<Thread *>((List<Thread *>*)readySJFList);
    for (; !iter->IsDone(); iter->Next()) {
        Thread* thread = iter->Item();
        if(thread->getPriority() < 60){
            // to priority Q
            readySJFList->Remove(thread);
            readyList->Insert(thread);
            cout<<"Tick "<<kernel->stats->totalTicks<<" Thread "<<thread->getID()<<" move to Priority queue"<<endl;
        }
        else if(60 <= thread->getPriority() && thread->getPriority() < 100){
            // to RR Q
            readySJFList->Remove(thread);
            readyRRList->Append(thread);
            cout<<"Tick "<<kernel->stats->totalTicks<<" Thread "<<thread->getID()<<" move to RR queue"<<endl;
        }
        else{// should I print this or not?
            cout<<"Tick "<<kernel->stats->totalTicks<<" Thread "<<thread->getID()<<" stay in SJF queue"<<endl;
        }
    }
}