#include <math.h>
#include "filter.h"


LPFilter::LPFilter(float fc){
  alpha = 1/fc;
}

float LPFilter::tick(float xn){
  float yn = xn + (alpha*y_n_1);
  y_n_1 = yn;
  return yn;
}

void LPFilter::setCutoff(float fc){
  alpha = 1/fc;
}

void LPFilter::setQFactor(float r){
  
}



RFilter::RFilter(float r, float fc){
  r_ = r;
  fc_ = fc_;
  
  alpha[0] = -2*r*cos(OMEGA*fc);
  alpha[1] = r*r;
  
  beta[0] = (1- (r*r))/2;
  beta[1] = 0;
  beta[2] = -beta[0];
}

float RFilter::tick(float xn){
  y_n[2] = y_n[1];
  y_n[1] = y_n[0];

  x_n[2] = x_n[1];
  x_n[1] = x_n[0];
  
  x_n[0] = xn;
  y_n[0] = beta[0]*x_n[0] + beta[1]*x_n[1] + beta[2]*x_n[2];
  y_n[0] -= alpha[0]*y_n[1] + alpha[1]*y_n[2];
  
  return y_n[0];
}

void RFilter::setCutoff(float fc){
  alpha[0] = -2*r_*cos(OMEGA*fc);
  alpha[1] = r_*r_;
  
  beta[0] = (1- (r_*r_))/2;
  beta[1] = 0;
  beta[2] = -beta[0];
}

void RFilter::setQFactor(float r){
  alpha[0] = -2*r*cos(OMEGA*fc_);
  alpha[1] = r*r;
  
  beta[0] = (1- (r*r))/2;
  beta[1] = 0;
  beta[2] = -beta[0];
}


float RLPFilter::tick(float xn){
  return rf.tick(xn) + lpf.tick(xn);
}

void RLPFilter::setCutoff(float fc){
  rf.setCutoff(fc);
  lpf.setCutoff(fc);
}

void RLPFilter::setQFactor(float r){
  rf.setQFactor(r);
  lpf.setQFactor(r);
}
