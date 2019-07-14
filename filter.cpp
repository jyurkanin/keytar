#include <math.h>
#include "filter.h"
#include <stdio.h>

LPFilter::LPFilter(float fc){
  fc_old = fc;
  qf_old = 1;
  float w0 = OMEGA*fc;
  alpha = sin(w0)/2;
  b[0] = (1 - cos(w0))/2;
  b[1] = (1-cos(w0));
  b[2] = b[0];
  
  a[0] = 1 + alpha;
  a[1] = -2*cos(w0);
  a[2] = 1 - alpha;

  for(int i = 0; i < 3; i++){
    x_n[0] = y_n[0] = 0;
  }
  
}

float LPFilter::tick(float xn){
  x_n[2] = x_n[1];
  x_n[1] = x_n[0];
  x_n[0] = xn;
  
  y_n[2] = y_n[1];
  y_n[1] = y_n[0];
  y_n[0] = (x_n[0]*b[0]/a[0]) + (x_n[1]*b[1]/a[0]) + (x_n[2]*b[2]/a[0]) - (y_n[1]*a[1]/a[0]) - (y_n[2]*a[2]/a[0]);
  return y_n[0];
}

void LPFilter::setCutoff(float fc){
  if(fc == fc_old) return;
  fc_old = fc;
  float w0 = OMEGA*fc;
  alpha = sin(w0)/(2*qf_old);
  b[0] = (1 - cos(w0))/2;
  b[1] = (1-cos(w0));
  b[2] = b[0];
  
  a[0] = 1 + alpha;
  a[1] = -2*cos(w0);
  a[2] = 1 - alpha;

}

void LPFilter::setQFactor(float qf){
  if(qf == qf_old) return;
  qf_old = qf;
  float w0 = OMEGA*fc_old;
  alpha = sin(w0)/(2*qf);
  /*  b[0] = (1 - cos(w0))/2;
  b[1] = (1-cos(w0));
  b[2] = b[0];*/
  
  a[0] = 1 + alpha;
  a[1] = -2*cos(w0);
  a[2] = 1 - alpha;
}



RFilter::RFilter(float r, float fc){
  r_ = r;
  fc_ = fc;
  
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
  fc_ = fc;
  alpha[0] = -2*r_*cos(OMEGA*fc);
  alpha[1] = r_*r_;
  
  beta[0] = (1- (r_*r_))/2;
  beta[1] = 0;
  beta[2] = -beta[0];
}

void RFilter::setQFactor(float r){
  r_ = r;
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
