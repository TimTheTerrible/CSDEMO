/*********************
  COUNTER STRIKE BOMB
**********************/

// include the library code:
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>
#include "Keypad.h"

// Display stuff
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// These #defines make it easy to set the backlight color
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

#define MAX_LINE_LEN   17

// Sound stuff
#define PIN_SND_BEEP           13
#define PIN_SND_BOMB_PLANTED    9
#define PIN_SND_BOMB_DEFUSED   10
#define PIN_SND_T_WIN          11
#define PIN_SND_CT_WIN         12
#define PIN_SND_BOMB_DETONATED  0
#define PIN_SND_ACTIVE          1

// Keypad stuff
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
const char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[ROWS] = {5, 6, 7, 8}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {2, 3, 4}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

#define KEY_ZERO      0x39
#define KEY_ONE       0x30
#define KEY_TWO       0x31
#define KEY_THREE     0x32
#define KEY_FOUR      0x33
#define KEY_FIVE      0x34
#define KEY_SIX       0x35
#define KEY_SEVEN     0x36
#define KEY_EIGHT     0x37
#define KEY_NINE      0x38
#define KEY_ASTERISK  0x2A
#define KEY_POUND     0x23

// State stuff
#define NUM_STATES      6
#define STATE_SAFE      0
#define STATE_ARMING    1
#define STATE_ARMED     2
#define STATE_DISARMING 3
#define STATE_DISARMED  4
#define STATE_DETONATED 5

char * StateNames[NUM_STATES] = {
  "SAFE",
  "ARMING",
  "ARMED",
  "DISARMING",
  "DISARMED",
  "DETONATED"
};

long timeToGoBOOM ;
int State;
int oldState;
char inputBuf[MAX_LINE_LEN] = "";
char armingCode[MAX_LINE_LEN] = "7355608";
int oldKey = NO_KEY;

// Main program
void playSound(int snd) {
  Serial.print("Playing sound ID: "); Serial.println(snd);
  while ( digitalRead(PIN_SND_ACTIVE) == LOW )
      delay(5);

  digitalWrite(snd, LOW);
  delay(150);
  digitalWrite(snd, HIGH);
  delay(150);
}

void checkTimer() {

  if ( millis() >= timeToGoBOOM and ( State == STATE_ARMED or State == STATE_DISARMING ) ) {
      State = STATE_DETONATED;
      playSound(PIN_SND_BOMB_DETONATED);
  }
}

void updateDisplay () {
  char line1[MAX_LINE_LEN] = "";
  char line2[MAX_LINE_LEN] = "";
  char buf[64] = ""; 
  
  long minutes = long( (timeToGoBOOM - millis() ) / 60000 );
  char s_minutes[3]; sprintf(s_minutes, "%2d", minutes);
  long seconds = long( ( ( timeToGoBOOM - millis() ) / 1000 ) % 60 );
  char s_seconds[3]; sprintf(s_seconds, "%2.2d", seconds);


  // Do a little debug...
  if ( State != oldState ) {
    Serial.println("******************************");
    sprintf(buf, "State changed to: %s", StateNames[State]); Serial.println(buf);
    oldState = State;
  }

/*
  if ( State == STATE_ARMED or State == STATE_DISARMING ) {
    Serial.println("Counting down:");
    sprintf(buf, "State: %s", StateNames[State]); Serial.println(buf);
    sprintf(buf, "Time left: %s:%s", s_minutes, s_seconds); Serial.println(buf);
    sprintf(buf, "timeToGoBOOM = %lu", timeToGoBOOM); Serial.println(buf);
    sprintf(buf, "millis() = %lu", millis()); Serial.println(buf);
    sprintf(buf, "millis left = %lu", timeToGoBOOM - millis()); Serial.println(buf);
    Serial.println();
  }
*/

  // Prepare the display contents...
  switch ( State ) {
    case STATE_SAFE:
      sprintf(line1, "%-16s", "SAFE");
      sprintf(line2, "%-16s", "PUSH # TO ARM");
      break;
    case STATE_ARMING:
      sprintf(line1, "%-16s", "ARM CODE");
      sprintf(line2, "> %-14s", inputBuf);
      break;
    case STATE_ARMED:
      sprintf(line1, "%-11s%s:%s", "ARMED", s_minutes, s_seconds);
      sprintf(line2, "%-16s", "PUSH # TO DEFUSE");
      break;
    case STATE_DISARMING:
      sprintf(line1, "%-11s%s:%s", "DEFUSE CODE", s_minutes, s_seconds);
      sprintf(line2, "> %-14s", inputBuf);
      break;
    case STATE_DISARMED:
      sprintf(line1, "%-16s", "DEFUSED!");
      sprintf(line2, "%-16s", "PUSH # TO RESET");
      break;
    case STATE_DETONATED:
      sprintf(line1, "%-16s", "DETONATED!");
      sprintf(line2, "%-16s", "PUSH # TO RESET");
      break;
    default:
      strcpy(line1, "INVALID STATE");
      strcpy(line2, "BOMB IS BROKEN!");
      break;
  }

  // Update the display...
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
  // if ( in_input )
  //lcd.setCursor(0, 1);
  //lcd.cursor();
}

