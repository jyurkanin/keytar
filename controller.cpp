#include "controller.h"

int Controller::controllerFD;
pthread_t Controller::c_thread;
Controller *Controller::active_controller;

unsigned char Controller::get_slider(int index){
  return slider[index];
}

unsigned char Controller::get_knob(int index){
  return knob[index];
}

unsigned char Controller::get_big_slider(){
  return big_slider;
}

unsigned char Controller::get_big_knob(){
  return big_knob;
}

unsigned char Controller::was_button_pressed(int index){
  int length = (sizeof(button)/sizeof(*(button)));
  if(index >= length){
    return 0;
  }
  if(button[index]){
    button[index] = 0;
    return 127;
  }
  else{
    return 0;
  }
}

unsigned char Controller::was_start_pressed(){
  if(start){
    start = 0;
    return 127;
  }
  else{
    return 0;
  }
}

unsigned char Controller::was_stop_pressed(){
  if(stop){
    stop = 0;
    return 127;
  }
  else{
    return 0;
  }
}

unsigned char Controller::was_record_pressed(){
  if(record){
    record = 0;
    return 127;
  }
  else{
    return 0;
  }
}

unsigned char Controller::was_loop_back_pressed(){
  if(loop_back){
    loop_back = 0;
    return 127;
  }
  else{
    return 0;
  }
}

unsigned char Controller::was_fastforward_pressed(){
  if(fastforward){
    fastforward = 0;
    return 127;
  }
  else{
    return 0;
  }
}

unsigned char Controller::was_rewind_pressed(){
  if(rewind){
    rewind = 0;
    return 127;
  }
  else{
    return 0;
  }
}

int Controller::has_new;

int Controller::has_new_data(){
  if(has_new){
    has_new = 0;
    return 1;
  }
  return 0;
}

void *Controller::read_controller(void *nothing){
  unsigned char c_byte;
  unsigned char packet[4]; //wow I hate typedefs Im so sorry
  has_new = 1;
  while(1){
    read(controllerFD, &packet, sizeof(packet));
    has_new = 1;
    
    if(packet[0] == PEDAL){
      if(packet[1] >= 3 && packet[1] <= 11){
	active_controller->slider[packet[1] - 3] = packet[2];
      }
      else if(packet[1] >= 14 && packet[1] <= 22){
	active_controller->knob[packet[1] - 14] = packet[2];
      }
      else if(packet[1] == 9){
	active_controller->big_slider = packet[2];
      }
      else if(packet[1] == 10){
	if(packet[2]) active_controller->big_knob = packet[2];
      }
      else if(packet[1] >= 23 && packet[1] <= 31 && packet[2]){
	active_controller->button[packet[1] - 23] = packet[2];
      }
      else if(packet[2]){ //only assign it if it was pressed. The application that reads it will have to reset the button.
	switch(packet[1]){
	case 49:
	  active_controller->loop_back = packet[2];
	  break;
	case 47:
	  active_controller->rewind = packet[2];
	  break;
	case 48:
	  active_controller->fastforward = packet[2];
	  break;
	case 46:
	  active_controller->stop = packet[2];
	  break;
	case 45:
	  active_controller->start = packet[2];
	  break;
	case 44:
	  active_controller->record = packet[2];
	  break;
	}
      }
    }
  }
}

//this sets this controller to the one being written to by the
// controller reading thread
void Controller::activate(){
  active_controller = this;
}

int Controller::init_controller(int argc, char *argv[]){
  char *midi_controller = argv[2];
  controllerFD = open(midi_controller, O_RDONLY);
  if(controllerFD < 0){
    printf("Couldn't open device %s\n", midi_controller);
    exit(-1);
  }
  
  pthread_create(&c_thread, NULL, &read_controller, NULL);
  return EXIT_SUCCESS;
}

int Controller::exit_controller(){
  pthread_join(c_thread, NULL);
  close(controllerFD);
  return EXIT_SUCCESS;
}
