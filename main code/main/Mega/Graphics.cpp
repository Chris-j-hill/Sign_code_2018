

#ifndef GRAPHICS_CPP
#define GRAPHICS_CPP

#include "Graphics.h"
#include "Mega_Pins.h"
#include "libs/Timer3/TimerThree.h"
#include "Menus.h"

#ifdef USE_CUSTOM_RGB_MATRIX_LIBRARY
#include "libs/RGBMatrixPanel/RGBmatrixPanel.h"
#else
#include "RGBmatrixPanel.h"
#endif


Text_Struct text_parameters;
Screen_Struct screen_parameters;
Cursor_Struct cursor_parameters;
Object_Struct_Circles startup_ring;   //general purpose object parameters, so as not to overwrite text parameters when displaying several things at once
Object_Struct_Polygons menu_select_bar;
extern struct Menu_tree_items menu_items;

extern struct Menu_colour_struct    menu_option_colour;
extern struct Menu_colour_struct    title_colour;
extern struct Menu_Struct           menu_parameters;

extern Graphics graphics;
extern RGBmatrixPanel matrix;

volatile byte x_pos_ISR_counter = 0;
volatile byte x_pos_isr_counter_overflow = 255;

volatile byte y_pos_ISR_counter = 0;
volatile byte y_pos_isr_counter_overflow = 255;

volatile bool suppress_x_pos_update = false;
volatile bool suppress_y_pos_update = false;

extern byte menu_width;


void pos_update_ISR() {   //ISR for updating the cursor position
  // this ISR runs fast and updates two counters. Once a counter has
  // reached an overflow value it is reset, a pulse is sent back to
  // the due and the position is incremented. this allows the cursor
  // speed to be anywhere between POS_ISR_FREQUENCY pixels/second and
  // POS_ISR_FREQUENCY/128 pixels/second


  x_pos_ISR_counter++;
  y_pos_ISR_counter++;

  if (x_pos_ISR_counter == x_pos_isr_counter_overflow && !suppress_x_pos_update) {
    x_pos_ISR_counter = 0;

#ifdef DELAY_FEEBDACK_PINS
    pinMode (DELAY_FEEDBACK_X_PIN, OUTPUT);   //drive line low on position update
    digitalWrite(DELAY_FEEDBACK_X_PIN, LOW);  //direct port manipulation?
#endif

    graphics.increment_cursor_position(1);

  }
  else if (x_pos_ISR_counter == x_pos_isr_counter_overflow && suppress_x_pos_update)
    suppress_x_pos_update = false;



  if (y_pos_ISR_counter == y_pos_isr_counter_overflow && !suppress_y_pos_update) {
    y_pos_ISR_counter = 0;

#ifdef DELAY_FEEBDACK_PINS
    pinMode (DELAY_FEEDBACK_X_PIN, OUTPUT);
    digitalWrite(DELAY_FEEDBACK_X_PIN, LOW);
#endif

    graphics.increment_cursor_position(2);


  }
  else if (y_pos_ISR_counter == y_pos_isr_counter_overflow && suppress_y_pos_update)
    suppress_y_pos_update = false;



#ifdef DELAY_FEEBDACK_PINS
  // set these as inputs to allow due to pull line high after pos update
  if (x_pos_ISR_counter == 1)
    pinMode(DELAY_FEEDBACK_X_PIN, INPUT);
  if (y_pos_ISR_counter == 1)
    pinMode(DELAY_FEEDBACK_Y_PIN, INPUT);
#endif
}


void Graphics::attach_pos_ISR() {
  int period = pos_isr_period();
  this -> pos_isr_counter_overflow();

  Timer3.attachInterrupt(pos_update_ISR, period);
  Timer3.start();
}

void Graphics::delay_pos_ISR(int value, byte counter) {

  if (counter == 1) { // increment x
    if (x_pos_ISR_counter + value < 0 ) { //decreasing counter and near 0
      value += x_pos_ISR_counter;
      x_pos_ISR_counter = x_pos_isr_counter_overflow + value;
      suppress_x_pos_update = true;
    }
    else if (x_pos_ISR_counter + value >= x_pos_isr_counter_overflow) { //account for overflow,also incrment cursor manually
      value -= (x_pos_isr_counter_overflow - x_pos_ISR_counter);
      x_pos_ISR_counter = value;
      this -> increment_cursor_position(1);
    }
    else
      x_pos_ISR_counter += value; // just move the
  }

  else if (counter == 2) { // increment y
    if (y_pos_ISR_counter + value < 0 ) { //decreasing counter and near 0
      value += y_pos_ISR_counter;
      y_pos_ISR_counter = y_pos_isr_counter_overflow + value;
      suppress_y_pos_update = true;
    }
    else if (y_pos_ISR_counter + value >= y_pos_isr_counter_overflow) { //account for overflow,also increment cursor manually
      value -= (y_pos_isr_counter_overflow - y_pos_ISR_counter);
      x_pos_ISR_counter = value;
      this -> increment_cursor_position(2);
    }
    else
      y_pos_ISR_counter += value; // just move the

  }
}

