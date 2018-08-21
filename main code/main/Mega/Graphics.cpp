

#ifndef GRAPHICS_CPP
#define GRAPHICS_CPP

#include "Graphics.h"
#include "Mega_Pins.h"
#include "src/Timer3/TimerThree.h"
#include "Menus.h"
#include "Local_Config.h"


#ifdef USE_CUSTOM_RGB_MATRIX_LIBRARY
#include "src/customRGBMatrixPanel/customRGBmatrixPanel.h"
#else
#include "RGBmatrixPanel.h"
#endif


Text_Struct text_parameters[MAX_NUM_OF_TEXT_OBJECTS];
Screen_Struct screen_parameters;
Cursor_Struct cursor_parameters[MAX_NUM_OF_TEXT_OBJECTS];
Object_Struct_Circles startup_ring;   //general purpose object parameters, so as not to overwrite text parameters when displaying several things at once

extern struct Menu_tree_items menu_items;

extern struct Menu_colour_struct    menu_option_colour;
extern struct Menu_colour_struct    title_colour;
extern struct Menu_Struct           menu_parameters;

extern Graphics graphics;
extern RGBmatrixPanel matrix;
extern Menu menu;

//volatile byte x_pos_ISR_counter = 0;
//volatile byte x_pos_isr_counter_overflow = 255;
//
//volatile byte y_pos_ISR_counter = 0;
//volatile byte y_pos_isr_counter_overflow = 255;

//volatile bool suppress_x_pos_update = false;
//volatile bool suppress_y_pos_update = false;

//extern byte menu_width;

extern bool menu_visible;

volatile bool interpolate_pos_flag = false;

const char plus_string[] PROGMEM = "+";
const char minus_string[] PROGMEM = "-";



void pos_update_ISR() {
  interpolate_pos_flag = true;
}


void Graphics::init_matrix() {
  matrix.begin();
  matrix.setTextWrap(false); // Allow text to run off right edge

  set_title_colour();
  set_menu_colour();
}


void Graphics::update_display() {

  static uint32_t time_since_last_update = millis();

  // only change buffers if something changed (and not too frequently, function can be slow, potientially filling a large buffer)
  // change can be only due to frame arrived

  //matrix.drawPixel(10, 10, matrix.Color444(10, 10, 10));

  //psudo state machine using menu_updated value -> 0 : menus equal, no change recently, 1 : just changed and screen not updated, 2 : screen updated but previous_menu not caught up
  static byte menu_updated = 0;

  //run these every time to detect change, v small chance of missed detection...
  if (menu.get_current_menu() != menu.get_previous_menu() && menu_updated == 0)
    menu_updated = 1;
  if (menu.get_current_menu() == menu.get_previous_menu() && menu_updated == 2)
    menu_updated = 0;

  if ((millis() > time_since_last_update + MIN_DISPLAY_UPDATE_PERIOD) && (!screen_parameters.updated || (menu_updated == 1))) {


    set_display_mode();

    if (menu.get_current_menu() == DEFAULT_MENU && !screen_parameters.updated) {

      for (byte i = 0; i < MAX_NUM_OF_TEXT_OBJECTS; i++) {
        if (text_parameters[i].object_used) {
          if (text_parameters[i].use_hue)
            graphics.set_text_colour(text_parameters[i].hue);
          else
            graphics.set_text_colour(text_parameters[i].colour_r, text_parameters[i].colour_g, text_parameters[i].colour_b);
          graphics.draw_text(i);
          Serial.println("draw");
        }
      }
    }

//    else if((menu.get_current_menu() != DEFAULT_MENU && menu.get_current_menu() != STARTUP_MENU) && !screen_parameters.updated) //<- partial coverage of menu...

    else// draw menu
    //after all objects written, cover with display if needed
    menu.display_menu();  //update any menu if applicable

//    matrix.drawPixel(10, 10, matrix.Color444(10, 0, 0));
//    for (byte i = 0; i < MAX_NUM_OF_TEXT_OBJECTS; i++) {
//      if (text_parameters[i].object_used)
//        matrix.drawPixel(11 + i, 10, matrix.Color444(10, 10, 10));
//      else
//        matrix.drawPixel(11 + i, 10, matrix.Color444(0, 0, 10));
//
//    }
    matrix.swapBuffers(false);  //push the buffer we just wrote to front


    //dealt with screen update
    screen_parameters.updated = true;

    // dealt with menu update, put into waiting state
    if (menu_updated == 1)
      menu_updated = 2;
  }

}

