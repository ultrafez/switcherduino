#include "MaplinCtrl.h"
#include "NexaCtrl.h"

#define DATA_PIN  2
#define VCC_PIN   3
#define GND_PIN   4
#define LED_PIN   17

#define NEXA_CONTROLLER_ID 1153614

MaplinCtrl maplinCtrl(DATA_PIN, LED_PIN);
NexaCtrl nexaCtrl(DATA_PIN, 0, LED_PIN); // rx pin isn't even used in that library. plus, we have no receiver hardware anyway

void setup() {
  pinMode(GND_PIN, OUTPUT);
  pinMode(VCC_PIN, OUTPUT);

  // Radio pin setup
  digitalWrite(GND_PIN, LOW);

  setupFSM();
  
  Serial.begin(115200);
  Serial.println("Maplin remote socket controller. Send commands in the regex format rs([1-4][1-4][01])*\\n to turn on/off sockets in channel 1-4 and button 1-4");
}

void loop() {
  // Valid serial string: rs131\r
  // Read and handle serial data if some has arrived
  if (Serial.available()) {
    updateFSM(Serial.read());
  }
}


/* RADIO */

void radioOn() {
  digitalWrite(VCC_PIN, HIGH);
}

void radioOff() {
  digitalWrite(VCC_PIN, LOW);
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

  radioOn();

  // Repeat the whole sequence 3 times to be safe
  int deviceId = 0;
  for (int j=0; j<3; j++) {
    for (int i=0; i<nextCommandIndex; i++) {
      //maplinCtrl.simulateButton(commandsToExec[i][0], commandsToExec[i][1], commandsToExec[i][2]);
      
      deviceId = (commandsToExec[i][0]-1)*4 + (commandsToExec[i][1]-1);
      
      if (commandsToExec[i][2] == 1) {
        nexaCtrl.DeviceOn(NEXA_CONTROLLER_ID, deviceId);
      } else {
        nexaCtrl.DeviceOff(NEXA_CONTROLLER_ID, deviceId);
      }
    }
  }

  radioOff();
}
