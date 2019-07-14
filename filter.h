#define OMEGA (2*M_PI/44100.0) //sample rate adjusted conversion from Hz to rad/s


class Filter{
public:
  Filter(){}
  virtual float tick(float) = 0;
  virtual void setCutoff(float) = 0;
  virtual void setQFactor(float) = 0;
};

class LPFilter : public Filter{
 public:
  LPFilter(float fc);
  float tick(float);
  void setCutoff(float);
  void setQFactor(float); //doesnt have a q factor
 private:
  float alpha;
  float y_n[3];
  float x_n[3];
  float a[3];
  float b[3];
  float fc_old;
  float qf_old;
};

class RFilter : public Filter{
 public:
  RFilter(float r, float fc);
  float tick(float);
  void setCutoff(float);
  void setQFactor(float);
 private:
  float r_;
  float fc_;
  float alpha[2];
  float beta[3];
  float y_n[3]; //y_n, y_n-1, y_n-2
  float x_n[3];
};

class RLPFilter : public Filter{
 public:
  RLPFilter(float r, float fc) : rf(r, fc), lpf(fc){}
  float tick(float);
  void setCutoff(float);
  void setQFactor(float);
 private:
  RFilter rf;
  LPFilter lpf;
};
