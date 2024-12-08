#define SPEAKER_PIN 8
#define BUTTON_A 12
#define BUTTON_B 13
#define BUTTON_C A1

// LED matrix pins
const int rows[] = {7, 6, 5, 4, 3, 2, 11};
const int cols[] = {A0, 9, 10, A5, A4, A3, A2, 0};

// Digit patterns for 0-9 and colon
const uint8_t digitPatterns[11][7] = {
  {B00111100, B01100110, B01100110, B01100110, B01100110, B01100110, B00111100}, // 0
  {B00011000, B00111000, B00011000, B00011000, B00011000, B00011000, B01111110}, // 1
  {B00111100, B01100110, B00000110, B00011100, B00110000, B01100110, B01111110}, // 2
  {B00111100, B01100110, B00000110, B00011100, B00000110, B01100110, B00111100}, // 3
  {B00001110, B00011110, B00110110, B01100110, B01111111, B00000110, B00001111}, // 4
  {B01111110, B01100000, B01111100, B00000110, B00000110, B01100110, B00111100}, // 5
  {B00011100, B00110000, B01100000, B01111100, B01100110, B01100110, B00111100}, // 6
  {B01111110, B01100110, B00000110, B00001100, B00011000, B00011000, B00011000}, // 7
  {B00111100, B01100110, B01100110, B00111100, B01100110, B01100110, B00111100}, // 8
  {B00111100, B01100110, B01100110, B00111110, B00000110, B00001100, B00111000},  // 9
  {B00000000, B00011000, B00011000, B00000000, B00011000, B00011000, B00000000}  // colon
};

// Global variables
unsigned long lastMillis = 0;
const unsigned long interval = 60000; // 1 minute interval
// test alarm
// int timerMinutes = 1;
int timerMinutes = 24 * 60;          // Default timer in minutes (24:00)
int keyword[3] = {1, 1, 1};          // Default keyword
int inputKeyword[3] = {0, 0, 0};
int currentDigit = 0;                // For keyword input and setting modes
bool timerRunning = true;            // Whether the timer is active
bool soundPlaying = false;           // Whether the alarm sound is active
bool keywordModeA = false;            // Whether in keyword entry mode(for timer)
bool keywordModeB = false;            // Whether in keyword entry mode(for change password)
bool timerSettingMode = false;       // Whether in timer setting mode
bool keywordSetMode = false;         // Whether in keyword set mode

// Timing variables for LED display updates
unsigned long previousMillisDisplay = 0;   // Last display update time
unsigned long previousMillisPattern = 0;   // Last pattern update time
const long displayInterval = 2;           // LED matrix refresh interval
const long patternInterval = 80;          // Scrolling pattern update interval

unsigned long previousFlashTime = 0;      // Flash timing for digit display
unsigned long previousRowTime = 0;        // Row refresh timing for LED matrix
int currentFlashCount = 0;                // Counter for flashes
int currentRow = 0;                       // Current row being refreshed
int flashDelay = 250;                     // Flash delay in milliseconds
bool displayOn = true;                    // Indicates if the display is active

unsigned long previousScrollTime = 0;     // Timing for scrolling text updates

const int rowDelay = 1; // Delay per row (in ms)
const int scrollDelay = 1000; // Delay between scroll updates (in ms)

// Timer display variables
// test alarm
// String timeString = "00:01";
String timeString = "24:00";
int scrollIndex = 0;                       // Scrolling text index
uint8_t cur_pattern[7] = {0};              // Current LED pattern for display

// Button state tracking structure
struct switchTracker {
  int pin;                // Pin associated with the button
  int lastReading;        // Last button state
  long lastChangeTime;    // Last debounce timing
  int switchState;        // Current button state
};

switchTracker buttonATracker = {BUTTON_A, HIGH, 0, HIGH};
switchTracker buttonBTracker = {BUTTON_B, HIGH, 0, HIGH};
switchTracker buttonCTracker = {BUTTON_C, HIGH, 0, HIGH};

