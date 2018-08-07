
#ifndef Coms_CPP
#define Coms_CPP

#include "Coms.h"
#include "Arduino.h"
#include "function_declarations.h"
#include "Due_Pins.h"
#include "Graphics.h"
#include "Current_Control.h"
#include "Fans.h"
#include "Internet.h"
#include "SD_Cards.h"
#include "Led_Strip.h"
#include "Encoder.h"
#include "LUTS.h"
#include "Config_Local.h"

// variables

//create these structs
Frame text_frame;
Frame menu_frame;
Frame pos_frame;
Frame sensor_data_frame;
Frame ping_frame;
//give access to these structs
extern struct Led_Strip_Struct led_strip_parameters;
extern struct Temp_sensor temp_parameters;
extern struct Fan_Struct fan_parameters;        //create fan struct
extern struct Timers timers;
extern struct Encoder_Struct encoder_parameters;     //create encoder struct
extern struct Button_Struct button_parameters;       //create button struct
extern struct LDR_Struct light_sensor_parameters;
extern struct Current_Meter_Struct current_meter_parameters;
extern struct SD_Card card1;
extern struct SD_Card card2;
extern struct Text text_parameters[MAX_NUM_OF_TEXT_OBJECTS];
extern struct Text_cursor text_cursor[MAX_NUM_OF_TEXT_OBJECTS];

extern Encoder encoder;


bool send_text_now = false;
extern volatile bool send_pos_now;  //variable set in interrupt to trigger send pos function in main loop. (serial doesnt work in interrutps)


byte comms_delay = 0;
byte pos_frame_length = 13;   //length of frame to transmit to update pos

//char menu_frame[FRAME_OVERHEAD+3] ={0};   // initialise menu_frame, overhead +menu numeber + 2 bytes for encoder position
//                                          // should only send references to strings already in megas LUT. Names from files need to be handled seperately

extern char text_str[MAX_NUM_OF_TEXT_OBJECTS][MAX_TWEET_SIZE];



extern byte screen_brightness;
extern byte screen_mode;   //mode of operation on startup should be both displaying
//mode0: both on
//mode1: one side on
//mode2: both off
//mode3: other side on



#ifndef ALL_MEGAS_ENABLED
bool mega_enabled[4] = {MEGA_1_ENABLED, MEGA_2_ENABLED, MEGA_3_ENABLED, MEGA_4_ENABLED};
#else
bool mega_enabled[4] = {true, true, true, true};  // ignore communication if board is false
#endif



void Coms::pack_disp_string_frame(uint16_t frame_num, byte obj_num) {   //function to pack a frame of text to display

  // function to pack a frame based on a given offset (ie this frames_num)

  uint16_t frame_offset = ((frame_num - 1) * (FRAME_DATA_LENGTH - 1)); //if this frame is 1 offset in data set is 0, if 2 offset 26, etc

  for (int i = 0; i < strlen(text_str[obj_num]) - frame_offset; i++) { //loop through string until end or break
    text_frame.frame_buffer[i + HEADER_LENGTH + 1] = (byte)text_str[obj_num][frame_offset + i]; //HEADER_LENGTH+1 for text obj_num
    if (i == FRAME_DATA_LENGTH) break;     //copy string until end or max bytes copied (frame full)
  }


  //incorporate error correcting data
#ifdef DO_ERROR_CHECKING  //do at least minimal error checking using 8 bit checksum
#ifdef DO_HEAVY_ERROR_CHECKING     // parity checking each byte and 13 bit checksum, hamming encoding done inside function
  set_frame_parity_and_checksum(TEXT_FRAME_TYPE, text_frame.frame_length);

#else
#ifdef DO_HEADER_ERROR_CORRECTING    // just do hamming but no parity, then do 8 bit checksum
  hamming_encoder(frame_type);
  text_frame.frame_buffer[text_frame.frame_length - 2] = generate_checksum(TEXT_FRAME_TYPE);
#endif
#endif

#ifndef DO_HEAVY_ERROR_CHECKING //if neither defined, just do 8 bit checksum
#ifndef DO_HEADER_ERROR_CORRECTING
  text_frame.frame_buffer[text_frame.frame_length - 2] = generate_checksum(TEXT_FRAME_TYPE);
#endif
#endif
#endif
  text_frame.frame_buffer[text_frame.frame_length - 1] = ENDBYTE_CHARACTER;
}

