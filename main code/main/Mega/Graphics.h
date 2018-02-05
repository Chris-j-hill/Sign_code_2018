

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "Local_Config.h"
#include "Arduino.h"


#define ASCII_CHARACTER_BASIC_WIDTH 6
#define ASCII_CHARACTER_BASIC_HEIGHT 9

#define POS_ISR_FREQUENCY 100    // 100 gives maximum speed 50 pixels/second, minimum 0.2 pixels/second (100/128)/2
                                 // Note: There may be severe stability issues with this at high speeds if delay 
                                 // correction is used due to overflows and amount the couner is incremented



// Similar to F(), but for PROGMEM string pointers rather than literals
#define F2(progmem_ptr) (const __FlashStringHelper *)progmem_ptr


struct Text_Struct{
  byte string [MAX_TWEET_SIZE];
  byte text_size = DEFAULT_TEXT_SIZE;
  byte colour_r = DEFAULT_TEXT_RED_BRIGHTNESS;
  byte colour_g = DEFAULT_TEXT_RED_BRIGHTNESS;
  byte colour_b = DEFAULT_TEXT_RED_BRIGHTNESS;
  byte hue = 0;
  bool use_hue = false;
  
  byte x_min =0; //region to allow text to be displayed
  byte x_max = SINGLE_MATRIX_WIDTH;
  bool hard_limit = false;
  bool limit_enabled = false;
};

struct Cursor_Struct{
  int global_x_pos = 0;   // position as recieved in frame
  int local_x_pos =0;     // relative position for this matrix
  int global_y_pos = 0; 
  int local_y_pos =0;     
  int x_dir = 0;         // direction and speed of text in x and y direction
  int y_dir = 0;   
  int local_min_x_pos = 0;  //how far the text will scroll off to the left 
  int local_max_x_pos = 0;  //how far the text will start to the right
  int local_min_y_pos = 0;  //how far the text will scroll up
  int local_max_y_pos = 0;  //how far the text will start down    
};

struct Screen_Struct{
  byte brightness =0;
  byte mode = 0;
  byte node_address =0;
};

struct Object_Struct_Polygons{   //colours of thing that isnt text

byte red =0;
byte green =0;
byte blue =0;
bool enabled = false;
int16_t x_pos = 0;      //coordinates of object
int16_t y_pos = 0;
byte size1=0;
byte size2=0;     
};

struct Object_Struct_Circles{   //colours of thing that isnt text

byte red =0;
byte green =0;
byte blue =0;
bool enabled = false;
int16_t x_pos = 0;      //coordinates of object
int16_t y_pos = 0;
byte radius=0;

};

void pos_update_ISR();    //ISR for updating the cursor position
// this ISR runs fast and updates a counter. Once the counter has 
// reached an overflow value it is reset, a pulse is send back to 
// the due and the position is incremented by the value x_dir and y_dir


class Graphics{

private:

    int pos_isr_period();
    void pos_isr_counter_overflow();
    byte menu_pixels_right_of_node();
public:
    Graphics(){};
    int init_matrix();
    int init_matrix(int address);
    void increment_cursor_position(byte axis);
    void attach_pos_ISR();  
    void delay_pos_ISR(int value, byte counter); // advance or delay the counter based on value from due  
    void set_text_min();
    void set_text_max();

    
    void set_object_colour(byte new_r, byte new_g, byte new_b);
    void draw_ring(byte x_center, byte y_center, uint16_t radius);         //global x/y coordinates

    void draw_objects(); // draw all enabled objects
    void set_text_colour(byte new_r, byte new_g, byte new_b);
    void draw_text();
    void draw_text_restricted_width(byte x_min, byte x_max, bool hard_limit);   // draw text from min to max. hard_limit is used to prevent anything being written beyond limits if true
                                                                            // full characters will be completed beyond limits if hard_limit is false 

    void init_menu_title_colour();    //since common struct cant define defualt there, needs seperate function
    void init_menu_option_colour();
    void write_title(byte title);
    void set_title_colour();  //set the colour as the title colour
    void write_menu_option(byte first, byte second, byte third, byte line_config);   //NB: line_config = 1 top line blank -> = 2 all filled -> = 3 bottom blank
    void set_menu_colour();

    void write_adjustment_menu(byte item);
    void write_enable_menu_item(byte item);   // animation to confirm click on enable/disable menu items   
    byte non_linear_startup_function(uint16_t x);

    void draw_background();   //if menu visible, draw partial background if screen not entirely covered
    void clear_area(byte top_left_x, byte top_left_y, byte bottom_right_x, byte bottom_right_y);  // clear any data in this part of the matrix, useful for clearing menu but not background etc
//    int set_refresh_rate(int rate);  //refresh rate of matrix library ( modify library to access this value)
};

#endif // GRAPHICS_H
