#include "controller.h"


unsigned char Controller::get_slider(int index){
  return controller.slider[index];
}

unsigned char Controller::get_knob(int index){
  return controller.knob[index];
}

unsigned char Controller::get_big_slide(){
  return controller.big_slider;
}

unsigned char Controller::get_big_knob(){
  return controller.big_knob;
}

unsigned char Controller::was_button_pressed(int index){
  int length = (sizeof(controller.button)/sizeof(*(controller.button)));
  if(index >= length){
    return 0;
  }
  if(controller.button[index]){
    controller.button[index] = 0;
    return 127;
  }
  else{
    return 0;
  }
}

unsigned char Controller::get_button_state(int index){
  int length = (sizeof(controller.button)/sizeof(*(controller.button)));
  if(index >= length){
    return 0;
  }
  return controller.button[index];
}

unsigned char Controller::was_start_pressed(){
  if(controller.start){
    controller.start = 0;
    return 127;
  }
  else{
    return 0;
  }
}

unsigned char Controller::was_stop_pressed(){
  if(controller.stop){
    controller.stop = 0;
    return 127;
  }
  else{
    return 0;
  }
}

unsigned char Controller::was_record_pressed(){
  if(controller.record){
    controller.record = 0;
    return 127;
  }
  else{
    return 0;
  }
}

unsigned char Controller::was_loop_back_pressed(){
  if(controller.loop_back){
    controller.loop_back = 0;
    return 127;
  }
  else{
    return 0;
  }
}

unsigned char Controller::was_fastforward_pressed(){
  if(controller.fastforward){
    controller.fastforward = 0;
    return 127;
  }
  else{
    return 0;
  }
}

unsigned char Controller::was_rewind_pressed(){
  if(controller.rewind){
    controller.rewind = 0;
    return 127;
  }
  else{
    return 0;
  }
}

void *Controller::read_controller(void *nothing){
  unsigned char c_byte;
  MidiByte packet[4]; //wow I hate typedefs Im so sorry
  while(1){
    read(controllerFD, &packet, sizeof(packet));
    printf("controller %d %d %d %d\n", packet[0], packet[1], packet[2], packet[3]);
    
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