void Coms::build_pos_frame(byte obj_num) {

  pack_xy_coordinates(obj_num);        //seperate function to bit shift values to correct order.
  pos_frame.frame_buffer[7] =  text_cursor[obj_num].x_pos_dir;
  pos_frame.frame_buffer[8] =  text_cursor[obj_num].y_pos_dir;
  pos_frame.frame_buffer[9] = comms_delay; //maybe implement this to sync up screens if needed
  pos_frame.frame_buffer[10] = obj_num;

  //  pos_frame.frame_buffer[pos_frame.checksum_address] = pos_frame.header_checksum; //zero checksum
  //  for (int alpha = HEADER_LENGTH; alpha < pos_frame.checksum_address; alpha++) { //sum of elements
  //    pos_frame.frame_buffer[pos_frame.checksum_address] = pos_frame.frame_buffer[pos_frame.checksum_address] + pos_frame.frame_buffer[alpha];
  //  }
  //  pos_frame.frame_buffer[pos_frame.checksum_address] = (byte) 256 - (pos_frame.frame_buffer[pos_frame.checksum_address] % 256); //calc checksum

  pos_frame.frame_buffer[pos_frame.checksum_address] = generate_checksum(POS_FRAME_TYPE);
  pos_frame.frame_buffer[pos_frame.checksum_address + 1] = ENDBYTE_CHARACTER;

}

void Coms::pack_xy_coordinates(byte obj_num) {       //function to pack the 4 bytes to send the x and y positions of the text cursor

  // Wire.write can only handle bytes (0-256) whereas we will need to use positive and negaitve values potientially significantly larger than 256
  // to accomplish this two sucessive bytes are sent to repersent one number. to deal with positive and negative numbers, the coordinate is split into
  // two bytes, leasat significant 8 bits are the second byte and the most significant 7 bits are in the first byte sent. if the coordinate is negaitve the
  // remaining bit is set 0 and 1 otherwise.

  // NOTE: current implementation will overflow if an out of bounds coordinate is presented (+/-32738 is usable)
  //       I cant see a reason this would be an issue so not fixing it for now
  //
  //  byte x_pos_LSB = 0;
  //  byte x_pos_MSB = 0;
  //  byte y_pos_LSB = 0;
  //  byte y_pos_MSB = 0;
  //
  //
  //  if (abs(text_cursor[obj_num].x) > 32738 || abs(text_cursor[obj_num].y) > 32738) //print warning that coordinate will be wrong
  //    Sprintln("WARNING: failed to send correct coordinate, out of bounds, overflow likely to occur");
  //
  //  if (text_cursor[obj_num].x > 0) {
  //    x_pos_MSB = text_cursor[obj_num].x >> 8; //take the multiples 256 and set as MS byte
  //    x_pos_MSB = x_pos_MSB + 128; //greater than zero, set MSB to 1
  //    x_pos_LSB = text_cursor[obj_num].x % 256; //take the modulo to get the LS byte
  //  }
  //  else {
  //    x_pos_MSB = text_cursor[obj_num].x >> 8; //take the multiples 256 and set as MS byte
  //    x_pos_LSB = text_cursor[obj_num].x % 256; //take the modulo to get the LS byte
  //  }
  //
  //  if (text_cursor[obj_num].y > 0) {
  //    y_pos_MSB = text_cursor[obj_num].y >> 8; //take the multiples 256 and set as MS byte
  //    y_pos_MSB = y_pos_MSB + 128; //greater than zero, set MSB to 1
  //    y_pos_LSB = text_cursor[obj_num].y % 256; //take the modulo to get the LS byte
  //  }
  //  else {
  //    y_pos_MSB = text_cursor[obj_num].y >> 8; //take the multiples 256 and set as MS byte
  //    y_pos_LSB = text_cursor[obj_num].y % 256; //take the modulo to get the LS byte
  //  }
  //
  //  pos_frame.frame_buffer[4] = x_pos_MSB;   //write new values to frame
  //  pos_frame.frame_buffer[5] = x_pos_LSB;
  //  pos_frame.frame_buffer[6] = y_pos_MSB;
  //  pos_frame.frame_buffer[7] = y_pos_LSB;

  pos_frame.frame_buffer[3] = ((text_cursor[obj_num].x >> 8) & 0xFF);   //write new values to frame
  pos_frame.frame_buffer[4] = (text_cursor[obj_num].x & 0xFF);
  pos_frame.frame_buffer[5] = ((text_cursor[obj_num].y >> 8) & 0xFF);
  pos_frame.frame_buffer[6] = (text_cursor[obj_num].y & 0xFF);

}


