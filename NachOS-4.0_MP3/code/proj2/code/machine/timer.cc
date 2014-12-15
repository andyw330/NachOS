// timer.cc 
//	Routines to emulate a hardware timer device.
//
//      A hardware timer generates a CPU interrupt every X milliseconds.
//      This means it can be used for implementing time-slicing.
//
//      We emulate a hardware timer by scheduling an interrupt to occur
//      every time stats->totalTicks has increased by TimerTicks.
//
//      In order to introduce some randomness into time-slicing, if "doRandom"
//      is set, then the interrupt is comes after a random number of ticks.
//
//	Remember -- nothing in here is part of Nachos.  It is just
//	an emulation for the hardware that Nachos is running on top of.
//
//  DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "timer.h"
#include "main.h"
#include "sysdep.h"

//----------------------------------------------------------------------
// Timer::Timer
//      Initialize a hardware timer device.  Save the place to call
//	on each interrupt, and then arrange for the timer to start
//	generating interrupts.
//
//      "doRandom" -- if true, arrange for the interrupts to occur
//		at random, instead of fixed, intervals.
//      "toCall" is the interrupt handler to call when the timer expires.
//----------------------------------------------------------------------

Timer::Timer(bool doRandom, CallBackObj *toCall)
{
    randomize = doRandom;
    callPeriodically = toCall;
	
    disable = FALSE;
    SetInterrupt();
}

//----------------------------------------------------------------------
// Timer::CallBack
//      Routine called when interrupt is generated by the hardware 
//	timer device.  Schedule the next interrupt, and invoke the
//	interrupt handler.
//----------------------------------------------------------------------
void 
Timer::CallBack() 
{
    // invoke the Nachos interrupt handler for this device
    callPeriodically->CallBack();
    
    SetInterrupt();	// do last, to let software interrupt handler
    			// decide if it wants to disable future interrupts
}

//----------------------------------------------------------------------
// Timer::SetInterrupt
//      Cause a timer interrupt to occur in the future, unless
//	future interrupts have been disabled.  The delay is either
//	fixed or random.
//----------------------------------------------------------------------

void
Timer::SetInterrupt() 
{
    if (!disable) {
       int delay = callPeriodically->time_quantum;
	cout << "Debug: delay is " << delay << endl;
    
       if (randomize) {
	     delay = 1 + (RandomNumber() % (TimerTicks * 2));
        }
       // schedule the next timer device interrupt
       kernel->interrupt->Schedule(this, delay, TimerInt);
    }
}
