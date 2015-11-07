/*********************
  COUNTER STRIKE
  Demolition Toy
**********************/

// include the library code:
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Keypad.h>
#include <Adafruit_MCP23017.h>
#include "LiquidCrystal.h"
#include "debugprint.h"

// Sound stuff
AudioPlaySdWav           playWav1;
AudioOutputI2S           audioOutput;
AudioConnection          patchCord1(playWav1, 0, audioOutput, 0);
AudioConnection          patchCord2(playWav1, 1, audioOutput, 1);
AudioControlSGTL5000     sgtl5000_1;

// Use these with the audio adaptor board
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

// Display stuff
LiquidCrystal lcd(0);

// These #defines make it easy to set the backlight color
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

// Display backlight timeout
#define IDLE_TIMEOUT 30000;

#define MAX_LINE_LEN   17

// Sound stuff
#define SND_BEEP            0
#define SND_LETS_MOVE_OUT   1
#define SND_BOMB_PLANTED    2
#define SND_BOMB_DEFUSED    3
#define SND_T_WIN           4
#define SND_CT_WIN          5
#define SND_BOMB_DETONATED  6
#define MAX_SOUNDS          7

const char * soundNames[MAX_SOUNDS] = {
  "Beep",
  "Let's Move Out",
  "Bomb Planted",
  "Bomb Defused",
  "Terrorists Win",
  "Counter-Terrorists Win",
  "KABOOM!!!",
};

const char * fileNames[MAX_SOUNDS] = {
  "T01.WAV",
  "T02.WAV",
  "T03.WAV",
  "T04.WAV",
  "T05.WAV",
  "T06.WAV",
  "T07.WAV",
};

// Keypad stuff
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
const char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[ROWS] = {2, 1, 21, 20}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 4, 3}; //connect to the column pinouts of the keypad

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
#define NUM_STATES       7
#define STATE_SAFE       0
#define STATE_ARMING     1
#define STATE_ARMED      2
#define STATE_DISARMING  3
#define STATE_DISARMED   4
#define STATE_DETONATED  5
#define STATE_SCOREBOARD 6

const char * StateNames[NUM_STATES] = {
  "SAFE",
  "ARMING",
  "ARMED",
  "DISARMING",
  "DISARMED",
  "DETONATED",
  "SCOREBOARD"
};

// Defuse Kit Override
#define PIN_DEFUSE 8
bool defuseKitPresent = FALSE;


// Global Variables
unsigned long timeToGoBOOM ;
int State;
int oldState;
char inputBuf[MAX_LINE_LEN] = "";
char armingCode[MAX_LINE_LEN] = "7355608";
int oldKey = NO_KEY;
int scoreCounterTerrorists = 0;
int scoreTerrorists = 0;
long idleTimer;

// Main program

void playSound(int snd, bool blocking) {
  debugprint(DEBUG_TRACE,"Playing sound ID: %d '%s'", snd, soundNames[snd]);
  
  playWav1.play(fileNames[snd]);
  
  // A brief delay for the library read WAV info
  delay(5);
  
  debugprint(DEBUG_TRACE, "Blocking is %s", blocking ? "on" : "off");

  if ( blocking ) {
    // Wait for the sound to play all the way through...
    while (playWav1.isPlaying()) {
      delay(5);
    }
  }

  debugprint(DEBUG_TRACE, "done!");
}

void checkTimer() {
  if ( millis() >= timeToGoBOOM and ( State == STATE_ARMED or State == STATE_DISARMING ) ) {
      State = STATE_DETONATED;
      playSound(SND_BOMB_DETONATED, TRUE);
  }
}

