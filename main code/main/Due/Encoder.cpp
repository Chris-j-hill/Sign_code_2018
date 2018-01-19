
#ifndef Encoder_CPP
#define Encoder_CPP     
#include "Arduino.h"
#include "Encoder.h"
//#include "Due.h"

#ifdef ENABLE_ENCODER
bool enable_encoder = true;
#else
bool enable_encoder = false;
#endif

#ifdef ENABLE_BUTTON
bool enable_button = true;
#else
bool enable_button = false;
#endif

bool encoder_enabled = false;
bool button_enabled = false;


Encoder_Struct encoder_parameters;     //create encoder struct
Button_Struct button_parameters;       //create button struct


int init_encoder() {
  if (enable_encoder && !encoder_enabled) {

    pinMode(encoder_parameters.pinA, INPUT);
    pinMode(encoder_parameters.pinB, INPUT);

    attachInterrupt(encoder_parameters.pinA, update_encoder_ISR, CHANGE);


    return (0);
  }

  else {
    Sprintln(F("Conflict with enabling encoder: make sure only enabled once and 'ENABLE_ENCODER' defined"));
    return (-1);
  }

}

int init_button() {
  if(enable_button && !button_enabled){
    pinMode(button_parameters.button_pin, INPUT);
    attachInterrupt(button_parameters.button_pin, update_button_ISR, CHANGE);
    Serial.println(F("Button Initialised"));
    return(0);
  }
  else {
    Sprintln(F("Conflict with enabling button: make sure only 'ENABLE_BUTTON' defined and init_button called once"));
    return (-1);
  }
}

void update_encoder_ISR () {
  encoder_parameters.aVal = digitalRead(encoder_parameters.pinA);
  if (encoder_parameters.aVal != encoder_parameters.pinALast) { // Means the knob is rotating
    // if the knob is rotating, we need to determine direction
    // We do that by reading pin B.
    if (digitalRead(encoder_parameters.pinB) != encoder_parameters.aVal) {  // Means pin A Changed first - We're Rotating Clockwise
      encoder_parameters.PosCount ++;

    } else {// Otherwise B changed first and we're moving CCW

      encoder_parameters.PosCount--;

    }
    encoder_parameters.position = encoder_parameters.PosCount / 2;

  }

  encoder_parameters.pinALast = encoder_parameters.aVal;
  encoder_parameters.encoder_moved = true;

}

void update_button_ISR() {

  if (digitalRead(button_parameters.button_pin) == false && millis() - button_parameters.last_button_pressed >= button_parameters.button_press_interval) {
    Sprintln(F("Button Pressed"));
    button_parameters.last_button_pressed = millis();
    button_parameters.button_pressed = true;
  }
}


int get_text_encoder_position(int byte_number) {  //function to return the MSB or LSB of the current hue value to send

  if (byte_number == 1) { //looking for MSB
    if (encoder_parameters.position < 0)
      return (abs(encoder_parameters.position) / 256);    //get quotient of absolute value and 256 rounded down

    else
      return (abs(encoder_parameters.position) / 256 + 128); //add 128 to indicate positve number
  }
  else if (byte_number == 2) { //LSB
    return (abs(encoder_parameters.position) % 256);    //get modulo of value and 256;
  }
  else {
    Sprintln("Error, cant get hue MSB/LSB, invalid byte number presented");
    return (-1);
  }
}

#endif // Encoder_Cpp