inline void Graphics::set_display_mode() {
  //only do this if mode was changed. mode method is quick so not much gian, but even so...
  if (screen_parameters.new_mode != screen_parameters.cur_mode) {
    screen_parameters.cur_mode = screen_parameters.new_mode;
    matrix.Mode(screen_parameters.new_mode);
  }
}

inline void Graphics::set_text_colour(byte new_r, byte new_g, byte new_b) {

  //limit colour range on off chance number is still wrong
  if (new_r > COLOUR_MAX_LEVEL)
    new_r = COLOUR_MAX_LEVEL;
  if (new_g > COLOUR_MAX_LEVEL)
    new_g = COLOUR_MAX_LEVEL;
  if (new_b > COLOUR_MAX_LEVEL)
    new_b = COLOUR_MAX_LEVEL;

  //scale colour value with brightness
  float multiplier = screen_parameters.brightness / 100;
  new_r = new_r * multiplier; //scale by this
  new_g = new_g * multiplier;
  new_b = new_b * multiplier;

#if defined(USING_COLOUR_SET_888)
  matrix.setTextColor(matrix.Color888(new_r, new_g, new_b));
#elif defined(USING_COLOUR_SET_444)
  matrix.setTextColor(matrix.Color444(new_r, new_g, new_b));
#else
  matrix.setTextColor(matrix.Color333(new_r, new_g, new_b));
#endif
}

inline void Graphics::set_text_colour(int new_hue) {

  if (new_hue > HUE_MAX_LEVEL)
    new_hue = HUE_MAX_LEVEL;
  else if (new_hue < HUE_MIN_LEVEL)
    new_hue = HUE_MIN_LEVEL;

  byte multiplier = map(screen_parameters.brightness, 0, 100, 0, 255);

  matrix.setTextColor(matrix.ColorHSV(new_hue, 255, multiplier, true));
}

inline void Graphics::draw_text(byte obj_num) {

  matrix.setTextSize(text_parameters[obj_num].text_size);
//  matrix.setCursor(cursor_parameters[obj_num].local_x_pos, cursor_parameters[obj_num].local_y_pos);
  matrix.setCursor(12,1);
  
  for (byte i = 0; i < MAX_TWEET_SIZE; i++) {
    if (text_parameters[obj_num].string[i] == 0) break;
    matrix.print((char)text_parameters[obj_num].string[i]);
  }

}



void Graphics::attach_pos_ISR() {

  uint16_t period = pos_isr_period();
  //  pos_isr_counter_overflow();

  Timer3.attachInterrupt(pos_update_ISR, period);
  Timer3.start();
}