void updateDisplay () {
  long minutes = long( (timeToGoBOOM - millis() ) / 60000 );
  char s_minutes[3]; sprintf(s_minutes, "%2ld", minutes);
  long seconds = long( ( ( timeToGoBOOM - millis() ) / 1000 ) % 60 );
  char s_seconds[3]; sprintf(s_seconds, "%2.2ld", seconds);


  // Do a little debug...
  if ( State != oldState ) {
    debugprint(DEBUG_TRACE,"******************************");
    debugprint(DEBUG_TRACE, "State changed to: %s", StateNames[State]);
    oldState = State;
  }

/*
  if ( State == STATE_ARMED or State == STATE_DISARMING ) {
    debugprint(DEBUG_TRACE,"Counting down:");
    debugprint(DEBUG_TRACE, "State: %s", StateNames[State]);
    debugprint(DEBUG_TRACE, "Time left: %s:%s", s_minutes, s_seconds);
    debugprint(DEBUG_TRACE, "timeToGoBOOM = %lu", timeToGoBOOM);
    debugprint(DEBUG_TRACE, "millis() = %lu", millis());
    debugprint(DEBUG_TRACE, "millis left = %lu", timeToGoBOOM - millis());
  }
*/

  // Prepare the display contents...
  char line1[MAX_LINE_LEN];
  char line2[MAX_LINE_LEN];  
  int slot, oldSlot;
  char hint[MAX_LINE_LEN];
  
  switch ( State ) {
    case STATE_SAFE:
      sprintf(line1, "%-16s", "SAFE");
      sprintf(line2, "%-16s", "PUSH # TO ARM");
      break;
    case STATE_ARMING:
      sprintf(line1, "%-16s", "ARM CODE");
      sprintf(line2, ">%-7s<       ", inputBuf);
      break;
    case STATE_ARMED:
      sprintf(line1, "%-11s%s:%s", "ARMED", s_minutes, s_seconds);
      sprintf(line2, "%-16s", "PUSH # TO DEFUSE");
      break;
    case STATE_DISARMING:
      if ( defuseKitPresent ) {
        sprintf(hint, "%s", armingCode);
      }
      else {
        sprintf(hint, "%s", "       ");
        slot = millis() % 7;      
        hint[slot] = armingCode[slot];
      }
      sprintf(line1, ">%-7s<  %s:%s", hint, s_minutes, s_seconds);
      sprintf(line2, ">%-7s<       ", inputBuf);
      delay(75);
      break;
    case STATE_DISARMED:
      sprintf(line1, "%-16s", "DEFUSED!");
      sprintf(line2, "%-16s", "PUSH # TO RESET");
      break;
    case STATE_DETONATED:
      sprintf(line1, "%-16s", "DETONATED!");
      sprintf(line2, "%-16s", "PUSH # TO RESET");
      break;
    case STATE_SCOREBOARD:
      sprintf(line1, "%2d %-s", scoreCounterTerrorists, "CT Score");
      sprintf(line2, "%2d %-s", scoreTerrorists, "Terrorists");
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

  // If the idle timer has expired, shut off the backlight...
  if ( millis() > idleTimer ) {
    lcd.setBacklight(LOW);
  }
}

void handleInput() {  
  char key = NO_KEY;

  // Get a key from the keypad...
  key = keypad.getKey();
  
  if ( key != NO_KEY and key != oldKey ) {

    // Turn on the display backlight...
    lcd.setBacklight(HIGH);
    
    // Set the idle timeout...
    idleTimer = millis() + IDLE_TIMEOUT;

    debugprint(DEBUG_TRACE,"Key pressed: %d", key);

    playSound(SND_BEEP, FALSE);
  
    // Handle the input
    switch ( State ) {
      case STATE_SAFE:
        if ( key == KEY_POUND ) {
          State = STATE_ARMING;
          memset(inputBuf, 0x0, MAX_LINE_LEN);
        }
        else if ( key == KEY_ASTERISK ) {
          State = STATE_SCOREBOARD;
        }
        break;
      case STATE_ARMING:
        if ( key == KEY_POUND ) {
          
          // Have they entered enough numbers?
          debugprint(DEBUG_TRACE, "strlen(inputBuf) = %d", strlen(inputBuf));
          if ( strlen(inputBuf) > 6 ) {
            
            // Save the new arming code...
            strncpy(armingCode, inputBuf, MAX_LINE_LEN);
            
            // Set the bomb to armed...
            State = STATE_ARMED;
            
            // Play an appropriate sound...
            playSound(SND_BOMB_PLANTED, TRUE);
            
            // Set the timer for five minutes...
            timeToGoBOOM = millis() + 300000;

            // Show your work...
            debugprint(DEBUG_TRACE, "handleInput() set armingCode to '%s'", armingCode);
            debugprint(DEBUG_TRACE, "handleInput() set State to STATE_ARMED"); 
            debugprint(DEBUG_TRACE, "handleInput() set timetoGoBOOM to %ld", timeToGoBOOM);
          }
          else
            // Otherwise, reset to safe and make them start over...
            State = STATE_SAFE;
        }
        else if ( key == KEY_ASTERISK ) {
          // Aterisk is "cancel"...
          State = STATE_SAFE;
        }
        else {
          if ( strlen(inputBuf) > 6 ) {
            playSound(SND_BEEP, FALSE); delay(150); playSound(SND_BEEP, FALSE);
          }
          else {
            strncat(inputBuf, &key, 1);
          }
        }
        break;
      case STATE_ARMED:
        if ( key == KEY_POUND ) {
          State = STATE_DISARMING;

          // Check to see if a defuse kit is attached...
          defuseKitPresent = digitalRead(PIN_DEFUSE);
          
          // Override missing hardware...
          defuseKitPresent = FALSE;
          
          debugprint(DEBUG_TRACE, "Defuse kit %sdetected!", defuseKitPresent ? "" : "NOT ");
  
          // Clear the input buffer...
          memset(inputBuf, 0x0, MAX_LINE_LEN);

          // Tamper with MY bomb, will you?!?!
          // If there's more than 30 seconds left, reduce the timer to 30 seconds.
          // If there's less, tough shit.
          if ( timeToGoBOOM - millis() > 30000 )
            timeToGoBOOM = millis() + 30000;

          debugprint(DEBUG_TRACE,"handleInput() set State to STATE_ARMED"); 
          debugprint(DEBUG_TRACE,"handleInput() set timetoGoBOOM to %ld", timeToGoBOOM);
        }
        break;
      case STATE_DISARMING:
        if ( key == KEY_POUND ) {
          if ( strncmp(inputBuf, armingCode, MAX_LINE_LEN) == 0 ) {
            State = STATE_DISARMED;
            playSound(SND_BOMB_DEFUSED, TRUE);
            timeToGoBOOM = 0;
            debugprint(DEBUG_TRACE,"handleInput() set State to STATE_DISARMED"); 
            debugprint(DEBUG_TRACE,"handleInput() set timetoGoBOOM to 0");
          }
          else {
            State = STATE_ARMED;
          }
        }
        else if ( key == KEY_ASTERISK ) {
          State = STATE_ARMED;
        }
        else {
          if ( strlen(inputBuf) < 7 ) {
            strncat(inputBuf, &key, 1);
          }
          else {
            playSound(SND_BEEP, FALSE); delay(150); playSound(SND_BEEP, FALSE);
          }
        }
        break;
      case STATE_DISARMED:
        if ( key == KEY_POUND ) {
          State = STATE_SAFE;
          playSound(SND_CT_WIN, TRUE);
          scoreCounterTerrorists += 1;
        }
        break;
      case STATE_DETONATED:
        if ( key == KEY_POUND ) {
          State = STATE_SAFE;
          playSound(SND_T_WIN, TRUE);
          scoreTerrorists += 1;
        }
        break;
      case STATE_SCOREBOARD:
        State = STATE_SAFE;
        break;
      default:
        break;
    }

    debugprint(DEBUG_TRACE,"inputBuf = '%s'", inputBuf);
  }

  oldKey = key;
}

void setup() {
  // Debugging output
  Serial.begin(9600);

  // Set up the sound...
  AudioMemory(20);

  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);

  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    while (1) {
      debugprint(DEBUG_TRACE,"Unable to access the SD card");
      delay(500);
    }
  }

  // Set up the LCD...
  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);

  // Draw the welcome message...
  lcd.setCursor(0, 0);
  lcd.print("Counter Strike");
  lcd.setCursor(0, 1);
  lcd.print("Bomb V2.2");
  delay(2000);
  lcd.clear();

  // Set up the defusal override
  pinMode(PIN_DEFUSE, INPUT_PULLUP);
  
  // Set up the state of the bomb...
  State = STATE_SAFE;
  oldState = State;

  playSound(SND_LETS_MOVE_OUT, TRUE);
  debugprint(DEBUG_TRACE,"Starting!");
  debugprint(DEBUG_TRACE,"State: STATE_SAFE");

  idleTimer = millis() + IDLE_TIMEOUT;
}

void loop() {
  // Handle Input
  handleInput();
  
  // Update Display
  updateDisplay();
  
  // Check Timer
  checkTimer();

}
