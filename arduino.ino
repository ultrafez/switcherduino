// Based on code at http://www.fanjita.org/serendipity/archives/53-Interfacing-with-radio-controlled-mains-sockets-part-2.html

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

void setup() {
  pinMode(GND_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(VCC_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Radio pin setup
  digitalWrite(GND_PIN, LOW);

  setupFSM();
  
  Serial.begin(115200);
  Serial.println("Maplin remote socket controller. Send commands in the regex format rs([1-4][1-4][01])*\\n to turn on/off sockets in channel 1-4 and button 1-4");
}

void sendData(long payload1, long payload2) {
  // Turn on the radio.
  digitalWrite(VCC_PIN, HIGH);
  digitalWrite(DATA_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);
  
  // Send a preamble of 13 ms low pulse
  digitalWrite(DATA_PIN, LOW);
  for (int ii = 0; ii < 26; ii++) {
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
  for (int jj = 0; jj < PAYLOAD_SIZE; jj++) {
    if (jj == 32) {
      payload = payload2;
      mask = 1;
    }
    
    char bit = (payload & mask) ? 1 : 0;
    mask <<= 1;
      
    state = !state;
    digitalWrite(DATA_PIN, state);
  
    delayMicroseconds(PULSE_WIDTH_SMALL);  
    if (bit) {
      delayMicroseconds(PULSE_WIDTH_SMALL);  
      delayMicroseconds(PULSE_WIDTH_SMALL);  
    }
  }
}

void simulateButton(int channel, int button, int on) {
  long payload1 = buttons[(channel - 1) * 4 + (button - 1)];
  long payload2 = on ? 13107L : 21299L;

  // Send the data 6 times
  for (int ii = 0; ii < 6; ii++) {
    sendData(payload1, payload2);
  }
  
  // turn off the radio
  digitalWrite(VCC_PIN, LOW);
}

void loop() {
  // Valid serial string: rs131\r
  // Read and handle serial data if some has arrived
  if (Serial.available()) {
    updateFSM(Serial.read());
  }
}


/* FSM */

// States
const int STATE_WAIT_FOR_R = 1;
const int STATE_WAIT_FOR_S = 2;
const int STATE_READY_FOR_CHANNEL_OR_EXECUTE = 3;
const int STATE_WAIT_FOR_BUTTON = 4;
const int STATE_WAIT_FOR_ONOFF = 5;

// Machine stored data
int currentState = STATE_WAIT_FOR_R;
int chosenChannel = 0;
int chosenButton = 0;

// store a list of commands to execute when the execute command is received.
// each command is represented by 3 consecutive array values - channel, button, state.
// set to -1 when they don't contain a command to execute
char commandsToExec[16][3];
int nextCommandIndex = 0; // what index should we insert the next command into?

void setupFSM() {
  clearCommandsToExec();
}

/**
 * Simulate a FSM that parses strings of the format "rs131*\r", meaning to set channel 1, button 3, to state 1 (on).
 * Handles multiple commands (up to a total of 16), e.g. rs131140\r to turn on channel 1 btn 3, and turn off channel 1 btn 4
 * \n may be used instead of \r
 */
void updateFSM(char inputChar) {
  int charAsInt;
  switch (currentState) {
    case STATE_WAIT_FOR_R:
      if (inputChar == 'r') {
        Serial.println("-> STATE_WAIT_FOR_S");
        currentState = STATE_WAIT_FOR_S;
      }
      break;

    case STATE_WAIT_FOR_S:
      if (inputChar == 's') {
        Serial.println("-> STATE_READY_FOR_CHANNEL_OR_EXECUTE");
        currentState = STATE_READY_FOR_CHANNEL_OR_EXECUTE;
      } else {
        clearCommandsToExec();
        Serial.println("-> STATE_WAIT_FOR_R");
        currentState = STATE_WAIT_FOR_R;
      }
      break;

    case STATE_READY_FOR_CHANNEL_OR_EXECUTE:
      if (inputChar == 13 || inputChar == 10) { // carriage return or line feed
        executeCommands();
        clearCommandsToExec();
        Serial.println("-> STATE_WAIT_FOR_R");
        currentState = STATE_WAIT_FOR_R;
        break;
      }

      charAsInt = inputChar - '0';
      if (charAsInt >= 1 && charAsInt <= 4) {
        chosenChannel = charAsInt;
        Serial.println("-> STATE_WAIT_FOR_BUTTON");
        currentState = STATE_WAIT_FOR_BUTTON;
      } else {
        clearCommandsToExec();
        Serial.println("-> STATE_WAIT_FOR_R");
        currentState = STATE_WAIT_FOR_R;
      }
      break;

    case STATE_WAIT_FOR_BUTTON:
      charAsInt = inputChar - '0';
      if (charAsInt >= 1 && charAsInt <= 4) {
        chosenButton = charAsInt;
        Serial.println("-> STATE_WAIT_FOR_ONOFF");
        currentState = STATE_WAIT_FOR_ONOFF;
      } else {
        clearCommandsToExec();
        Serial.println("-> STATE_WAIT_FOR_R");
        currentState = STATE_WAIT_FOR_R;
      }
      break;
      
    case STATE_WAIT_FOR_ONOFF:
      charAsInt = inputChar - '0';
      if (charAsInt == 0 || charAsInt == 1) {
        addCommand(chosenChannel, chosenButton, charAsInt);
        Serial.println("-> STATE_READY_FOR_CHANNEL_OR_EXECUTE");
        currentState = STATE_READY_FOR_CHANNEL_OR_EXECUTE;
      } else {
        clearCommandsToExec();
        Serial.println("-> STATE_WAIT_FOR_R");
        currentState = STATE_WAIT_FOR_R;
      }
      break;
  }
}

void addCommand(int channel, int button, int onOff) {
  Serial.print("Adding command to queue: channel ");
  Serial.print((int)channel);
  Serial.print(", button ");
  Serial.print((int)button);
  Serial.print(", power ");
  Serial.println((int)onOff);
  commandsToExec[nextCommandIndex][0] = channel;
  commandsToExec[nextCommandIndex][1] = button;
  commandsToExec[nextCommandIndex][2] = onOff;
  if (nextCommandIndex < 15) {
    nextCommandIndex += 1;
  }
}

void clearCommandsToExec() {
  Serial.println("Clearing button command queue");
  for (int i=0; i<16; i++) {
    commandsToExec[i][0] = -1; // only set the channel to -1, because only the channel is checked for emptiness
  }
  nextCommandIndex = 0;
}

void executeCommands() {
  Serial.println("Executing button command queue");
  for (int i=0; i<16; i++) {
    if (commandsToExec[i][0] != -1) {
      simulateButton(commandsToExec[i][0], commandsToExec[i][1], commandsToExec[i][2]);
    }
  }
}
