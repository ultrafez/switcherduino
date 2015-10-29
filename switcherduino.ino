#include "MaplinCtrl.h"
#include "NexaTransmitter.h"

/*
Nexa and Maplin socket/device controller.
Accepts commands over serial and executes them using the radio transmitter.
*/

#define DATA_PIN  2
#define VCC_PIN   3
#define GND_PIN   4
#define LED_PIN   17

#define NEXA_CONTROLLER_ID 1153614

MaplinCtrl maplinCtrl(DATA_PIN, LED_PIN);
NexaTransmitter nexaTransmitter(DATA_PIN, NEXA_CONTROLLER_ID);
typedef enum commandTypeEnum {UNKNOWN = -1, UNUSED = 0, NEXA = 1, MAPLIN = 2};

typedef struct {
  enum commandTypeEnum commandType; // whether this is a nexa command or maplin. futureproof, just add more enum types!
  unsigned int device : 4; // the device to turn on/off. for nexa, this the 16 bit device code, for maplin, it's composed from the channel+button
  bool doDim : 1; // are we setting the dim level of the device? or just turning on/off? not applicable for maplin
  bool onOff : 1; // turn the device on or off? the value is ignored if we're dimming
  unsigned int dim : 4; // the level between 0 and 15 to dim to. not applicable to maplin. only used if doDim is 1
} nexaCommand;


void setup() {
  pinMode(GND_PIN, OUTPUT);
  pinMode(VCC_PIN, OUTPUT);

  // Radio pin setup
  digitalWrite(GND_PIN, LOW);

  setupFSM();
  
  Serial.begin(115200);
  Serial.println("Remote socket controller. Supports Nexa (self-learning protocol) and Maplin sockets.");
}

