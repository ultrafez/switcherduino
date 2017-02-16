// Based on code at http://www.fanjita.org/serendipity/archives/53-Interfacing-with-radio-controlled-mains-sockets-part-2.html

#include "MaplinCtrl.h"

const int MaplinCtrl::kPayloadSize = 48;
const int MaplinCtrl::kPulseWidthSmall = 400;

// Button ID (payload1) values.  There are 4 values for 4 channels, organised as
// ch1_btn1, ch1_btn2, ch1_btn3, ch1_btn4, ch2_btn1, etc.
const long MaplinCtrl::buttons[16] = {
  859124533L,
  861090613L,
  892547893L,
  1395864373L,
  859124563L,
  861090643L,
  892547923L,
  1395864403L,
  859125043L,
  861091123L,
  892548403L,
  1395864883L,
  859132723L,
  861098803L,
  892556083L,
  1395872563L  
};

MaplinCtrl::MaplinCtrl(unsigned int tx_pin, unsigned int led_pin) {
    led_pin_ = led_pin;
    tx_pin_ = tx_pin;

    pinMode(tx_pin_, OUTPUT);
    pinMode(led_pin_, OUTPUT);
}

void MaplinCtrl::deviceOn(unsigned int channel_id, unsigned int button_id) {
    simulateButton(channel_id, button_id, 1);
}

void MaplinCtrl::deviceOff(unsigned int channel_id, unsigned int button_id) {
    simulateButton(channel_id, button_id, 1);
}

// RADIO MUST BE ON!
void MaplinCtrl::simulateButton(int channel, int button, int on) {
    simulateButton((channel - 1) * 4 + (button - 1), on);
}

// RADIO MUST BE ON!
void MaplinCtrl::simulateButton(int device, int on) {
  long payload1 = buttons[device];
  long payload2 = on ? 13107L : 21299L;

  // Send the data twice - once doesn't seem to be enough
  sendData(payload1, payload2);
  sendData(payload1, payload2);
  sendData(payload1, payload2);
}

// RADIO MUST BE ON!
void MaplinCtrl::sendData(long payload1, long payload2) {
  // Reset radio status
  digitalWrite(tx_pin_, HIGH);
  digitalWrite(led_pin_, LOW); // on a Pro Micro, LED low = on
  
  // Send a preamble of 12.4 ms low pulse
  digitalWrite(tx_pin_, LOW);
  for (int ii = 0; ii < 31; ii++) {
    delayMicroseconds(kPulseWidthSmall);
  }
  digitalWrite(led_pin_, HIGH);
  
  // send sync pulse : high for 0.4 ms
  digitalWrite(tx_pin_, HIGH);
  delayMicroseconds(kPulseWidthSmall);
  digitalWrite(tx_pin_, LOW);
  
  // Now send the digits.  
  // We send a 1 as a state change for 1.5ms, and a 0 as a state change for 0.5ms
  long mask = 1;
  char state = HIGH;
  long payload = payload1;
  for (int jj = 0; jj < kPayloadSize; jj++) {
    if (jj == 32) {
      payload = payload2;
      mask = 1;
    }
    
    char bit = (payload & mask) ? 1 : 0;
    mask <<= 1;
      
    state = !state;
    digitalWrite(tx_pin_, state);
  
    delayMicroseconds(kPulseWidthSmall);  
    if (bit) {
      delayMicroseconds(kPulseWidthSmall);  
      delayMicroseconds(kPulseWidthSmall);  
    }
  }
}
