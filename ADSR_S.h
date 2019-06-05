#pragma once

#include "stk-4.6.0/include/ADSR.h"
#include "stk-4.6.0/include/OneZero.h"
#include "stk-4.6.0/include/Stk.h"

//adsr with filters for a synthesizer
class ADSR_S : public stk::ADSR {
public:
    ADSR_S();
    stk::StkFloat tick(stk::StkFloat input);
    void set_filters(stk::OneZero attack, stk::OneZero decay, stk::OneZero sustain, stk::OneZero release);
private:
    stk::OneZero attack_filter;
    stk::OneZero decay_filter;
    stk::OneZero sustain_filter;
    stk::OneZero release_filter;
};