void Graphics::interpolate_pos() {

  /* this function executes the required interpolation of the cursor
     based on the flag set inside the pos_update_isr funtion. this
     is reasonably slow with many divides so run outside of interrupts
     whenever processor isnt doing anything else
  */
  if (interpolate_pos_flag) {
    interpolate_pos_flag = false;
    uint32_t cur_time = millis(); //only need to do this once, millis isnt that fast...

    for (byte i = 0; i < MAX_NUM_OF_TEXT_OBJECTS; i++) {

      //check if period elapsed it is time to update
      if ((cur_time - cursor_parameters[i].isr_last_update_x_time) > cursor_parameters[i].time_between_increments_x) {
        cursor_parameters[i].isr_last_update_x_time = cur_time;

        //increment value, speed now irrelevant, since this interrupt should run the required num times per second, so just increment by one
        if (cursor_parameters[i].x_dir < 0)
          cursor_parameters[i].local_x_pos--;
        else if (cursor_parameters[i].x_dir > 0)
          cursor_parameters[i].local_x_pos++;
      }

      if ((cur_time - cursor_parameters[i].isr_last_update_y_time) > cursor_parameters[i].time_between_increments_y) {
        cursor_parameters[i].isr_last_update_y_time = cur_time;

        if (cursor_parameters[i].y_dir < 0)
          cursor_parameters[i].local_y_pos--;
        else if (cursor_parameters[i].y_dir > 0)
          cursor_parameters[i].local_y_pos++;
      }
    }
  }
}

//void Graphics::delay_pos_ISR(int value, byte counter) {
//
//  if (counter == 1) { // increment x
//    if (x_pos_ISR_counter + value < 0 ) { //decreasing counter and near 0
//      value += x_pos_ISR_counter;
//      x_pos_ISR_counter = x_pos_isr_counter_overflow + value;
//      suppress_x_pos_update = true;
//    }
//    else if (x_pos_ISR_counter + value >= x_pos_isr_counter_overflow) { //account for overflow,also incrment cursor manually
//      value -= (x_pos_isr_counter_overflow - x_pos_ISR_counter);
//      x_pos_ISR_counter = value;
//      this -> increment_cursor_position(1);
//    }
//    else
//      x_pos_ISR_counter += value; // just move the
//  }
//
//  else if (counter == 2) { // increment y
//    if (y_pos_ISR_counter + value < 0 ) { //decreasing counter and near 0
//      value += y_pos_ISR_counter;
//      y_pos_ISR_counter = y_pos_isr_counter_overflow + value;
//      suppress_y_pos_update = true;
//    }
//    else if (y_pos_ISR_counter + value >= y_pos_isr_counter_overflow) { //account for overflow,also increment cursor manually
//      value -= (y_pos_isr_counter_overflow - y_pos_ISR_counter);
//      x_pos_ISR_counter = value;
//      this -> increment_cursor_position(2);
//    }
//    else
//      y_pos_ISR_counter += value; // just move the
//
//  }
//}

void Graphics::increment_cursor_position(byte axis, byte obj_num) {

  if (axis == 1) {
    if (cursor_parameters[obj_num].x_dir < 0) {
      cursor_parameters[obj_num].global_x_pos --;
      cursor_parameters[obj_num].local_x_pos --;
    }
    else if (cursor_parameters[obj_num].x_dir > 0) {
      cursor_parameters[obj_num].global_x_pos ++;
      cursor_parameters[obj_num].local_x_pos ++;
    }
  }

  else if (axis == 2) {
    if (cursor_parameters[obj_num].y_dir < 0) {
      cursor_parameters[obj_num].global_y_pos --;
      cursor_parameters[obj_num].local_y_pos --;
    }
    else if (cursor_parameters[obj_num].y_dir > 0) {
      cursor_parameters[obj_num].global_y_pos ++;
      cursor_parameters[obj_num].local_y_pos ++;
    }
  }
}

inline uint16_t Graphics::pos_isr_period() {

  return (1000000 / POS_ISR_FREQUENCY);// number of microseconds between functions calls

}

//void Graphics::pos_isr_counter_overflow() {
//
//  x_pos_isr_counter_overflow = abs(cursor_parameters[0].x_dir) * 2; // *2 doubles range to increase of overflow to improve achievable scroll speed
//  y_pos_isr_counter_overflow = abs(cursor_parameters[0].y_dir) * 2;
//
//}