void Coms::calc_delay() {    // function to calculate the dalay in sending frames to the megas
  // this could be useful for syncing up the screen updates. this value is only necessary for the pos data
  //frame as the other ones dont require accurate timing
  int time_to_read_millis = abs(millis() - millis());
  uint32_t round_trip_time = millis();

  //send_all_pos_now();
  round_trip_time = millis() - round_trip_time - time_to_read_millis;
  comms_delay = round_trip_time / (NUM_SCREENS * 2); //time to send one frame

}

byte Coms::get_text_colour_hue(byte byte_number, byte obj_num) { //function to return the MSB or LSB of the current hue value to send

  //  if (byte_number == 1) { //looking for MSB
  //    if (text_parameters[obj_num].hue < 0)
  //      return (abs(text_parameters[obj_num].hue) /256);    //get quotient of absolute value and 256 rounded down
  //
  //    else
  //      return (abs(text_parameters[obj_num].hue) / 256 + 128); //add 128 to indicate positve number
  //  }
  //  else if (byte_number == 2) { //LSB
  //    return (abs(text_parameters[obj_num].hue) % 256);    //get modulo of value and 256;
  //  }

  if (byte_number == 1) return ((text_parameters[obj_num].hue >> 8) & 0xFF);   //return msb
  else return (text_parameters[obj_num].hue & 0xFF);

}

void Coms::init_frames() {

  // text frame
  text_frame.frame_type = TEXT_FRAME_TYPE;

  // pos frame
  pos_frame.frame_length = POS_FRAME_LENGTH;
  pos_frame.frame_type = POS_FRAME_TYPE;
  pos_frame.frame_buffer[0] = pos_frame.frame_length;
  pos_frame.frame_buffer[1] = pos_frame.frame_type;
  pos_frame.frame_buffer[2] = PACK_FRAME_NUM_DATA(1, 1);

  pos_frame.header_checksum = pos_frame.frame_buffer[0] + pos_frame.frame_buffer[1] + pos_frame.frame_buffer[2];
  pos_frame.checksum_address = pos_frame.frame_length - TRAILER_LENGTH;

  // sensor_data_frame
  sensor_data_frame.frame_type = SENSOR_FRAME_TYPE;
  sensor_data_frame.frame_buffer[1] = sensor_data_frame.frame_type;
  pos_frame.frame_buffer[2] = PACK_FRAME_NUM_DATA(1, 1); //doesnt matter if it thinks theres one frame or many, data not related to eachother

  sensor_data_frame.header_checksum = sensor_data_frame.frame_buffer[1] + sensor_data_frame.frame_buffer[2];


  //menu_frame
  menu_frame.frame_length = MENU_FRAME_LENGTH;  //three pieces of data, current menu + 2 encoder pos bytes
  menu_frame.frame_type = MENU_FRAME_TYPE;

  menu_frame.frame_buffer[0] = menu_frame.frame_length;
  menu_frame.frame_buffer[1] = menu_frame.frame_type;
  pos_frame.frame_buffer[2] = PACK_FRAME_NUM_DATA(1, 1);

  menu_frame.header_checksum = menu_frame.frame_buffer[0] + menu_frame.frame_buffer[1] + menu_frame.frame_buffer[2];
  menu_frame.checksum_address = menu_frame.frame_length - 2;

  //ping frame
  ping_frame.frame_length = PING_FRAME_LENGTH;
}


