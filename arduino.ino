
// See http://www.fanjita.org/serendipity/archives/53-Interfacing-with-radio-controlled-mains-sockets-part-2.html

#define PAYLOAD_SIZE 48

#define DATA_PIN  2
#define VCC_PIN   3
#define GND_PIN   4
#define LED_PIN   17

#define PULSE_WIDTH_SMALL  500

// Button ID (payload1) values.  There are 4 values for 4 channels, organised as
// ch1_btn1, ch1_btn2, ch1_btn3, ch1_btn4, ch2_btn1, etc.
long buttons[] = {
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

void setup()
{
  // Plug the TX module into A3-A5, with the antenna pin hanging off the end of the header.
  pinMode(GND_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(VCC_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  //Serial.begin(115200);
}

void sendData(long payload1, long payload2)
{
  // Turn on the radio. A3=GND, A5=Vcc, A4=data)
  digitalWrite(GND_PIN, LOW);
  digitalWrite(VCC_PIN, HIGH);
  digitalWrite(DATA_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);
  
  // Send a preamble of 13 ms low pulse
  digitalWrite(DATA_PIN, LOW);
  for (int ii = 0; ii < 26; ii++)
  {
    delayMicroseconds(PULSE_WIDTH_SMALL);
  }
  digitalWrite(LED_PIN, HIGH);
  
  // send sync pulse : high for 0.5 ms
  digitalWrite(DATA_PIN, HIGH);
  delayMicroseconds(PULSE_WIDTH_SMALL);
  digitalWrite(DATA_PIN, LOW);
  
  // Now send the digits.  
  // We send a 1 as a state change for 1.5ms, and a 0 as a state change for 0.5ms
  long mask = 1;
  char state = HIGH;
  long payload = payload1;
  for (int jj = 0; jj < PAYLOAD_SIZE; jj++)
  {
    if (jj == 32)
    {
      payload = payload2;
      mask = 1;
    }
    
    char bit = (payload & mask) ? 1 : 0;
    mask <<= 1;
      
    state = !state;
    digitalWrite(DATA_PIN, state);
  
    delayMicroseconds(PULSE_WIDTH_SMALL);  
    if (bit)
    {
      delayMicroseconds(PULSE_WIDTH_SMALL);  
      delayMicroseconds(PULSE_WIDTH_SMALL);  
    }
  }
}

void simulate_button(int channel, int button, int on)
{
  long payload1 = buttons[(channel - 1) * 4 + (button - 1)];
  long payload2 = on? 13107L : 21299L;
  
  //Serial.println(payload1);
  //Serial.println(payload2);

  // Send the data 6 times
  for (int ii = 0; ii < 6; ii++)
  {
    sendData(payload1, payload2);
  }
  
  // turn off the radio
  digitalWrite(VCC_PIN, LOW);
}

void loop()
{
//  digitalWrite(LED_PIN, LOW);
  simulate_button(3, 1, 1);
//  digitalWrite(LED_PIN, HIGH);
  delay(2000);
//  digitalWrite(LED_PIN, LOW);
  simulate_button(3, 1, 0);
//  digitalWrite(LED_PIN, HIGH);  
 
  delay(2000);
}