void Graphics::draw_ring(byte x_center, byte y_center, uint16_t radius) {

  int16_t local_x;

  switch (screen_parameters.node_address) {
    case 0:   local_x = x_center; break;                    // global top left is this screens top left
    case 1:   local_x = x_center - SINGLE_MATRIX_WIDTH;     // global top left is -64
    case 2:   local_x = x_center - (2 << SINGLE_MATRIX_WIDTH_AS_POW_2); // global top left is -128
    case 3:   local_x = x_center - (3 << SINGLE_MATRIX_WIDTH_AS_POW_2);
  }


#if  defined(USING_COLOUR_SET_888)
  matrix.drawCircle(local_x, y_center, radius, matrix.Color888(startup_ring.red, startup_ring.green, startup_ring.blue));
#elif defined(USING_COLOUR_SET_444)
  matrix.drawCircle(local_x, y_center, radius, matrix.Color444(startup_ring.red, startup_ring.green, startup_ring.blue));
#else
  matrix.drawCircle(local_x, y_center, radius, matrix.Color333(startup_ring.red, startup_ring.green, startup_ring.blue));
#endif
}

void Graphics::set_object_colour(byte new_r, byte new_g, byte new_b) {
  startup_ring.red = new_r;
  startup_ring.blue = new_b;
  startup_ring.green = new_g;
}


void Graphics::init_menu_title_colour() {
  title_colour.red = MENU_TITLE_R;
  title_colour.green = MENU_TITLE_G;
  title_colour.blue = MENU_TITLE_B;
}

void Graphics::init_menu_option_colour() {
  menu_option_colour.red = MENU_OPTION_R;
  menu_option_colour.green = MENU_OPTION_G;
  menu_option_colour.blue = MENU_OPTION_B;

}


void Graphics::set_title_colour() {

#if defined(USING_COLOUR_SET_888)
  matrix.setTextColor(matrix.Color888(title_colour.red, title_colour.green, title_colour.blue));
#elif defined(USING_COLOUR_SET_444)
  matrix.setTextColor(matrix.Color444(title_colour.red, title_colour.green, title_colour.blue));
#else
  matrix.setTextColor(matrix.Color333(title_colour.red, title_colour.green, title_colour.blue));
#endif
}


void Graphics::set_menu_colour() {

#if defined(USING_COLOUR_SET_888)
  matrix.setTextColor(matrix.Color888(menu_option_colour.red, menu_option_colour.green, menu_option_colour.blue));
#elif defined(USING_COLOUR_SET_444)
  matrix.setTextColor(matrix.Color444(menu_option_colour.red, menu_option_colour.green, menu_option_colour.blue));
#else
  matrix.setTextColor(matrix.Color333(menu_option_colour.red, menu_option_colour.green, menu_option_colour.blue));
#endif
}


void Graphics::set_menu_colour_highlighted() {

  byte delta = COLOUR_LEVEL_STEP_SIZE; //modify colour by this

#if defined(USING_COLOUR_SET_888)
  matrix.setTextColor(matrix.Color888(menu_option_colour.red + delta, menu_option_colour.green + delta, menu_option_colour.blue + delta));
#elif defined(USING_COLOUR_SET_444)
  matrix.setTextColor(matrix.Color444(menu_option_colour.red + delta, menu_option_colour.green + delta, menu_option_colour.blue + delta));
#else
  matrix.setTextColor(matrix.Color333(menu_option_colour.red + delta, menu_option_colour.green + delta, menu_option_colour.blue + delta));
#endif
}

byte Graphics::non_linear_startup_function(uint16_t x) {//<- i have no idea how i came up with this

  //  float a = 0.000000000000010658 //<- tiny effect, not included to improve speed
  float b = -0.151;
  float c = 0.0127;
  float d = -0.000066;
  return (byte) round(abs(b * x + c * (x * x) + d * (x * x * x))); // d*x^3 + c*x^2 + b*x + c

}