void loop() {
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
const int STATE_WAIT_FOR_CMD = 1;
const int STATE_SET_TRANSMITTER_CODE = 2;
const int STATE_SET_NEXA_DEVICE = 3;
const int STATE_SET_NEXA_DIM = 4;
const int STATE_WAIT4_MAPLIN_CHANNEL = 5;
const int STATE_WAIT4_MAPLIN_BUTTON = 6;
const int STATE_WAIT4_MAPLIN_ONOFF = 7;


// State machine state
nexaCommand cmdQueue[16]; // you can execute a maximum of 16 commands in one go. These commands will be executed when \n is received
int currentCmdIndex = 0; // the command that we're currently editing
unsigned long transmitterCode = 0; // the Nexa transmitter code to use. 26 bits long
int currentState = STATE_WAIT_FOR_CMD;
int charsSeen = 0; // general purpose counter for counting how many chars we've seen in any particular step.

void setupFSM() {
  clearCommandsToExec();
}

/**
 * A finite state machine to implement the protocol at the top of the file.
 */
void updateFSM(char inputChar) {
  int charAsInt;
  int startState = currentState;
  //Serial.print("Processing char ");
  //Serial.println(inputChar);
  if (strchr("tnm", inputChar) != NULL && charsSeen > 0) {
    finishCmd();
  }

  switch (currentState) {
    case STATE_WAIT_FOR_CMD:
      if (inputChar == 't') {
        currentState = STATE_SET_TRANSMITTER_CODE;
        transmitterCode = 0;
      } else if (inputChar == 'n') {
        currentState = STATE_SET_NEXA_DEVICE;
        charsSeen = 0;
        cmdQueue[currentCmdIndex].commandType = NEXA;
        cmdQueue[currentCmdIndex].device = 0;
        cmdQueue[currentCmdIndex].doDim = false;
        cmdQueue[currentCmdIndex].onOff = false;
      } else if (inputChar == 'm') {
        currentState = STATE_WAIT4_MAPLIN_CHANNEL;
        cmdQueue[currentCmdIndex].commandType = MAPLIN;
        cmdQueue[currentCmdIndex].device = 0;
      } else if (inputChar == 13 || inputChar == 10) { // carriage return or line feed
        executeCmds();
      }
      break;

    case STATE_SET_TRANSMITTER_CODE:
      if (inputChar == ';') {
        currentState = STATE_WAIT_FOR_CMD;
      } else {
        charAsInt = inputChar - '0';
        if (isNum(charAsInt)) {
          transmitterCode = transmitterCode*10 + charAsInt;
        }
      }
      break;

    case STATE_SET_NEXA_DEVICE:
      charAsInt = inputChar - '0';
      if (isNum(charAsInt)) {
        if (charsSeen < 2) {
          cmdQueue[currentCmdIndex].device = cmdQueue[currentCmdIndex].device * 10 + charAsInt;
          charsSeen++;
        } else {
          cmdQueue[currentCmdIndex].onOff = (charAsInt == 1);
          finishCmd();
        }
      } else if (inputChar == 'd') {
        currentState = STATE_SET_NEXA_DIM;
        cmdQueue[currentCmdIndex].doDim = true;
        cmdQueue[currentCmdIndex].dim = 0;
        charsSeen = 0;
      }
      break;

    case STATE_SET_NEXA_DIM:
      charAsInt = inputChar - '0';
      if (isNum(charAsInt)) {
        cmdQueue[currentCmdIndex].dim = cmdQueue[currentCmdIndex].dim*10 + charAsInt;
        charsSeen++;
        if (charsSeen == 2) {
          finishCmd();
        }
      }
      break;

    case STATE_WAIT4_MAPLIN_CHANNEL:
      charAsInt = inputChar - '0';
      if (isNum(charAsInt)) {
        cmdQueue[currentCmdIndex].device = charAsInt;
        currentState = STATE_WAIT4_MAPLIN_BUTTON;
      }
      break;

    case STATE_WAIT4_MAPLIN_BUTTON:
      charAsInt = inputChar - '0';
      if (isNum(charAsInt)) {
        cmdQueue[currentCmdIndex].device = (cmdQueue[currentCmdIndex].device-1) * 4 + charAsInt-1;
        currentState = STATE_WAIT4_MAPLIN_ONOFF;
      }
      break;

    case STATE_WAIT4_MAPLIN_ONOFF:
      cmdQueue[currentCmdIndex].onOff = (inputChar == '1');
      finishCmd();
      break;
  }

  /*if (currentState != startState) {
    Serial.print("state ");
    Serial.print(startState);
    Serial.print(" -> ");
    Serial.println(currentState);
  }*/
}

// we've finished writing our command
void finishCmd() {
  currentState = STATE_WAIT_FOR_CMD;
  if (currentCmdIndex < 15) { // the last index of the command array
    currentCmdIndex++;
    cmdQueue[currentCmdIndex].commandType = UNKNOWN;
  }
  charsSeen = 0;
}

bool isNum(char ch) {
  return ch >= 0 && ch <= 9;
}

void clearCommandsToExec() {
  Serial.println("Clearing button command queue");
  currentCmdIndex = 0;
  cmdQueue[currentCmdIndex].commandType = UNKNOWN;
}

void executeCmds() {
  Serial.println("Executing button command queue");
  // Repeat the whole sequence 3 times to be safe
  for (int j=0; j<3; j++) {
    for (int i=0; i<currentCmdIndex; i++) {
      //Serial.print("Execute command ");Serial.println(i);
      
      radioOn();
      
      switch (cmdQueue[i].commandType) {
        case NEXA :
          if (cmdQueue[i].doDim) {
            Serial.print("Nexa dim "); Serial.print(cmdQueue[i].device); Serial.print(" "); Serial.println(cmdQueue[i].dim);
            nexaTransmitter.setSwitch(false, cmdQueue[i].device, cmdQueue[i].dim);
          } else {
            Serial.print("Nexa switch "); Serial.print(cmdQueue[i].device); Serial.print(" "); Serial.println(cmdQueue[i].onOff);
            nexaTransmitter.setSwitch(cmdQueue[i].onOff, cmdQueue[i].device, 0);
          }
          break;
        case MAPLIN :
          Serial.print("Maplin exec "); Serial.print(cmdQueue[i].device); Serial.print(" "); Serial.println(cmdQueue[i].onOff);
          maplinCtrl.simulateButton(cmdQueue[i].device, cmdQueue[i].onOff);
        break;
      }
      
      radioOff();
      delay(100);
    }
  }

  clearCommandsToExec();
}