// Function to handle debouncing for buttons
boolean switchChange(struct switchTracker &sw) {
  const long debounceTime = 100;          // Debounce time in milliseconds
  boolean result = false;
  int reading = digitalRead(sw.pin);
  if (reading != sw.lastReading) sw.lastChangeTime = millis();
  sw.lastReading = reading;
  if ((millis() - sw.lastChangeTime) > debounceTime) {
    result = (reading != sw.switchState);
    sw.switchState = reading;
  }
  return result;
}

void setup() {
  // Initialize LED matrix pins
  for (int i = 0; i < 8; i++) {
    pinMode(cols[i], OUTPUT);
  }
  for (int i = 0; i < 7; i++) {
    pinMode(rows[i], OUTPUT);
  }

  // Initialize button pins as inputs
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(BUTTON_A, INPUT);
  pinMode(BUTTON_B, INPUT);
  pinMode(BUTTON_C, INPUT);

  // Initialize button trackers with the actual button states
  buttonATracker.lastReading = digitalRead(BUTTON_A);
  buttonATracker.switchState = buttonATracker.lastReading;

  buttonBTracker.lastReading = digitalRead(BUTTON_B);
  buttonBTracker.switchState = buttonBTracker.lastReading;

  buttonCTracker.lastReading = digitalRead(BUTTON_C);
  buttonCTracker.switchState = buttonCTracker.lastReading;
}

void loop() {
  unsigned long currentMillis = millis();

  // Default timer mode
  if (!keywordModeA && !keywordModeB && !timerSettingMode && !keywordSetMode) {
    if (timerRunning) {
      if (currentMillis - lastMillis >= interval) {
        lastMillis = currentMillis;
        timerMinutes--;
        if (timerMinutes <= 0) {
          timerMinutes = 0;
          timerRunning = false;
          soundPlaying = true;
        }
        updateTimerString();
      }
    } else if (soundPlaying) {
      playAlarm();
      if (switchChange(buttonCTracker) && buttonCTracker.switchState == LOW) {
        soundPlaying = false;
        timerMinutes = 24 * 60; // Reset timer
        timerRunning = true;
        updateTimerString();
      }
    }
  }

  // Enter keyword mode for keyword setting
  if (!keywordModeB && !keywordModeA && !keywordSetMode && !timerSettingMode &&
      switchChange(buttonBTracker) && buttonBTracker.switchState == LOW) {
    // reset input keyword storage
    inputKeyword[0] = 0;
    inputKeyword[1] = 0;
    inputKeyword[2] = 0;
    currentDigit = 0;
    keywordModeB = true;
  }

  // Handle keyword input for keyword setting
  if (keywordModeB) {
    displayDigit(inputKeyword[currentDigit]);
    if (switchChange(buttonATracker) && buttonATracker.switchState == LOW) {
      inputKeyword[currentDigit]++;
      if (inputKeyword[currentDigit] > 9) inputKeyword[currentDigit] = 0;
    }
    if (switchChange(buttonBTracker) && buttonBTracker.switchState == LOW) {
      inputKeyword[currentDigit]--;
      if (inputKeyword[currentDigit] < 0) inputKeyword[currentDigit] = 9;
    }

    if (switchChange(buttonCTracker) && buttonCTracker.switchState == LOW) {
      currentDigit++;
      if (currentDigit > 2) {
        // Check keyword
        if (memcmp(inputKeyword, keyword, 3) == 0) { // correct keyword, enter keyword set mode
          keywordSetMode = true;
          keywordModeB = false;
          currentDigit = 0;
        } else {
          keywordModeB = false; // Return to default mode if keyword is incorrect
          currentDigit = 0;
        }
      }
    }
  }

  // Keyword set mode
  if (keywordSetMode) {
    flashDisplayDigit(keyword[currentDigit]);
    if (switchChange(buttonATracker) && buttonATracker.switchState == LOW) {
      keyword[currentDigit]++;
      if (keyword[currentDigit] > 9) keyword[currentDigit] = 0;
    }
    if (switchChange(buttonBTracker) && buttonBTracker.switchState == LOW) {
      keyword[currentDigit]--;
      if (keyword[currentDigit] < 0) keyword[currentDigit] = 9;
    }
    if (switchChange(buttonCTracker) && buttonCTracker.switchState == LOW) {
      currentDigit++; //confirm the digit and move on to the next digit
      if (currentDigit > 2) { //finished setting all three digits
        keywordSetMode = false;
        currentDigit = 0;
      }
    }
  }

  // Enter keyword mode for timer setting
  if (!keywordModeB && !keywordModeA && !keywordSetMode && !timerSettingMode &&
     switchChange(buttonATracker) && buttonATracker.switchState == LOW) {
    // reset input keyword storage
    inputKeyword[0] = 0;
    inputKeyword[1] = 0;
    inputKeyword[2] = 0;
    currentDigit = 0;
    keywordModeA = true;
  }

  // Handle keyword input for timer setting
  if (keywordModeA) {
    displayDigit(inputKeyword[currentDigit]);
    if (switchChange(buttonATracker) && buttonATracker.switchState == LOW) {
      inputKeyword[currentDigit]++;
      if (inputKeyword[currentDigit] > 9) inputKeyword[currentDigit] = 0;
    }
    if (switchChange(buttonBTracker) && buttonBTracker.switchState == LOW) {
      inputKeyword[currentDigit]--;
      if (inputKeyword[currentDigit] < 0) inputKeyword[currentDigit] = 9;
    }

    if (switchChange(buttonCTracker) && buttonCTracker.switchState == LOW) {
      currentDigit++;
      if (currentDigit > 2) {
        // Check keyword
        if (memcmp(inputKeyword, keyword, 3) == 0) { // correct keyword, enter keyword set mode
          timerSettingMode = true;
          keywordModeA = false;
          currentDigit = 0;
        } else {
          keywordModeA = false; // Return to default mode if keyword is incorrect
          currentDigit = 0;
        }
      }
    }
  }

  // Handle timer setting mode
  if (timerSettingMode) {
    // Adjust the timer duration in 15-minute increments
    if (switchChange(buttonATracker) && buttonATracker.switchState == LOW) {
      timerMinutes += 15;
      updateTimerString();
    }
    if (switchChange(buttonBTracker) && buttonBTracker.switchState == LOW) {
      timerMinutes -= 15;
      if (timerMinutes < 0) timerMinutes = 0;
      updateTimerString();
    }
    if (switchChange(buttonCTracker) && buttonCTracker.switchState == LOW) {
      timerSettingMode = false; // Exit timer setting mode
    }
  }

  // Update LED matrix display
  if (currentMillis - previousMillisDisplay >= displayInterval) {
    previousMillisDisplay = currentMillis;
    updateDisplay();
  }

  // Update scrolling pattern
  if (currentMillis - previousMillisPattern >= patternInterval) {
    previousMillisPattern = currentMillis;
    updatePattern();
  }
}

