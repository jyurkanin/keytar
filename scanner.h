#pragma once

#include <X11/Xlib.h>

#define OMEGA (2*M_PI/44100.0) //sample rate adjusted conversion from Hz to rad/s
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

//Scanner is a model of connected point masses with springs and damping on the amplitude
//Essentially a model of a string
//Simplification: No connectivity matrix, assumed connected to adjacent point masses
//The altitude of each point forms a single cycle wave which is read repeatedly.
class Scanner{
 public:
  Scanner(int);
  ~Scanner();
  
  //actions
  float tick(float pitch);
  void reset();
  void strike();
  void setBoundaries(float start, float end); //boundary conditions for the wave. can be perturbed
  void draw_scanner(Display *dpy, Window w, GC gc);
  
  //getters, then setters
  void setFreq(float scan_freq);
  
 private:
  //actions
  void update_point(int i);
  
  int table_size;
  float *z_table;      //This is the height of the point mass of the spring mass damper
  float *zd_table;     //This is the velocity of the points
  float *zdd_table;    //this is the acceleration of the points

  float *x_table;
  
  float *z_center;    //provides centering force
  float *z_damping;  
  float *x_stiffness;  //is size+1. Stiffness of spring connections between masses. Connects to end points which I guess can be pertuerbed if you want. SO not necessarily boundary conditions = 0
  float *masses;       //masses
  
  float *hammer_table; //this is the shape of the hammer
  
  unsigned int k_; //k_ is the number of scanner updates. 
  unsigned int t_; //t_ is the number of sampling periods passed.
  float update_freq_;
  float time_step;

  const float EQ_LEN = 1; //length of the spring with no forces acting on it. Not sure this matters.
  const float X_DIST = 1;
  const float TIMESTEP = .01; //simulate with discrete timestep = 1 milliseconds
};
