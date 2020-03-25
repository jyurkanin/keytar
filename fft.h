#pragma once
#include <deque>

void calc_fft(std::deque<float>::iterator buffer, float* result, int len);
void calc_fft(float* input, float* result, int len);
void calc_fft(char* input, float* result, int len);