// Function to update the timer as HH:MM string
void updateTimerString() {
  int hours = timerMinutes / 60;
  int mins = timerMinutes % 60;
  timeString = (hours < 10 ? "0" : "") + String(hours) + ":" + (mins < 10 ? "0" : "") + String(mins);
}

// Function to update the pattern by scrolling it from right to left
void updatePattern() {
  if (!keywordModeA && !keywordModeB && !keywordSetMode){
    // Iterate over each row to update the current pattern
    for (int row = 0; row < 7; row++) {
      // Get the appropriate bit from the combined sequence of the time string
      uint8_t nextBit = getCombinedPatternBit(row, scrollIndex);

      // Shift the current pattern to the left by one bit and add the new bit
      cur_pattern[row] = (cur_pattern[row] << 1) | nextBit;
    }

    // Increment the scroll index and wrap it around the total sequence length
    scrollIndex = (scrollIndex + 1) % (timeString.length() * 8 + 16); // Include spaces for better separation
  }
}

// Function to get the appropriate bit from the combined sequence of timer string and spaces
uint8_t getCombinedPatternBit(int row, int offset) {
  int charIndex = offset / 8;
  int bitIndex = 7 - (offset % 8);

  if (charIndex >= timeString.length()) {
    return 0; // Return a blank space if beyond the string length
  }

  char c = timeString.charAt(charIndex);
  int patternIndex = (c == ':') ? 10 : (c - '0');
  return (digitPatterns[patternIndex][row] & (1 << bitIndex)) ? 1 : 0;
}