void Coms::build_menu_data_frame(byte menu_number) {   //function to build the frame to send menu info
  //byte type = 4;

  menu_frame.frame_buffer[3] = (byte) menu_number;
  menu_frame.frame_buffer[4] = (byte) encoder.get_text_encoder_position(1);
  menu_frame.frame_buffer[5] = (byte) encoder.get_text_encoder_position(2);

  //  menu_frame.frame_buffer[menu_frame.checksum_address] = menu_frame.header_checksum; //initial checksum
  //
  //  for (byte alpha = HEADER_LENGTH; alpha < menu_frame.checksum_address; alpha++) { //sum of elements
  //    menu_frame.frame_buffer[menu_frame.checksum_address] = menu_frame.frame_buffer[menu_frame.checksum_address] + menu_frame.frame_buffer[alpha];
  //  }
  //  menu_frame.frame_buffer[menu_frame.checksum_address] = (byte) 256 - (menu_frame.frame_buffer[menu_frame.checksum_address] % 256); //calc checksum

  menu_frame.frame_buffer[menu_frame.checksum_address] = generate_checksum(MENU_FRAME_TYPE);
  menu_frame.frame_buffer[menu_frame.checksum_address + 1] = ENDBYTE_CHARACTER;


}

uint16_t Coms::generate_checksum(byte frame_type, uint16_t modulo_mask) {

  byte *frame_index_zero;  //pointer to address of first element of given array
  byte frame_length;



  switch (frame_type) {
    case TEXT_FRAME_TYPE:
      frame_length = text_frame.frame_length;
      frame_index_zero = text_frame.frame_buffer;   //pointer to this buffer element 0
      break;
    case POS_FRAME_TYPE:
      frame_length = pos_frame.frame_length;
      frame_index_zero = pos_frame.frame_buffer;
      break;
    case SENSOR_FRAME_TYPE:
      frame_length = sensor_data_frame.frame_length;
      frame_index_zero = sensor_data_frame.frame_buffer;
      break;
    case MENU_FRAME_TYPE:
      frame_length = menu_frame.frame_length;
      frame_index_zero = menu_frame.frame_buffer;
      break;
    case PING_STRING_TYPE:
      frame_length = ping_frame.frame_length;
      frame_index_zero = ping_frame.frame_buffer;
      break;
    default:
      Serial.println("checksum calc error");
      return (0);
  }

  //calculate checksum
  uint16_t checksum = 0;  //set checksum as 16 bit number and modulo to fit
  for (byte i = 0; i < frame_length - 2; i++) {
    checksum += *(frame_index_zero + i);          //sum all elements
  }

  checksum = checksum & modulo_mask;
  return checksum;

}

bool Coms::error_sanity_check(byte frame_num, byte obj_num) {
  if (!text_cursor[obj_num].object_used)
    return false;
  if (frame_num > (1 + (strlen(text_str[obj_num]) / (FRAME_DATA_LENGTH - 1)))) //requested frame num not possible for this obj
    return false;
  return true;
}


void Coms::set_frame_parity_and_checksum(byte frame_type, byte frame_length) {

  //heavy error checking sets parity of all bytes and 13 bit checksum
#ifdef DO_HEAVY_ERROR_CHECKING
  set_header_parity(frame_type);  //same regardless of frame type or length
#endif

  //header error correction applies hamming code
#ifdef DO_HEADER_ERROR_CORRECTING
  hamming_encoder(frame_type);
#endif

#ifdef DO_HEAVY_ERROR_CHECKING
  if (frame_type == TEXT_FRAME_TYPE) {

    set_buffer_parity_bits(text_frame.frame_buffer, 7 , text_frame.frame_length - TRAILER_LENGTH, HEADER_LENGTH);
    //set_eighth_parity_bits(frame_length);// set parity of last bit for all bytes except trailer(ie the checksums, which is dependant on the value of the bytes)
    set_verical_parity_byte(text_frame.frame_buffer ,text_frame.frame_length-3);
    set_checksum_13(generate_checksum_13(frame_type), frame_type); //macro to generate 13 bit checksum
  }

  else if (frame_type == POS_FRAME_TYPE) {

  }

  else if (frame_type == SENSOR_FRAME_TYPE) {

  }

  else if (frame_type == MENU_FRAME_TYPE) {

  }

  else if (frame_type == PING_STRING_TYPE) {

  }
  else return;
#endif
}