void Graphics::draw_background() {

  //  byte pixels_right_of_node = ((NUM_SCREENS - 1) - screen_parameters.node_address) * SINGLE_MATRIX_WIDTH; //number of pixels to the right of this screen

  if (DEFAULT_MENU_WIDTH - menu_pixels_right_of_node() < SINGLE_MATRIX_WIDTH) { // draw partial background
    this -> clear_area(SINGLE_MATRIX_WIDTH - (DEFAULT_MENU_WIDTH - menu_pixels_right_of_node()), 0, SINGLE_MATRIX_WIDTH, SINGLE_MATRIX_HEIGHT); // clear area for menu
    //    text_parameters[0].x_max = SINGLE_MATRIX_WIDTH - (DEFAULT_MENU_WIDTH - menu_pixels_right_of_node());
    //    text_parameters[0].hard_limit = false;
    //    text_parameters[0].limit_enabled = true;
  }
}

void Graphics::clear_area(byte top_left_x, byte top_left_y, byte bottom_right_x, byte bottom_right_y) {

  if (top_left_x == 0 && top_left_y == 0 && bottom_right_x == SINGLE_MATRIX_WIDTH && bottom_right_y == SINGLE_MATRIX_HEIGHT)
    matrix.fillScreen(0); //optimised method for filling the screen
  else

#if defined(USING_COLOUR_SET_888)
    matrix.fillRect(top_left_x, top_left_y, bottom_right_x, bottom_right_y, matrix.Color888(0, 0, 0));
#elif defined(USING_COLOUR_SET_444)
    matrix.fillRect(top_left_x, top_left_y, bottom_right_x, bottom_right_y, matrix.Color444(0, 0, 0));
#else
    matrix.fillRect(top_left_x, top_left_y, bottom_right_x, bottom_right_y, matrix.Color333(0, 0, 0));
#endif

}

void Graphics::write_title(byte title) {
  int center_of_menu = SINGLE_MATRIX_WIDTH - ((DEFAULT_MENU_WIDTH >> 1) - menu_pixels_right_of_node());

  set_title_colour();
  switch (title) {
    case MAIN_MENU:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.main_menu)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.main_menu));
        break;
      }

    case SCREEN_MODE_MENU:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.screen_mode_menu)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.screen_mode_menu));
        break;
      }

    case BRIGHTNESS_MENU:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.brightness_menu)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.brightness_menu));
        break;
      }

    case TEXT_SETTINGS_MENU:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.text_settings_menu)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.text_settings_menu));
        break;
      }

    case FAN_SETTINGS_MENU:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.fan_settings_menu)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.fan_settings_menu));
        break;
      }

    case INTERNET_CONFIG_MENU:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.internet_config_menu)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.internet_config_menu));
        break;
      }

    case SD_CARD_MENU:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.sd_cards_menu)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.sd_cards_menu));
        break;
      }

    case LED_STRIP_MENU:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.led_strip_menu)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.led_strip_menu));
        break;
      }

    case TEXT_SIZE_MENU:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.text_size_menu)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.text_size_menu));
        break;
      }

    case TEXT_COLOUR_MENU:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.text_colour_menu)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.text_colour_menu));
        break;
      }

    case TEXT_COLOUR_RED:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.text_colour_red)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.text_colour_red));
        break;
      }

    case TEXT_COLOUR_GREEN:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.text_colour_green)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.text_colour_green));
        break;
      }

    case TEXT_COLOUR_BLUE:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.text_colour_blue)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.text_colour_blue));
        break;
      }

    case TEXT_COLOUR_HUE:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.text_colour_hue)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.text_colour_hue));
        break;
      }

    case SCROLL_SPEED_MENU:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.fan_speed_settings)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.fan_speed_settings));
        break;
      }

    case MIN_FAN_SPEED_MENU:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.minimum_fan_speed_menu)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.minimum_fan_speed_menu));
        break;
      }

    case SD_FOLDERS_MENU:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.sd_card_folders)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.sd_card_folders));
        break;
      }

    case LED_STRIP_BRIGHTNESS_MENU:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.led_strip_brightness)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.led_strip_brightness));
        break;
      }
    default:
      {
        matrix.setCursor(1, center_of_menu - (sizeof(menu_items.default_title)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.println(F2(menu_items.default_title));
        break;
      }
  }
}

