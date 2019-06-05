#include "ADSR_S.h"
#include "stk-4.6.0/include/Iir.h"

ADSR_S::ADSR_S(){
    stk::OneZero dummy(.01);
    attack_filter = dummy;
    decay_filter = dummy;
    sustain_filter = dummy;
    release_filter = dummy;
}

void ADSR_S::set_filters(stk::OneZero attack, stk::OneZero decay, stk::OneZero sustain, stk::OneZero release){
    attack_filter = attack;
    decay_filter = decay;
    sustain_filter = sustain;
    release_filter = release;
}

stk::StkFloat ADSR_S::tick(stk::StkFloat input){
    stk::StkFloat adsr_m = ADSR::tick();
    switch(state_){
    case RELEASE:
      return adsr_m * release_filter.tick(input);
    case DECAY:
      return adsr_m * decay_filter.tick(input);
    case SUSTAIN:
      return adsr_m * sustain_filter.tick(input);
    case ATTACK:
      return adsr_m * attack_filter.tick(input);
    default:
      return 0;
    }
    
}