void Graphics::increment_cursor_position(byte axis) {

  if (axis == 1) {
    if (cursor_parameters.x_dir < 0) {
      cursor_parameters.global_x_pos --;
      cursor_parameters.local_x_pos --;
    }
    else if (cursor_parameters.x_dir > 0) {
      cursor_parameters.global_x_pos ++;
      cursor_parameters.local_x_pos ++;
    }
  }

  else if (axis == 2) {
    if (cursor_parameters.y_dir < 0) {
      cursor_parameters.global_y_pos --;
      cursor_parameters.local_y_pos --;
    }
    else if (cursor_parameters.y_dir > 0) {
      cursor_parameters.global_y_pos ++;
      cursor_parameters.local_y_pos ++;
    }
  }
}


int Graphics::pos_isr_period() {

  return (1000 / POS_ISR_FREQUENCY);

}

void Graphics::pos_isr_counter_overflow() {

  x_pos_isr_counter_overflow = abs(cursor_parameters.x_dir) * 2; // *2 doubles range to increase of overflow to improve achievable scroll speed
  y_pos_isr_counter_overflow = abs(cursor_parameters.y_dir) * 2;

}

void Graphics::set_text_min() {
  //how far to the left will the text scroll
  cursor_parameters.local_min_x_pos = -1 * (screen_parameters.node_address * SINGLE_MATRIX_WIDTH) * (text_parameters.text_size * ASCII_CHARACTER_BASIC_WIDTH);
}

void Graphics::set_text_max() {
  //how far to the left will the text scroll
  cursor_parameters.local_max_x_pos = screen_parameters.node_address * SINGLE_MATRIX_WIDTH;
}


void Graphics::draw_ring(byte x_center, byte y_center, uint16_t radius) {

  int16_t local_x;

  switch (screen_parameters.node_address) {
    case 0:   local_x = x_center; break;                    // global top left is this screens top left
    case 1:   local_x = x_center - SINGLE_MATRIX_WIDTH;     // global top left is -64
    case 2:   local_x = x_center - (2 * SINGLE_MATRIX_WIDTH); // global top left is -128
    case 3:   local_x = x_center - (3 * SINGLE_MATRIX_WIDTH);
  }

  matrix.drawCircle(local_x, y_center, radius, matrix.Color333(startup_ring.red, startup_ring.green, startup_ring.blue));

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
  matrix.setTextColor(matrix.Color333(title_colour.red, title_colour.green, title_colour.blue));
}


void Graphics::set_menu_colour() {
  matrix.setTextColor(matrix.Color333(menu_option_colour.red, menu_option_colour.green, menu_option_colour.blue));
}

byte Graphics::non_linear_startup_function(uint16_t x) {
  //  float a = 0.000000000000010658 //<- tiny effect, not included to improve speed
  float b = -0.151;
  float c = 0.0127;
  float d = -0.000066;
  return (byte) round(abs(b * x + c * (x * x) + d * (x * x * x))); // d*x^3 + c*x^2 + b*x + c

}


void Graphics::draw_background() {

  //  byte pixels_right_of_node = ((NUM_SCREENS - 1) - screen_parameters.node_address) * SINGLE_MATRIX_WIDTH; //number of pixels to the right of this screen

  if (menu_width - menu_pixels_right_of_node() < SINGLE_MATRIX_WIDTH) { // draw partial background
    this -> clear_area(SINGLE_MATRIX_WIDTH - (menu_width - menu_pixels_right_of_node()), 0, SINGLE_MATRIX_WIDTH, SINGLE_MATRIX_HEIGHT); // clear area for menu
    text_parameters.x_max = SINGLE_MATRIX_WIDTH - (menu_width - menu_pixels_right_of_node());
    text_parameters.hard_limit = false;
    text_parameters.limit_enabled = true;
  }
}

void Graphics::clear_area(byte top_left_x, byte top_left_y, byte bottom_right_x, byte bottom_right_y) {

  if (top_left_x == 0 && top_left_y == 0 && bottom_right_x == SINGLE_MATRIX_WIDTH && bottom_right_y == SINGLE_MATRIX_HEIGHT)
    matrix.fillScreen(0); //optimised method for filling the screen
  else
    matrix.fillRect(top_left_x, top_left_y, bottom_right_x, bottom_right_y, matrix.Color333(0, 0, 0));

}