void Coms::set_header_parity(byte frame_type) {

  switch (frame_type) {
    case TEXT_FRAME_TYPE:
      text_frame.frame_buffer[0] = (text_frame.frame_buffer[0] << 1) | (parity_of(text_frame.frame_buffer[0]));
      text_frame.frame_buffer[1] = (text_frame.frame_buffer[1] << 1) | (parity_of(text_frame.frame_buffer[1]));
      text_frame.frame_buffer[2] = (text_frame.frame_buffer[2]       | (parity_of(GET_FRAME_NUM_DATA(text_frame.frame_buffer[2])) << 4)); //isolate three data bits, get parity, move parity in correct loc
      text_frame.frame_buffer[2] = (text_frame.frame_buffer[2]       | (parity_of(GET_THIS_FRAME_DATA( text_frame.frame_buffer[2] ))));
      text_frame.frame_buffer[3] = (text_frame.frame_buffer[3]       | (parity_of(text_frame.frame_buffer[3])));
      break;
    case POS_FRAME_TYPE:
      pos_frame.frame_buffer[0] = (pos_frame.frame_buffer[0] << 1) | (parity_of(pos_frame.frame_buffer[0]));
      pos_frame.frame_buffer[1] = (pos_frame.frame_buffer[1] << 1) | (parity_of(pos_frame.frame_buffer[1]));
      pos_frame.frame_buffer[2] = (pos_frame.frame_buffer[2]       | (parity_of(GET_FRAME_NUM_DATA(pos_frame.frame_buffer[2])) << 4));
      pos_frame.frame_buffer[2] = (pos_frame.frame_buffer[2]       | (parity_of(GET_THIS_FRAME_DATA( pos_frame.frame_buffer[2] ))));
      pos_frame.frame_buffer[3] = (pos_frame.frame_buffer[3] << 1) | (parity_of(pos_frame.frame_buffer[3]));
      break;

    case SENSOR_FRAME_TYPE:
      sensor_data_frame.frame_buffer[0] = (sensor_data_frame.frame_buffer[0] << 1) | (parity_of(sensor_data_frame.frame_buffer[0]));
      sensor_data_frame.frame_buffer[1] = (sensor_data_frame.frame_buffer[1] << 1) | (parity_of(sensor_data_frame.frame_buffer[1]));
      sensor_data_frame.frame_buffer[2] = (sensor_data_frame.frame_buffer[2]       | (parity_of(GET_FRAME_NUM_DATA(sensor_data_frame.frame_buffer[2])) << 4));
      sensor_data_frame.frame_buffer[2] = (sensor_data_frame.frame_buffer[2]       | (parity_of(GET_THIS_FRAME_DATA( sensor_data_frame.frame_buffer[2] ))));
      sensor_data_frame.frame_buffer[3] = (sensor_data_frame.frame_buffer[3] << 1) | (parity_of(sensor_data_frame.frame_buffer[3]));
      break;

    case MENU_FRAME_TYPE:
      menu_frame.frame_buffer[0] = (menu_frame.frame_buffer[0] << 1) | (parity_of(menu_frame.frame_buffer[0]));
      menu_frame.frame_buffer[1] = (menu_frame.frame_buffer[1] << 1) | (parity_of(menu_frame.frame_buffer[1]));
      menu_frame.frame_buffer[2] = (menu_frame.frame_buffer[2]       | (parity_of(GET_FRAME_NUM_DATA(menu_frame.frame_buffer[2])) << 4));
      menu_frame.frame_buffer[2] = (menu_frame.frame_buffer[2]       | (parity_of(GET_THIS_FRAME_DATA( menu_frame.frame_buffer[2] ))));
      menu_frame.frame_buffer[3] = (menu_frame.frame_buffer[3] << 1) | (parity_of(menu_frame.frame_buffer[3]));
      break;

    case PING_STRING_TYPE:
      ping_frame.frame_buffer[0] = (ping_frame.frame_buffer[0] << 1) | (parity_of(ping_frame.frame_buffer[0]));
      ping_frame.frame_buffer[1] = (ping_frame.frame_buffer[1] << 1) | (parity_of(ping_frame.frame_buffer[1]));
      ping_frame.frame_buffer[2] = (ping_frame.frame_buffer[2]       | (parity_of(GET_FRAME_NUM_DATA(ping_frame.frame_buffer[2])) << 4));
      ping_frame.frame_buffer[2] = (ping_frame.frame_buffer[2]       | (parity_of(GET_THIS_FRAME_DATA( ping_frame.frame_buffer[2] ))));
      ping_frame.frame_buffer[3] = (ping_frame.frame_buffer[3] << 1) | (parity_of(ping_frame.frame_buffer[3]));
      break;

    default:
      Serial.println("header parity calc error");
      return;
  }
}

inline byte Coms::parity_of(byte value) {
  return parity[value];   //LUT of parity given up to 8 bit value
}