void Graphics::write_menu_option(byte first, byte second, byte third, byte line_config) {  //NB: line_config = 1 top line blank -> = 2 all filled -> = 3 bottom blank
  byte line_item = 255;
  for (byte i = 1; i < 4; i++) { //loop through three lines
    if (i == 1) {
      line_item = first;
      matrix.setCursor(SINGLE_MATRIX_WIDTH - (DEFAULT_MENU_WIDTH - menu_pixels_right_of_node()), ASCII_CHARACTER_BASIC_HEIGHT + 1);
    }
    else if (i == 2) {
      line_item = second;
      matrix.fillTriangle(MENU_POINTER_X0, MENU_POINTER_Y0, MENU_POINTER_X1, MENU_POINTER_Y1, MENU_POINTER_X2, MENU_POINTER_Y2, matrix.Color333(MENU_POINTER_COLOUR_R, MENU_POINTER_COLOUR_G, MENU_POINTER_COLOUR_B));
      matrix.setCursor(SINGLE_MATRIX_WIDTH - (DEFAULT_MENU_WIDTH - menu_pixels_right_of_node()) + 6, (ASCII_CHARACTER_BASIC_HEIGHT << 1) + 1);
    }
    else if (i == 3) {
      line_item = third;
      matrix.setCursor(SINGLE_MATRIX_WIDTH - (DEFAULT_MENU_WIDTH - menu_pixels_right_of_node()), 3 * ASCII_CHARACTER_BASIC_HEIGHT + 1);
    }

    set_menu_colour();

    switch (line_item) {
      case RETURN_MENU:                 matrix.println(F2(menu_items.RETURN));                      break;
      case SCREEN_MODE_MENU:            matrix.println(F2(menu_items.screen_mode));                 break;
      case BRIGHTNESS_MENU:             matrix.println(F2(menu_items.brightness));                  break;
      case TEXT_SETTINGS_MENU:          matrix.println(F2(menu_items.text_settings));               break;
      case FAN_SETTINGS_MENU:           matrix.println(F2(menu_items.fan_settings));                break;
      case INTERNET_CONFIG_MENU:        matrix.println(F2(menu_items.internet_settings));           break;
      case SD_CARD_MENU:                matrix.println(F2(menu_items.sd_card_settings));            break;
      case LED_STRIP_MENU:              matrix.println(F2(menu_items.led_strip_settings));          break;

      case SCREEN_MODE_0:
        if (screen_parameters.cur_mode == 0) //if the item being displayed is the current mode, change its colour a bit to identify it
          set_menu_colour_highlighted();
        matrix.println(F2(menu_items.screen_mode0));
        break;

      case SCREEN_MODE_1:
        if (screen_parameters.cur_mode == 1) //if the item being displayed is the current mode, change its colour a bit to identify it
          set_menu_colour_highlighted();
        matrix.println(F2(menu_items.screen_mode1));
        break;

      case SCREEN_MODE_2:
        if (screen_parameters.cur_mode == 2) //if the item being displayed is the current mode, change its colour a bit to identify it
          set_menu_colour_highlighted();
        matrix.println(F2(menu_items.screen_mode2));
        break;

      case SCREEN_MODE_3:
        if (screen_parameters.cur_mode == 3) //if the item being displayed is the current mode, change its colour a bit to identify it
          set_menu_colour_highlighted();
        matrix.println(F2(menu_items.screen_mode3));
        break;

      case TEXT_SIZE_MENU:              matrix.println(F2(menu_items.text_size_settings));          break;
      case TEXT_COLOUR_MENU:            matrix.println(F2(menu_items.text_colour_settings));        break;
      case SCROLL_SPEED_MENU:           matrix.println(F2(menu_items.scroll_speed_settings));       break;

      case FAN_SPEED_MENU:              matrix.println(F2(menu_items.fan_speed_settings));          break;
      case ENABLE_FAN:                  matrix.println(F2(menu_items.fan_enable));                  break;
      case DISABLE_FAN:                 matrix.println(F2(menu_items.fan_disable));                 break;
      case MIN_FAN_SPEED_MENU:          matrix.println(F2(menu_items.minimum_rotating_speed));      break;

      case SELECT_NETWORK_MANUALLY:     matrix.println(F2(menu_items.select_network_manually));     break;

      case SD_FOLDERS_MENU:             matrix.println(F2(menu_items.sd_card_folders));             break;

      case LED_STRIP_BRIGHTNESS_MENU:   matrix.println(F2(menu_items.led_strip_brightness));        break;

      case TEXT_COLOUR_RED:             matrix.println(F2(menu_items.text_colour_red));             break;
      case TEXT_COLOUR_GREEN:           matrix.println(F2(menu_items.text_colour_green));           break;
      case TEXT_COLOUR_BLUE:            matrix.println(F2(menu_items.text_colour_blue));            break;
      case TEXT_COLOUR_HUE:             matrix.println(F2(menu_items.text_colour_hue));             break;






      case NULL_STRING:                 matrix.println(F2(menu_items.null_string));                 break;
      default:                          matrix.println(F2(menu_items.default_string));              break;

    }
  }
}