void Graphics::write_title(byte title) {
  int center_of_menu = SINGLE_MATRIX_WIDTH - (menu_width / 2 - menu_pixels_right_of_node());

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
  for (int i = 1; i < 4; i++) { //loop through htree lines
    if (i == 1) {
      line_item = first;
      matrix.setCursor(SINGLE_MATRIX_WIDTH - (menu_width - menu_pixels_right_of_node()), ASCII_CHARACTER_BASIC_HEIGHT + 1);
    }
    else if (i == 2) {
      line_item = second;
      matrix.fillTriangle(MENU_POINTER_X0, MENU_POINTER_Y0, MENU_POINTER_X1, MENU_POINTER_Y1, MENU_POINTER_X2, MENU_POINTER_Y2, matrix.Color333(MENU_POINTER_COLOUR_R, MENU_POINTER_COLOUR_G, MENU_POINTER_COLOUR_B));
      matrix.setCursor(SINGLE_MATRIX_WIDTH - (menu_width - menu_pixels_right_of_node()) + 6, 2 * ASCII_CHARACTER_BASIC_HEIGHT + 1);
    }
    else if (i == 3) {
      line_item = third;
      matrix.setCursor(SINGLE_MATRIX_WIDTH - (menu_width - menu_pixels_right_of_node()), 3 * ASCII_CHARACTER_BASIC_HEIGHT + 1);
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

      case SCREEN_MODE_0:               matrix.println(F2(menu_items.screen_mode0));                break;
      case SCREEN_MODE_1:               matrix.println(F2(menu_items.screen_mode1));                break;
      case SCREEN_MODE_2:               matrix.println(F2(menu_items.screen_mode2));                break;
      case SCREEN_MODE_3:               matrix.println(F2(menu_items.screen_mode3));                break;

      case TEXT_SIZE_MENU:              matrix.println(F2(menu_items.text_size_settings));          break;
      case TEXT_COLOUR_MENU:            matrix.println(F2(menu_items.text_colour_settings));        break;
      case SCROLL_SPEED_MENU:           matrix.println(F2(menu_items.scroll_speed_settings));       break;

      case FAN_SPEED_MENU:              matrix.println(F2(menu_items.fan_speed_settings));          break;
      case MIN_FAN_SPEED_MENU:          matrix.println(F2(menu_items.minimum_rotating_speed));      break;

      case SELECT_NETWORK_MANUALLY:     matrix.println(F2(menu_items.select_network_manually));     break;

      case SD_FOLDERS_MENU:             matrix.println(F2(menu_items.sd_card_folders));             break;

      case LED_STRIP_BRIGHTNESS_MENU:   matrix.println(F2(menu_items.led_strip_brightness));        break;

      case TEXT_COLOUR_RED:             matrix.println(F2(menu_items.text_colour_red));             break;
      case TEXT_COLOUR_GREEN:           matrix.println(F2(menu_items.text_colour_green));           break;
      case TEXT_COLOUR_BLUE:            matrix.println(F2(menu_items.text_colour_blue));            break;
      case TEXT_COLOUR_HUE:             matrix.println(F2(menu_items.text_colour_hue));             break;

      case NULL_STRING:                 matrix.println(F2(menu_items.text_colour_hue));             break;
      default:                          matrix.println(F2(menu_items.default_string));              break;

    }
  }
}

void Graphics::write_adjustment_menu(byte item) {

  int center_of_menu = SINGLE_MATRIX_WIDTH - (menu_width / 2 - menu_pixels_right_of_node());
  char buf[3] = {' ', ' ', ' '}; //to store converted byte
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
        itoa(text_parameters.text_size, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print("-" + (String)buf + "+");
        break;
      }

    case SCROLL_SPEED_MENU:
      {
        itoa(menu_parameters.scroll_speed, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print("-" + (String)buf + "+");
        break;
      }

    case FAN_SPEED_MENU:
      {
        itoa(menu_parameters.fan_speed, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print("-" + (String)buf + "+");
        break;
      }

    case MIN_FAN_SPEED_MENU:
      {
        itoa(menu_parameters.min_fan_speed, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print("-" + (String)buf + "+");
        break;
      }

    case LED_STRIP_BRIGHTNESS_MENU:
      {
        itoa(menu_parameters.led_strip_brightness, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print("-" + (String)buf + "+");
        break;
      }

    case TEXT_COLOUR_RED:
      {
        itoa(text_parameters.colour_r, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print("-" + (String)buf + "+");
        break;
      }

    case TEXT_COLOUR_GREEN:
      {
        itoa(text_parameters.colour_g, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print("-" + (String)buf + "+");
        break;
      }

    case TEXT_COLOUR_BLUE:
      {
        itoa(text_parameters.colour_b, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print("-" + (String)buf + "+");
        break;
      }

    case TEXT_COLOUR_HUE:
      {
        itoa(text_parameters.hue, buf, 10);
        matrix.setCursor(12, center_of_menu - (sizeof(buf)*ASCII_CHARACTER_BASIC_WIDTH) / 2);
        matrix.print("-" + (String)buf + "+");
        break;
      }
  }
}

void Graphics::write_enable_menu_item(byte item){}



byte Graphics::menu_pixels_right_of_node() {
  return ((NUM_SCREENS - 1) - screen_parameters.node_address) * SINGLE_MATRIX_WIDTH; //number of pixels to the right of this screen

}
#endif //GRAPHICS_CPP