// Function to update the LED matrix display based on the pattern array
void updateDisplay() {
  if (!keywordModeA && !keywordModeB&& !keywordSetMode) {
    if (!timerSettingMode) {
      for (int row = 0; row < 7; row++) {
        // Set the columns according to the pattern
        for (int col = 0; col < 8; col++) {
          if (cur_pattern[row] & (1 << (7 - col))) {
            digitalWrite(cols[col], LOW);  // Activate the specific column
          } else {
            digitalWrite(cols[col], HIGH); // Deactivate the other columns
          }
        }

        // Set the current row to HIGH to activate it
        digitalWrite(rows[row], HIGH);

        // Small delay to make the row visible
        delay(1);

        // Reset the current row back to LOW to deactivate it
        digitalWrite(rows[row], LOW);
      }

    } else {
      unsigned long currentTime = millis();

      // Handle row updates for the current frame
      if (currentTime - previousRowTime >= rowDelay) {
        previousRowTime = currentTime;
        clearMatrix();

        if (displayOn) {
          // Set the columns according to the pattern
          for (int col = 0; col < 8; col++) {
            if (cur_pattern[currentRow] & (1 << (7 - col))) {
              digitalWrite(cols[col], LOW);  // Activate the specific column
            } else {
              digitalWrite(cols[col], HIGH); // Deactivate the other columns
            }
          }

          // Activate the current row
          digitalWrite(rows[currentRow], HIGH);
        }

        // Move to the next row
        currentRow = (currentRow + 1) % 7;
      }

      // Handle flashing state
      if (currentTime - previousFlashTime >= flashDelay) {
        previousFlashTime = currentTime;

        // Toggle display state
        displayOn = !displayOn;
      }
    }  
  }
}

// Function to clear the matrix
void clearMatrix() {
  for (int row = 0; row < 7; row++) {
    for (int col = 0; col < 8; col++) {
      digitalWrite(rows[row], LOW);
      digitalWrite(cols[col], HIGH);
    }
  }
}

// Function to play alarm sound: on -- off -- on -- off
void playAlarm() {
  if (soundPlaying){
    static bool toneOn = false;
    static unsigned long lastToneMillis = 0;
    if (millis() - lastToneMillis >= 100) {
      lastToneMillis = millis();
      if (toneOn) {
        noTone(SPEAKER_PIN); // Turn off the tone
      } else {
        tone(SPEAKER_PIN, 2000); // 2 kHz sound
      }
      toneOn = !toneOn;
    }
  }
}

// manage digit flashing for keywordSetMode
void flashDisplayDigit(int digit) {
  unsigned long currentTime = millis();

  // Handle row updates for the current frame
  if (currentTime - previousRowTime >= 1) {
    previousRowTime = currentTime; // Update the row timing
    clearMatrix(); // Clear the matrix for the current frame

    if (displayOn) { // If display is active
      // Set the columns according to the digit pattern for the current row
      for (int col = 0; col < 8; col++) {
        if (digitPatterns[digit][currentRow] & (1 << (7 - col))) {
          digitalWrite(cols[col], LOW);  // Activate the specific column
        } else {
          digitalWrite(cols[col], HIGH); // Deactivate the column
        }
      }

      // Activate the current row
      digitalWrite(rows[currentRow], HIGH);
    }

    // Move to the next row
    currentRow = (currentRow + 1) % 7;
  }

  // Handle flashing state based on the defined flash delay
  if (currentTime - previousFlashTime >= flashDelay) {
    previousFlashTime = currentTime;

    // Toggle display state
    displayOn = !displayOn;
  }
}

// Function to display a single digit without flashing, for keywords
void displayDigit(int digit) {
  clearMatrix();
  for (int row = 0; row < 7; row++) {
    // Set the columns according to the digit pattern
    for (int col = 0; col < 8; col++) {
      if (digitPatterns[digit][row] & (1 << (7 - col))) {
        digitalWrite(cols[col], LOW);  // Activate the specific column
      } else {
        digitalWrite(cols[col], HIGH); // Deactivate the other columns
      }
    }

    // Set the current row to HIGH to activate it
    digitalWrite(rows[row], HIGH);

    // Small delay to make the row visible
    delay(1);

    // Reset the current row back to LOW to deactivate it
    digitalWrite(rows[row], LOW);
  }
}