void Coms::set_buffer_parity_bits(byte *buf, byte bit_loc, int buf_length, int start_from) { // set parity of last bit for all bytes excpet last two(ie the checksums, which is dependant on the value of the bytes)

  //loc = 0 parity bit is MSB
  //loc = 7 parity bit is LSB


//NB:  logic not working for non MSB parity bit

  byte loc_bit_suppress_mask = 0;
  switch (bit_loc) {
    case 0:
      loc_bit_suppress_mask = 0x7F;
      break;
    case 1:
      loc_bit_suppress_mask = 0xBF;
      break;
    case 2:
      loc_bit_suppress_mask = 0xDF;
      break;
    case 3:
      loc_bit_suppress_mask  = 0xEF;
      break;
    case 4:
      loc_bit_suppress_mask = 0xF7;
      break;
    case 5:
      loc_bit_suppress_mask = 0xFB;
      break;
    case 6:
      loc_bit_suppress_mask = 0xFD;
      break;
    case 7:
      loc_bit_suppress_mask = 0xFE;
      break;
    default: return;
  }
  byte suppress_data_mask = ~ loc_bit_suppress_mask; //is inverse of suppress mask
  byte shift_bit_by = 7 - bit_loc;  //locate bit in byte for parity
  byte shift_data_by =1;  //hard code data shift, change this to split data and shift in two parts to allow parity to be located anywhere
  
  for (int i = start_from; i < buf_length; i++) {
//    Serial.print("before :");
//    Serial.println(buf[i],BIN);
    buf[i] = ((buf[i]<<shift_data_by)  & loc_bit_suppress_mask) | (parity_of(buf[i]) & suppress_data_mask);
//    Serial.print("after  :");
//    Serial.println(buf[i],BIN);
//    Serial.println();
  }
}



void Coms::set_verical_parity_byte(byte *buf, int checksum_loc,int start_byte) {

  byte count =0;
  byte mask =1;
  for(byte j =0; j<8 ;j++){
    //byte mask = 1<<j
  for (int i = start_byte; i< checksum_loc;i++){
    count+= ((buf[i]>>j)&mask); 
  }
  buf[checksum_loc] = buf[checksum_loc] | ((count & mask)<<j);
  }

}


void Coms::set_checksum_13(uint16_t checksum, byte frame_type) {

  byte three_bit = ((checksum >> 7) & 0b00001110);
  byte eight_bit = (checksum & 0xFF);
  byte disp_from_frame_end = 2;
  switch (frame_type) {
    case TEXT_FRAME_TYPE:
      text_frame.frame_buffer[CHECKSUM_3_BIT_LOC] = (text_frame.frame_buffer[CHECKSUM_3_BIT_LOC] | (three_bit));
      text_frame.frame_buffer[text_frame.frame_length - disp_from_frame_end] = eight_bit;
      break;
    case POS_FRAME_TYPE:
      pos_frame.frame_buffer[CHECKSUM_3_BIT_LOC] = (pos_frame.frame_buffer[CHECKSUM_3_BIT_LOC] | (three_bit));
      pos_frame.frame_buffer[pos_frame.frame_length - disp_from_frame_end] = eight_bit;
      break;
    case SENSOR_FRAME_TYPE:
      sensor_data_frame.frame_buffer[CHECKSUM_3_BIT_LOC] = (sensor_data_frame.frame_buffer[CHECKSUM_3_BIT_LOC] | (three_bit));
      sensor_data_frame.frame_buffer[sensor_data_frame.frame_length - disp_from_frame_end] = eight_bit;
      break;
    case MENU_FRAME_TYPE:
      menu_frame.frame_buffer[CHECKSUM_3_BIT_LOC] = (menu_frame.frame_buffer[CHECKSUM_3_BIT_LOC] | (three_bit));
      menu_frame.frame_buffer[menu_frame.frame_length - disp_from_frame_end] = eight_bit;
      break;
    case PING_STRING_TYPE:
      ping_frame.frame_buffer[CHECKSUM_3_BIT_LOC] = (ping_frame.frame_buffer[CHECKSUM_3_BIT_LOC] | (three_bit));
      ping_frame.frame_buffer[ping_frame.frame_length - 2] = eight_bit;
      break;
    default:
      Serial.println("set checksum error");
      return;

  }
}
#endif // Coms_CPP