void handleInput() {  
  char key = NO_KEY;

  // Handle input
  key = keypad.getKey();
  if ( key != NO_KEY and key != oldKey ) {

    Serial.print("Key pressed: "); Serial.println(key);

    // TODO: Play key beep sound
    playSound(PIN_SND_BEEP);
  
    // Handle the input
    switch ( State ) {
      case STATE_SAFE:
        if ( key == KEY_POUND )
          State = STATE_ARMING;
          memset(inputBuf, 0x0, MAX_LINE_LEN);
        break;
      case STATE_ARMING:
        if ( key == KEY_POUND ) {
          
          // Did they enter the right code?
          if ( strncmp(inputBuf, armingCode, MAX_LINE_LEN) == 0 ) {
            State = STATE_ARMED;
            playSound(PIN_SND_BOMB_PLANTED);
            timeToGoBOOM = millis() + 300000;
            Serial.println("handleInput() set State to STATE_ARMED"); 
            Serial.print("handleInput() set timetoGoBOOM to "); Serial.println(timeToGoBOOM);
          }
          else
            State = STATE_SAFE;
        }
        else if ( key == KEY_ASTERISK ) {
          State = STATE_SAFE;
        }
        else {
          strncat(inputBuf, &key, 1);
        }
        break;
      case STATE_ARMED:
        if ( key == KEY_POUND )
          State = STATE_DISARMING;
          memset(inputBuf, 0x0, MAX_LINE_LEN);
          timeToGoBOOM = millis() + 30000;
          Serial.println("handleInput() set State to STATE_ARMED"); 
          Serial.print("handleInput() set timetoGoBOOM to "); Serial.println(timeToGoBOOM);
        break;
      case STATE_DISARMING:
        if ( key == KEY_POUND ) {
          if ( strncmp(inputBuf, armingCode, MAX_LINE_LEN) == 0 ) {
            State = STATE_DISARMED;
            playSound(PIN_SND_BOMB_DEFUSED);
            timeToGoBOOM = 0;
            Serial.println("handleInput() set State to STATE_DISARMED"); 
            Serial.println("handleInput() set timetoGoBOOM to 0");
          }
          else
            State = STATE_ARMED;
        }
        else if ( key == KEY_ASTERISK ) {
          State = STATE_ARMED;
        }
        else {
          strncat(inputBuf, &key, 1);
        }
        break;
      case STATE_DISARMED:
        if ( key == KEY_POUND ) {
          State = STATE_SAFE;
          playSound(PIN_SND_CT_WIN);
        }
        break;
      case STATE_DETONATED:
        if ( key == KEY_POUND ) {
          State = STATE_SAFE;
          playSound(PIN_SND_T_WIN);
        }
        break;
      default:
        break;
    }

    Serial.print("inputBuf = '"); Serial.print(inputBuf); Serial.println("'");
  }

  oldKey = key;
}

void setup() {
  // Debugging output
  Serial.begin(9600);

  // Set up the sound module...
  pinMode(PIN_SND_BEEP, OUTPUT);
  digitalWrite(PIN_SND_BEEP, HIGH);

  pinMode(PIN_SND_T_WIN, OUTPUT);
  digitalWrite(PIN_SND_T_WIN, HIGH);

  pinMode(PIN_SND_CT_WIN, OUTPUT);
  digitalWrite(PIN_SND_CT_WIN, HIGH);

  pinMode(PIN_SND_BOMB_DEFUSED, OUTPUT);
  digitalWrite(PIN_SND_BOMB_DEFUSED, HIGH);

  pinMode(PIN_SND_BOMB_PLANTED, OUTPUT);
  digitalWrite(PIN_SND_BOMB_PLANTED, HIGH);

  pinMode(PIN_SND_BOMB_DETONATED, OUTPUT);
  digitalWrite(PIN_SND_BOMB_DETONATED, HIGH);

  pinMode(PIN_SND_ACTIVE, INPUT);

  // Set up the LCD...
  lcd.begin(16, 2);
  lcd.setBacklight(WHITE);

  // Draw the welcome message...
  lcd.setCursor(0, 0);
  lcd.print("Counter Strike");
  lcd.setCursor(0, 1);
  lcd.print("Bomb V1.0");
  delay(2000);
  lcd.clear();

  //playSound(PIN_SND_BOMB_PLANTED);
  
  // Set up the state of the bomb...
  State = STATE_SAFE;
  oldState = State;

  Serial.println("Starting!");
  Serial.print("State: STATE_SAFE");
}

void loop() {
  // Handle Input
  handleInput();
  
  // Update Display
  updateDisplay();
  
  // Check Timer
  checkTimer();

}
