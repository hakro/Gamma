/*	Gamma - Generic processing library
	See COPYRIGHT file for authors and license information
	
	Tutorial:		Generator / Oscillator / Accumulator
	Description:	Using an accumulator as a timer to trigger impulses
*/

#include "tutorial.h"

Accum<> tmr(10);			// timer to switch between LFO types

void audioCB(AudioIOData& io){

	for(uint32_t i=0; i<io.framesPerBuffer(); i++){
	
		float s = 0;
	
		if(tmr()){
			tmr.period(rnd::pick(0.4, 0.2));
			s = 0.2;
		}
		
		io.out(0)[i] = io.out(1)[i] = s;
	}
}

RUN(audioCB);