void Graphics::write_adjustment_menu(byte item, byte obj_num) {

  int center_of_menu = SINGLE_MATRIX_WIDTH - (DEFAULT_MENU_WIDTH >> 1 - menu_pixels_right_of_node());
  char buf[3] = {' '}; //to store converted byte
  byte val;
  switch (item) {
    case BRIGHTNESS_MENU:
      {
        itoa(screen_parameters.brightness, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print("-" + (String)buf + "+");
        break;
      }

    case TEXT_SIZE_MENU:
      {
        itoa(text_parameters[obj_num].text_size, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print("-" + (String)buf + "+");
        break;
      }

    case SCROLL_SPEED_MENU:
      {
        itoa(cursor_parameters[obj_num].x_dir, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print(minus_string + (String)buf + plus_string);
        break;
      }

    case FAN_SPEED_MENU:
      {
        itoa(menu_parameters.fan_speed, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print(minus_string + (String)buf + plus_string);
        break;
      }

    case MIN_FAN_SPEED_MENU:
      {
        itoa(menu_parameters.min_fan_speed, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print(minus_string + (String)buf + plus_string);
        break;
      }

    case LED_STRIP_BRIGHTNESS_MENU:
      {
        itoa(menu_parameters.led_strip_brightness, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print(minus_string + (String)buf + plus_string);
        break;
      }

    case TEXT_COLOUR_RED:
      {
        itoa(text_parameters[obj_num].colour_r, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print(minus_string + (String)buf + plus_string);
        break;
      }

    case TEXT_COLOUR_GREEN:
      {
        itoa(text_parameters[obj_num].colour_g, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print(minus_string + (String)buf + plus_string);
        break;
      }

    case TEXT_COLOUR_BLUE:
      {
        itoa(text_parameters[obj_num].colour_b, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print(minus_string + (String)buf + plus_string);
        break;
      }

    case TEXT_COLOUR_HUE:
      {
        itoa(text_parameters[obj_num].hue, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print(minus_string + (String)buf + plus_string);
        break;
      }
  }
}

void Graphics::write_enable_menu_item(byte item) {}



byte Graphics::menu_pixels_right_of_node() {
  return ((NUM_SCREENS - 1) - screen_parameters.node_address) * SINGLE_MATRIX_WIDTH; //number of pixels to the right of this screen

}
#endif //GRAPHICS_CPP
