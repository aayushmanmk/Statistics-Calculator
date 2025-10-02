// ISC-11 Statistics Calculator with RGB LED Data Visualization
// 16x2 I2C LCD, 2 servos (mean D9, spread D10), RGB LED (R:6, G:7, B:8)
// Buttons: UP D2, DOWN D3, ENTER D4, MODE D5 (internal pull-up)
// LCD I2C 0x27 SDA A4 SCL A5

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <math.h>
#include <avr/pgmspace.h>

// System constants
#define NUM_BINS 16
#define MAX_CUSTOM_CHARS 8
#define DEBOUNCE 150
#define REPEAT_DELAY 80
#define REPEAT_RATE 28
#define ACCEL_THRESHOLD1 700
#define ACCEL_RATE1 14
#define ACCEL_THRESHOLD2 2000
#define ACCEL_RATE2 8
#define LONG_PRESS 900
#define DISPLAY_UPDATE_INTERVAL 1000
#define MODE_COOLDOWN 300
#define ANIMATION_DURATION 300
#define SCROLL_DELAY 300
#define CREDITS_DISPLAY_TIME 10000
#define INFO_SCROLL_DELAY 400
#define DINO_GAME_SPEED 200

// Pin definitions
#define BTN_UP 2
#define BTN_DOWN 3
#define BTN_ENTER 4
#define BTN_MODE 5
#define SERVO_MEAN_PIN 9
#define SERVO_SPREAD_PIN 10
#define LED_RED 6
#define LED_GREEN 7
#define LED_BLUE 8

// System states
enum SystemState { START_MENU, STOCK_SUBMENU, ENTERING_DATA, ENTERING_FREQUENCY, DISPLAYING_STATS, CREDITS_SCREEN, DINO_GAME };
SystemState currentState = START_MENU;

// Menu options
enum MenuOption { ENTER_DATA, EXAMPLE_DATA, HELP_INFO, CREDITS, INFO };
MenuOption menuSelection = ENTER_DATA;

// Stock options (extended to include height data)
enum StockOption { AAPL, NVDA, NKE, GOOGL, SONY, TTWO, MALE_HEIGHT, FEMALE_HEIGHT };
StockOption stockSelection = AAPL;

// Global objects
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo sMean, sSpread;

// Dynamic arrays for samples and frequencies
float* samples = NULL;
uint16_t* frequencies = NULL;
uint16_t n = 0;
uint16_t capacity = 0;
float curVal = 50.0;
uint16_t curFreq = 1;
uint8_t page = 0;
uint8_t infoSubPage = 0;

// Timing variables
unsigned long lastAction = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastButtonPress = 0;
unsigned long animationStart = 0;
bool animating = false;

// Button state tracking
unsigned long upPressedAt = 0;
unsigned long downPressedAt = 0;
unsigned long lastUpRepeat = 0;
unsigned long lastDownRepeat = 0;
unsigned long modePressedAt = 0;
unsigned long modeReleasedAt = 0;
unsigned long enterPressedAt = 0; // Added for tracking ENTER button long press

// Credits timing
unsigned long creditsStartTime = 0;
bool creditsPhase1Shown = false;

// Histogram tracking
bool isHistogramPage = false;

// Info page scrolling
unsigned long lastInfoScrollTime = 0;
uint8_t infoScrollPos = 0;

// Dino game variables
uint8_t dinoY = 1; // 0 = jumping, 1 = on ground
int8_t dinoVelocity = 0;
uint8_t cactusX = 15;
uint16_t dinoScore = 0;
bool dinoGameOver = false;
unsigned long lastDinoUpdate = 0;
bool bothButtonsPressed = false;

// Info texts stored in PROGMEM to save RAM
const char infoText_0[] PROGMEM = "Servos: Mean & Spread Visualization";
const char infoText_1[] PROGMEM = "RGB LED: Data Distribution Indicators";
const char infoText_2[] PROGMEM = "LCD Display: 16x2 Character Output";
const char infoText_3[] PROGMEM = "Control Buttons: Navigation Interface";

const char* const infoTexts[] PROGMEM = {
  infoText_0,
  infoText_1,
  infoText_2,
  infoText_3
};

uint8_t trackedRow = 0;

// Helper function to create characters from PROGMEM
void createCharFromProgmem(uint8_t index, const uint8_t* charData) {
  uint8_t tempChar[8];
  for (uint8_t i = 0; i < 8; i++) {
    tempChar[i] = pgm_read_byte(&charData[i]);
  }
  lcd.createChar(index, tempChar);
}

// Custom characters for UI - stored in PROGMEM
const uint8_t ch_small_block[8] PROGMEM = {0x00,0x04,0x0E,0x0E,0x0E,0x0E,0x04,0x00};
const uint8_t ch_full_block[8] PROGMEM  = {0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F};
const uint8_t ch_pointer[8] PROGMEM     = {0x00,0x04,0x06,0x07,0x06,0x04,0x00,0x00};
const uint8_t ch_arrow_up[8] PROGMEM    = {0x04,0x0E,0x1F,0x04,0x04,0x04,0x04,0x00};
const uint8_t ch_arrow_down[8] PROGMEM  = {0x04,0x04,0x04,0x04,0x1F,0x0E,0x04,0x00};
const uint8_t ch_check[8] PROGMEM       = {0x00,0x01,0x03,0x16,0x1C,0x08,0x00,0x00};
const uint8_t ch_progress[8] PROGMEM    = {0x00,0x00,0x1F,0x1F,0x1F,0x1F,0x00,0x00};
const uint8_t ch_filled_circle[8] PROGMEM = {0x00,0x0E,0x1F,0x1F,0x1F,0x0E,0x00,0x00};

// Dino game characters
const uint8_t ch_dino[8] PROGMEM = {
  0b00000,
  0b00100,
  0b00110,
  0b00111,
  0b00110,
  0b00100,
  0b00000,
  0b00000
};

const uint8_t ch_cactus[8] PROGMEM = {
  0b00100,
  0b00100,
  0b10101,
  0b10101,
  0b11111,
  0b00100,
  0b00100,
  0b00000
};

// Histogram bar characters
const uint8_t ch_bar_1[8] PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1F};
const uint8_t ch_bar_2[8] PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x00,0x1F,0x1F};
const uint8_t ch_bar_3[8] PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x1F,0x1F,0x1F};
const uint8_t ch_bar_4[8] PROGMEM = {0x00,0x00,0x00,0x00,0x1F,0x1F,0x1F,0x1F};
const uint8_t ch_bar_5[8] PROGMEM = {0x00,0x00,0x00,0x1F,0x1F,0x1F,0x1F,0x1F};
const uint8_t ch_bar_6[8] PROGMEM = {0x00,0x00,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F};
const uint8_t ch_bar_7[8] PROGMEM = {0x00,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F};
const uint8_t ch_bar_8[8] PROGMEM = {0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F};

// Info page custom characters
const uint8_t ch_servo[8] PROGMEM = {0x0E,0x1B,0x1B,0x0E,0x0E,0x0E,0x0E,0x0E};
const uint8_t ch_led[8] PROGMEM = {0x0E,0x1B,0x1B,0x0E,0x1F,0x1F,0x1F,0x1F};
const uint8_t ch_lcd[8] PROGMEM = {0x1F,0x1F,0x1F,0x00,0x00,0x1F,0x1F,0x1F};
const uint8_t ch_button[8] PROGMEM = {0x0E,0x1B,0x1B,0x0E,0x0E,0x0E,0x0E,0x00};
const uint8_t ch_info[8] PROGMEM = {0x0E,0x11,0x13,0x15,0x19,0x1D,0x1D,0x00};

// Example data stored in PROGMEM
const uint8_t EX_N = 20;
const float EX_AAPL[EX_N] PROGMEM = {
  221.34, 223.11, 219.88, 224.50, 226.30,
  228.75, 230.10, 227.40, 229.99, 232.45,
  235.10, 233.60, 234.80, 236.20, 238.00,
  240.50, 242.10, 241.75, 244.20, 246.00
};

const float EX_NVDA[EX_N] PROGMEM = {
  475.20, 482.50, 490.75, 495.30, 502.60,
  510.80, 505.40, 512.90, 520.30, 515.70,
  525.40, 530.80, 528.20, 535.60, 542.10,
  538.90, 545.30, 552.70, 560.20, 565.80
};

const float EX_NKE[EX_N] PROGMEM = {
  105.30, 106.80, 104.50, 108.20, 110.50,
  112.30, 111.70, 114.20, 116.50, 115.80,
  118.20, 120.50, 119.70, 122.30, 124.60,
  123.80, 126.20, 128.50, 130.20, 132.70
};

const float EX_GOOGL[EX_N] PROGMEM = {
  135.40, 137.20, 139.80, 142.50, 140.20,
  143.70, 145.90, 148.30, 146.50, 149.80,
  152.40, 150.70, 153.20, 155.60, 158.90,
  157.30, 160.50, 163.20, 165.80, 168.40
};

const float EX_SONY[EX_N] PROGMEM = {
  88.50, 90.20, 92.70, 94.50, 93.20,
  95.80, 97.40, 99.10, 101.50, 103.20,
  105.70, 107.40, 109.80, 112.30, 114.60,
  117.20, 119.50, 122.10, 124.80, 127.30
};

const float EX_TTWO[EX_N] PROGMEM = {
  155.80, 158.20, 160.50, 163.70, 165.40,
  168.90, 171.20, 173.50, 176.80, 179.20,
  182.50, 185.70, 188.30, 191.60, 194.20,
  197.50, 200.80, 203.40, 206.70, 209.50
};

// Male height data (cm) for different countries
const float EX_MALE_HEIGHT[EX_N] PROGMEM = {
  184.0, 183.5, 184.2, 183.8, 184.3,
  183.7, 184.1, 183.9, 184.4, 184.0,
  183.6, 184.5, 183.8, 184.2, 184.1,
  183.9, 184.3, 184.0, 183.7, 184.2
};

// Female height data (cm) for different countries
const float EX_FEMALE_HEIGHT[EX_N] PROGMEM = {
  170.0, 169.5, 170.2, 169.8, 170.3,
  169.7, 170.1, 169.9, 170.4, 170.0,
  169.6, 170.5, 169.8, 170.2, 170.1,
  169.9, 170.3, 170.0, 169.7, 170.2
};

// Function prototypes
void setup();
void loop();
void setCursorTracked(uint8_t col, uint8_t row);
void safeServosCenter();
void showStartupAnimation();
void showStartMenu();
void showStockSubmenu();
void showEntryScreen();
void showFrequencyScreen();
void handleButtons();
void incCurValImmediate();
void decCurValImmediate();
void incCurValByHold(unsigned long held);
void decCurValByHold(unsigned long held);
void incCurFreqImmediate();
void decCurFreqImmediate();
void incCurFreqByHold(unsigned long held);
void decCurFreqByHold(unsigned long held);
bool loadExampleData(StockOption stock);
const float* getStockData(StockOption stock);
const char* getStockName(StockOption stock);
void flashSaved();
void showPage(uint8_t p);
void computeStats(float &xmin, float &xmax, float &mean, float &mad, float &sampVar, float &sampSD, float &popVar, float &popSD, float &median, float &modeVal, uint16_t &modeCount);
void showCountPage();
void showMeanPage();
void showMinPage();
void showMaxPage();
void showStdDevPage();
void showVariancePage();
void showMadPage();
void showMedianPage();
void showModePage();
void showHistogramPage();
void showInfoPage();
void updateInfoPageScroll();
void printSmallFloat(float v, uint8_t colStart, uint8_t decimals = 1);
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
void moveServoSmooth(Servo &sv, uint8_t target);
void drawProgressBar(uint8_t col, uint8_t row, float value, float minVal, float maxVal, uint8_t width);
void drawArrow(uint8_t col, uint8_t row, bool up);
void animateTransition();
void resetToStartMenu();
void drawMeter(uint8_t col, uint8_t row, float value, float minVal, float maxVal, uint8_t width);
void printCentered(const __FlashStringHelper* text, uint8_t row);
void drawBarGraph(uint8_t col, uint8_t row, float value, float minVal, float maxVal, uint8_t width);
void setRGBLED(uint8_t r, uint8_t g, uint8_t b);
void pulseLED(uint8_t pin, uint16_t duration, uint8_t count);
void updateDataLEDs();
void pulseDataEntryLED();
void resizeSamplesArray(uint16_t newCapacity);
void freeSamplesArray();
void showCredits();
void updateCreditsDisplay();
void defineHistogramChars();
void restoreOriginalChars();
void setServosToStats();
void defineInfoChars();
void restoreInfoChars();
void showDinoGame();
void updateDinoGame();
void defineDinoChars();
void resetDinoGame();

void setup() {
  Serial.begin(9600);
  
  // Initialize I2C
  Wire.begin();
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  
  // Initialize buttons with internal pull-up
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_ENTER, INPUT_PULLUP);
  pinMode(BTN_MODE, INPUT_PULLUP);

  // Initialize servos
  sMean.attach(SERVO_MEAN_PIN);
  sSpread.attach(SERVO_SPREAD_PIN);
  
  safeServosCenter();

  // Initialize RGB LED
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  setRGBLED(0, 0, 0);

  // Create custom characters using helper function
  createCharFromProgmem(0, ch_small_block);
  createCharFromProgmem(1, ch_full_block);
  createCharFromProgmem(2, ch_pointer);
  createCharFromProgmem(3, ch_arrow_up);
  createCharFromProgmem(4, ch_arrow_down);
  createCharFromProgmem(5, ch_check);
  createCharFromProgmem(6, ch_progress);
  createCharFromProgmem(7, ch_filled_circle);

  showStartupAnimation();
  showStartMenu();
}

void loop() {
  handleButtons();
  
  // Update display periodically when not in entry mode
  if (currentState == DISPLAYING_STATS && (millis() - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL)) {
    // Skip update for median, mode, histogram, and info pages to prevent flashing
    if (page != 7 && page != 8 && page != 9 && page != 10) {
      showPage(page);
      lastDisplayUpdate = millis();
    }
  }
  
  // Handle animations
  if (animating && millis() - animationStart > ANIMATION_DURATION) {
    animating = false;
    if (currentState == START_MENU) {
      showStartMenu();
    } else if (currentState == STOCK_SUBMENU) {
      showStockSubmenu();
    } else if (currentState == ENTERING_DATA) {
      showEntryScreen();
    } else if (currentState == ENTERING_FREQUENCY) {
      showFrequencyScreen();
    } else {
      showPage(page);
    }
  }
  
  // Update credits display if in credits state
  if (currentState == CREDITS_SCREEN) {
    updateCreditsDisplay();
  }
  
  // Update info page scrolling
  if (currentState == DISPLAYING_STATS && page == 10) {
    if (millis() - lastInfoScrollTime > INFO_SCROLL_DELAY) {
      updateInfoPageScroll();
      lastInfoScrollTime = millis();
    }
  }
  
  // Update dino game if in game state
  if (currentState == DINO_GAME) {
    if (millis() - lastDinoUpdate > DINO_GAME_SPEED) {
      updateDinoGame();
      lastDinoUpdate = millis();
    }
  }
}

void resetToStartMenu() {
  // Restore original characters if we were on histogram or info page
  if (isHistogramPage || page == 10) {
    restoreOriginalChars();
    isHistogramPage = false;
  }
  
  currentState = START_MENU;
  menuSelection = ENTER_DATA;
  page = 0;
  infoSubPage = 0;
  curVal = 50.0;
  curFreq = 1;
  safeServosCenter();
  setRGBLED(0, 0, 0);
  
  // Free the samples and frequencies arrays
  freeSamplesArray();
  
  lcd.clear();
  printCentered(F("Returning to menu"), 0);
  delay(500);
  animateTransition();
  showStartMenu();
}

void setCursorTracked(uint8_t col, uint8_t row) {
  trackedRow = row;
  lcd.setCursor(col, row);
}

void safeServosCenter() {
  sMean.write(180);
  sSpread.write(180);
  delay(40);
}

void showStartupAnimation() {
  lcd.clear();
  
  // RGB LED animation
  setRGBLED(255, 0, 0);
  delay(200);
  setRGBLED(0, 255, 0);
  delay(200);
  setRGBLED(0, 0, 255);
  delay(200);
  setRGBLED(255, 255, 255);
  delay(200);
  setRGBLED(0, 0, 0);
  
  // LCD animation
  for (uint8_t i = 0; i <= 16; i++) {
    lcd.clear();
    setCursorTracked(0,0);
    for (uint8_t j = 0; j < i && j < 16; j++) {
      lcd.print("=");
    }
    setCursorTracked(0,1);
    lcd.print(F("ISC11 Statistics"));
    delay(50);
  }
  
  delay(500);
  animateTransition();
}

void showStartMenu() {
  currentState = START_MENU;
  lcd.clear();
  
  // Title
  setCursorTracked(0,0);
  lcd.print(F("* Main Menu"));
  
  // Show selected option with indicator
  setCursorTracked(0,1);
  lcd.write(byte(2));
  lcd.print(" ");
  
  // Print option text
  switch(menuSelection) {
    case ENTER_DATA:
      lcd.print(F("Enter Data"));
      break;
    case EXAMPLE_DATA:
      lcd.print(F("Examples"));
      break;
    case HELP_INFO:
      lcd.print(F("Help"));
      break;
    case CREDITS:
      lcd.print(F("Credits"));
      break;
    case INFO:
      lcd.print(F("Info"));
      break;
  }
  
  // Visual indicator for menu selection
  drawProgressBar(14, 1, menuSelection, 0, 4, 2);
  
  setRGBLED(50, 50, 50);
}

void showStockSubmenu() {
  currentState = STOCK_SUBMENU;
  lcd.clear();
  
  // Title
  setCursorTracked(0,0);
  lcd.print(F("* Stock Examples"));
  
  // Show selected stock with indicator
  setCursorTracked(0,1);
  lcd.write(byte(2));
  lcd.print(" ");
  lcd.print(getStockName(stockSelection));
  
  // Visual indicator for stock selection (now 8 options)
  drawProgressBar(14, 1, stockSelection, 0, 7, 2);
  
  setRGBLED(100, 0, 100);
}

const char* getStockName(StockOption stock) {
  switch(stock) {
    case AAPL: return "AAPL";
    case NVDA: return "NVDA";
    case NKE: return "NKE";
    case GOOGL: return "GOOGL";
    case SONY: return "SONY";
    case TTWO: return "TTWO";
    case MALE_HEIGHT: return "M_HT";
    case FEMALE_HEIGHT: return "F_HT";
    default: return "UNKNOWN";
  }
}

const float* getStockData(StockOption stock) {
  switch(stock) {
    case AAPL: return EX_AAPL;
    case NVDA: return EX_NVDA;
    case NKE: return EX_NKE;
    case GOOGL: return EX_GOOGL;
    case SONY: return EX_SONY;
    case TTWO: return EX_TTWO;
    case MALE_HEIGHT: return EX_MALE_HEIGHT;
    case FEMALE_HEIGHT: return EX_FEMALE_HEIGHT;
    default: return EX_AAPL;
  }
}

void showEntryScreen() {
  currentState = ENTERING_DATA;
  lcd.clear();
  
  // Title
  setCursorTracked(0,0);
  lcd.print(F("Value:"));
  
  // Format value with padding
  char buf[8];
  dtostrf(curVal, 0, 2, buf);
  uint8_t len = strlen(buf);
  uint8_t pad = (6 > len) ? (6 - len) : 0;
  for (uint8_t i = 0; i < pad; i++) lcd.print(' ');
  lcd.print(buf);
  
  // Visual meter for value
  drawMeter(12, 0, curVal, 0, 100, 4);
  
  // Entry count with visual indicator
  setCursorTracked(0,1);
  lcd.print(F("N:"));
  if (n < 10) lcd.print(' ');
  lcd.print(n);
  
  // Progress indicator
  uint8_t progress = (capacity > 0) ? map(n, 0, capacity, 0, 4) : 0;
  setCursorTracked(10,1);
  lcd.print(F("Mem:"));
  setCursorTracked(14,1);
  if (progress > 0) lcd.write(byte(1));
  else lcd.print(' ');
  
  setRGBLED(0, 30, 0);
}

void showFrequencyScreen() {
  currentState = ENTERING_FREQUENCY;
  lcd.clear();
  
  // Title
  setCursorTracked(0,0);
  lcd.print(F("Value:"));
  
  // Format value with padding
  char buf[8];
  dtostrf(curVal, 0, 2, buf);
  uint8_t len = strlen(buf);
  uint8_t pad = (6 > len) ? (6 - len) : 0;
  for (uint8_t i = 0; i < pad; i++) lcd.print(' ');
  lcd.print(buf);
  
  // Frequency entry
  setCursorTracked(0,1);
  lcd.print(F("Freq:"));
  if (curFreq < 10) lcd.print(' ');
  if (curFreq < 100) lcd.print(' ');
  lcd.print(curFreq);
  
  // Visual meter for frequency
  drawMeter(12, 1, curFreq, 1, 50, 4);
  
  setRGBLED(30, 0, 30);
}

void handleButtons() {
  unsigned long now = millis();
  bool buttonPressed = false;
  
  // Check for both buttons pressed (easter egg trigger)
  bool upPressed = (digitalRead(BTN_UP) == LOW);
  bool downPressed = (digitalRead(BTN_DOWN) == LOW);
  
  if (currentState == CREDITS_SCREEN && upPressed && downPressed) {
    if (!bothButtonsPressed) {
      bothButtonsPressed = true;
      // Start dino game
      currentState = DINO_GAME;
      resetDinoGame();
      defineDinoChars();
      showDinoGame();
      lastAction = now;
    }
  } else {
    bothButtonsPressed = false;
  }
  
  // Handle UP button
  if (upPressed) {
    buttonPressed = true;
    if (upPressedAt == 0) {
      upPressedAt = now;
      lastUpRepeat = now;
      
      if (currentState == START_MENU) {
        menuSelection = (MenuOption)((menuSelection - 1 + 5) % 5);
        showStartMenu();
        pulseLED(LED_BLUE, 100, 1);
      } else if (currentState == STOCK_SUBMENU) {
        stockSelection = (StockOption)((stockSelection - 1 + 8) % 8);
        showStockSubmenu();
        pulseLED(LED_BLUE, 100, 1);
      } else if (currentState == ENTERING_DATA) {
        incCurValImmediate();
        showEntryScreen();
      } else if (currentState == ENTERING_FREQUENCY) {
        incCurFreqImmediate();
        showFrequencyScreen();
      } else if (currentState == DISPLAYING_STATS && page == 10) {
        infoSubPage = (infoSubPage - 1 + 4) % 4;
        infoScrollPos = 0;
        showInfoPage();
      } else if (currentState == DINO_GAME && dinoY == 1 && !dinoGameOver) {
        // Jump in dino game
        dinoVelocity = 4;
        dinoY = 0; // Set to jumping position
      }
      
      lastAction = now;
      lastButtonPress = now;
    } else {
      unsigned long held = now - upPressedAt;
      unsigned long since = now - lastUpRepeat;
      unsigned long threshold = (held >= ACCEL_THRESHOLD2) ? ACCEL_RATE2 : 
                                (held >= ACCEL_THRESHOLD1) ? ACCEL_RATE1 : REPEAT_RATE;
      
      if (held >= REPEAT_DELAY && since >= threshold) {
        if (currentState == ENTERING_DATA) {
          incCurValByHold(held);
          showEntryScreen();
        } else if (currentState == ENTERING_FREQUENCY) {
          incCurFreqByHold(held);
          showFrequencyScreen();
        }
        lastUpRepeat = now;
        lastButtonPress = now;
      }
    }
  } else {
    upPressedAt = 0;
  }
  
  // Handle DOWN button
  if (downPressed) {
    buttonPressed = true;
    if (downPressedAt == 0) {
      downPressedAt = now;
      lastDownRepeat = now;
      
      if (currentState == START_MENU) {
        menuSelection = (MenuOption)((menuSelection + 1) % 5);
        showStartMenu();
        pulseLED(LED_BLUE, 100, 1);
      } else if (currentState == STOCK_SUBMENU) {
        stockSelection = (StockOption)((stockSelection + 1) % 8);
        showStockSubmenu();
        pulseLED(LED_BLUE, 100, 1);
      } else if (currentState == ENTERING_DATA) {
        decCurValImmediate();
        showEntryScreen();
      } else if (currentState == ENTERING_FREQUENCY) {
        decCurFreqImmediate();
        showFrequencyScreen();
      } else if (currentState == DISPLAYING_STATS && page == 10) {
        infoSubPage = (infoSubPage + 1) % 4;
        infoScrollPos = 0;
        showInfoPage();
      }
      
      lastAction = now;
      lastButtonPress = now;
    } else {
      unsigned long held = now - downPressedAt;
      unsigned long since = now - lastDownRepeat;
      unsigned long threshold = (held >= ACCEL_THRESHOLD2) ? ACCEL_RATE2 : 
                                (held >= ACCEL_THRESHOLD1) ? ACCEL_RATE1 : REPEAT_RATE;
      
      if (held >= REPEAT_DELAY && since >= threshold) {
        if (currentState == ENTERING_DATA) {
          decCurValByHold(held);
          showEntryScreen();
        } else if (currentState == ENTERING_FREQUENCY) {
          decCurFreqByHold(held);
          showFrequencyScreen();
        }
        lastDownRepeat = now;
        lastButtonPress = now;
      }
    }
  } else {
    downPressedAt = 0;
  }
  
  // Handle ENTER button
  if (digitalRead(BTN_ENTER) == LOW) {
    buttonPressed = true;
    if (enterPressedAt == 0) {
      enterPressedAt = now;
    } else if (now - enterPressedAt >= LONG_PRESS) {
      // Long press detected
      if (currentState == ENTERING_DATA && n > 0) {
        // Remove last entry
        n--;
        lcd.clear();
        setCursorTracked(0,0);
        lcd.print(F("Entry removed!"));
        setCursorTracked(0,1);
        lcd.print(F("N: "));
        lcd.print(n);
        delay(1000);
        showEntryScreen();
      } else if (currentState == ENTERING_FREQUENCY && n > 0) {
        // Remove last entry
        n--;
        lcd.clear();
        setCursorTracked(0,0);
        lcd.print(F("Entry removed!"));
        setCursorTracked(0,1);
        lcd.print(F("N: "));
        lcd.print(n);
        delay(1000);
        showEntryScreen();
      }
      enterPressedAt = 0; // Reset to prevent repeated triggering
      lastAction = now;
      lastButtonPress = now;
    }
    
    if (now - lastAction >= DEBOUNCE && enterPressedAt != 0 && now - enterPressedAt < LONG_PRESS) {
      // Short press
      if (currentState == START_MENU) {
        switch(menuSelection) {
          case ENTER_DATA:
            currentState = ENTERING_DATA;
            n = 0;
            resizeSamplesArray(10);
            animateTransition();
            showEntryScreen();
            setRGBLED(0, 255, 0);
            break;
          case EXAMPLE_DATA:
            animateTransition();
            showStockSubmenu();
            break;
          case HELP_INFO:
            lcd.clear();
            printCentered(F("Help: Use UP/DOWN"), 0);
            printCentered(F("to navigate pages"), 1);
            setRGBLED(255, 255, 0);
            delay(2000);
            showStartMenu();
            break;
          case CREDITS:
            showCredits();
            break;
          case INFO:
            currentState = DISPLAYING_STATS;
            page = 10;
            infoSubPage = 0;
            infoScrollPos = 0;
            animateTransition();
            showInfoPage();
            break;
        }
      } else if (currentState == ENTERING_DATA) {
        // Move to frequency entry screen
        animateTransition();
        showFrequencyScreen();
      } else if (currentState == ENTERING_FREQUENCY) {
        // Add the value with its frequency
        if (n >= capacity) {
          resizeSamplesArray(capacity * 2);
        }
        samples[n] = curVal;
        frequencies[n] = curFreq;
        n++;
        flashSaved();
        pulseDataEntryLED();
        // Reset frequency for next entry
        curFreq = 1;
        // Go back to value entry screen
        animateTransition();
        showEntryScreen();
      } else if (currentState == STOCK_SUBMENU) {
        if (loadExampleData(stockSelection)) {
          currentState = DISPLAYING_STATS;
          page = 0;
          animateTransition();
          showPage(page);
        }
      } else if (currentState == DINO_GAME) {
        if (dinoGameOver) {
          resetDinoGame();
          showDinoGame();
        } else if (dinoY == 1) {
          // Jump in dino game
          dinoVelocity = 4;
          dinoY = 0; // Set to jumping position
        }
      }
      
      lastAction = now;
      lastButtonPress = now;
    }
    
    // Wait for button release
    unsigned long startWait = millis();
    while(digitalRead(BTN_ENTER) == LOW) {
      delay(6);
      if (millis() - startWait > 1000) break;
    }
    enterPressedAt = 0; // Reset after button release
    return;
  } else {
    enterPressedAt = 0;
  }
  
  // Handle MODE button
  if (digitalRead(BTN_MODE) == LOW) {
    buttonPressed = true;
    if (modePressedAt == 0) {
      modePressedAt = now;
    }
    return;
  } else {
    if (modePressedAt != 0) {
      unsigned long held = now - modePressedAt;
      modePressedAt = 0;
      modeReleasedAt = now;
      
      if (modeReleasedAt - lastAction < MODE_COOLDOWN) {
        return;
      }
      
      if (held >= LONG_PRESS) {
        if (isHistogramPage || page == 10 || currentState == DINO_GAME) {
          restoreOriginalChars();
          isHistogramPage = false;
        }
        resetToStartMenu();
        pulseLED(LED_RED, 100, 2);
      } else {
        if (currentState == ENTERING_FREQUENCY) {
          // Cancel frequency entry and go back to value entry
          curFreq = 1;
          animateTransition();
          showEntryScreen();
        } else if (currentState == ENTERING_DATA) {
          if (n > 0) {
            currentState = DISPLAYING_STATS;
            page = 0;
            setServosToStats();
            animateTransition();
            showPage(page);
          }
        } else if (currentState == DISPLAYING_STATS) {
          if (isHistogramPage || page == 10) {
            restoreOriginalChars();
            isHistogramPage = false;
          }
          
          // Special handling for info page - return to start menu instead of cycling
          if (page == 10) {
            resetToStartMenu();
          } else {
            page = (page + 1) % 10; // Changed from 11 to 10 to prevent going to info page
            animateTransition();
            showPage(page);
          }
        } else if (currentState == CREDITS_SCREEN || currentState == STOCK_SUBMENU) {
          if (isHistogramPage || page == 10 || currentState == DINO_GAME) {
            restoreOriginalChars();
            isHistogramPage = false;
          }
          resetToStartMenu();
        } else if (currentState == DINO_GAME) {
          restoreOriginalChars();
          resetToStartMenu();
        }
        lastAction = now;
      }
      lastButtonPress = now;
    }
  }
  
  if (buttonPressed) {
    lastButtonPress = now;
  }
}

void resizeSamplesArray(uint16_t newCapacity) {
  float* newSamples = new float[newCapacity];
  uint16_t* newFrequencies = new uint16_t[newCapacity];
  
  if (newSamples == NULL || newFrequencies == NULL) {
    Serial.println(F("Memory allocation failed!"));
    if (newSamples != NULL) delete[] newSamples;
    if (newFrequencies != NULL) delete[] newFrequencies;
    return;
  }
  
  for (uint16_t i = 0; i < n; i++) {
    newSamples[i] = samples[i];
    newFrequencies[i] = frequencies[i];
  }
  
  if (samples != NULL) {
    delete[] samples;
  }
  
  if (frequencies != NULL) {
    delete[] frequencies;
  }
  
  samples = newSamples;
  frequencies = newFrequencies;
  capacity = newCapacity;
}

void freeSamplesArray() {
  if (samples != NULL) {
    delete[] samples;
    samples = NULL;
  }
  if (frequencies != NULL) {
    delete[] frequencies;
    frequencies = NULL;
  }
  n = 0;
  capacity = 0;
}

void incCurValImmediate() { 
  curVal += 0.1; 
  if (curVal > 999.9) curVal = 999.9;
}

void decCurValImmediate() { 
  curVal -= 0.1; 
  if (curVal < 0) curVal = 0;
}

void incCurValByHold(unsigned long held) {
  if (held < 800) curVal += 0.3;
  else if (held < 2200) curVal += 1.0;
  else if (held < 6000) curVal += 5.0;
  else curVal += 10.0;
  
  if (curVal > 999.9) curVal = 999.9;
}

void decCurValByHold(unsigned long held) {
  if (held < 800) curVal -= 0.3;
  else if (held < 2200) curVal -= 1.0;
  else if (held < 6000) curVal -= 5.0;
  else curVal -= 10.0;
  
  if (curVal < 0) curVal = 0;
}

void incCurFreqImmediate() { 
  curFreq++; 
  if (curFreq > 999) curFreq = 999;
}

void decCurFreqImmediate() { 
  if (curFreq > 1) curFreq--; 
}

void incCurFreqByHold(unsigned long held) {
  if (held < 800) curFreq += 1;
  else if (held < 2200) curFreq += 5;
  else if (held < 6000) curFreq += 10;
  else curFreq += 50;
  
  if (curFreq > 999) curFreq = 999;
}

void decCurFreqByHold(unsigned long held) {
  if (held < 800) curFreq -= 1;
  else if (held < 2200) curFreq -= 5;
  else if (held < 6000) curFreq -= 10;
  else curFreq -= 50;
  
  if (curFreq < 1) curFreq = 1;
}

bool loadExampleData(StockOption stock) {
  freeSamplesArray();
  
  capacity = EX_N;
  samples = new float[capacity];
  frequencies = new uint16_t[capacity];
  
  if (samples == NULL || frequencies == NULL) {
    Serial.println(F("Memory allocation failed for example data!"));
    if (samples != NULL) delete[] samples;
    if (frequencies != NULL) delete[] frequencies;
    lcd.clear();
    printCentered(F("Memory Error!"), 0);
    printCentered(F("Try reset"), 1);
    setRGBLED(255, 0, 0);
    delay(2000);
    return false;
  }
  
  const float* stockData = getStockData(stock);
  for (uint8_t i = 0; i < EX_N; i++) {
    samples[i] = pgm_read_float(&stockData[i]);
    frequencies[i] = 1; // Default frequency of 1 for example data
  }
  n = EX_N;
  
  lcd.clear();
  printCentered(F("Example Loaded!"), 0);
  setCursorTracked(0,1);
  lcd.print(getStockName(stockSelection));
  
  // Show description for height data
  if (stockSelection == MALE_HEIGHT) {
    lcd.print(F(" Male Height"));
  } else if (stockSelection == FEMALE_HEIGHT) {
    lcd.print(F(" Female Height"));
  } else {
    lcd.print(F(" Stock Data"));
  }
  
  setRGBLED(0, 255, 0);
  delay(1500);
  
  setServosToStats();
  
  return true;
}

void flashSaved() {
  lcd.clear();
  
  for (uint8_t i = 0; i < 3; i++) {
    setCursorTracked(0,0);
    lcd.print(F("Saved: "));
    char buf[8];
    dtostrf(curVal, 0, 2, buf);
    lcd.print(buf);
    lcd.print(F(" x"));
    lcd.print(curFreq);
    
    setCursorTracked(0,1);
    if (i % 2 == 0) {
      lcd.write(byte(5));
    } else {
      lcd.print(" ");
    }
    lcd.print(F(" Total: "));
    lcd.print(n);
    lcd.print(F(" values"));
    delay(200);
  }
  
  animateTransition();
  showEntryScreen();
}

void showPage(uint8_t p) {
  if ((isHistogramPage || page == 10) && p != 9) {
    restoreOriginalChars();
    isHistogramPage = false;
  }
  
  currentState = DISPLAYING_STATS;
  lastDisplayUpdate = millis();
  
  if (n == 0 && p != 10) {
    lcd.clear();
    printCentered(F("No data available"), 0);
    printCentered(F("Enter values first"), 1);
    delay(1000);
    resetToStartMenu();
    return;
  }
  
  switch(p) {
    case 0: showCountPage(); break;
    case 1: showMeanPage(); break;
    case 2: showMinPage(); break;
    case 3: showMaxPage(); break;
    case 4: showStdDevPage(); break;
    case 5: showVariancePage(); break;
    case 6: showMadPage(); break;
    case 7: showMedianPage(); break;
    case 8: showModePage(); break;
    case 9: showHistogramPage(); break;
    case 10: showInfoPage(); break;
  }
}

void computeStats(float &xmin, float &xmax, float &mean, float &mad, float &sampVar, float &sampSD, float &popVar, float &popSD, float &median, float &modeVal, uint16_t &modeCount) {
  if (n == 0) {
    xmin = xmax = mean = mad = sampVar = sampSD = popVar = popSD = median = modeVal = 0;
    modeCount = 0;
    return;
  }
  
  // First, calculate the total number of data points (considering frequencies)
  uint32_t totalPoints = 0;
  for (uint16_t i = 0; i < n; i++) {
    totalPoints += frequencies[i];
  }
  
  if (totalPoints == 0) {
    xmin = xmax = mean = mad = sampVar = sampSD = popVar = popSD = median = modeVal = 0;
    modeCount = 0;
    return;
  }
  
  xmin = xmax = samples[0];
  float sum = 0;
  float sumOfSquares = 0;
  
  // First pass: calculate min, max, sum, and sum of squares (considering frequencies)
  for (uint16_t i = 0; i < n; i++) {
    if (samples[i] < xmin) xmin = samples[i];
    if (samples[i] > xmax) xmax = samples[i];
    sum += samples[i] * frequencies[i];
    sumOfSquares += samples[i] * samples[i] * frequencies[i];
  }
  
  // Calculate mean
  mean = sum / totalPoints;
  
  // Calculate sum of squared differences from the mean (considering frequencies)
  float sumSquaredDiff = 0;
  for (uint16_t i = 0; i < n; i++) {
    float diff = samples[i] - mean;
    sumSquaredDiff += diff * diff * frequencies[i];
  }
  
  // Calculate population variance: σ² = Σ(x - μ)² / N
  popVar = sumSquaredDiff / totalPoints;
  
  // Calculate sample variance: s² = Σ(x - μ)² / (N - 1)
  if (totalPoints > 1) {
    sampVar = sumSquaredDiff / (totalPoints - 1);
  } else {
    sampVar = 0;
  }
  
  // Calculate standard deviations
  popSD = sqrt(popVar);
  sampSD = sqrt(sampVar);
  
  // Calculate MAD (Mean Absolute Deviation)
  mad = 0;
  for (uint16_t i = 0; i < n; i++) {
    mad += fabs(samples[i] - mean) * frequencies[i];
  }
  mad /= totalPoints;
  
  // Calculate median and mode
  // Create an expanded array with repeated values based on frequencies
  float* expanded = new float[totalPoints];
  if (expanded == NULL) {
    Serial.println(F("Memory allocation failed for expanded array!"));
    return;
  }
  
  uint32_t index = 0;
  for (uint16_t i = 0; i < n; i++) {
    for (uint16_t j = 0; j < frequencies[i]; j++) {
      expanded[index++] = samples[i];
    }
  }
  
  // Sort the expanded array
  for (uint32_t i = 1; i < totalPoints; i++) {
    float key = expanded[i];
    int32_t j = i - 1;
    while (j >= 0 && expanded[j] > key) {
      expanded[j + 1] = expanded[j];
      j--;
    }
    expanded[j + 1] = key;
  }
  
  // Calculate median
  median = (totalPoints % 2 == 1) ? expanded[totalPoints / 2] : 
           (expanded[totalPoints / 2 - 1] + expanded[totalPoints / 2]) / 2.0;
  
  // Calculate mode
  modeVal = expanded[0];
  modeCount = 1;
  uint16_t currentCount = 1;
  
  for (uint32_t i = 1; i < totalPoints; i++) {
    if (fabs(expanded[i] - expanded[i-1]) < 0.0001) {
      currentCount++;
    } else {
      if (currentCount > modeCount) {
        modeCount = currentCount;
        modeVal = expanded[i-1];
      }
      currentCount = 1;
    }
  }
  
  // Check the last group
  if (currentCount > modeCount) {
    modeCount = currentCount;
    modeVal = expanded[totalPoints-1];
  }
  
  delete[] expanded;
}

void showCountPage() {
  // Calculate total number of data points (considering frequencies)
  uint32_t totalPoints = 0;
  for (uint16_t i = 0; i < n; i++) {
    totalPoints += frequencies[i];
  }
  
  lcd.clear();
  setCursorTracked(0, 0);
  lcd.print(F("+ Count"));
  setCursorTracked(11, 0);
  lcd.print(F("1/10"));
  
  setCursorTracked(0, 1);
  lcd.print(F("Items: "));
  lcd.print(n);
  lcd.print(F(" Total:"));
  if (totalPoints < 1000) lcd.print(' ');
  lcd.print(totalPoints);
  
  updateDataLEDs();
}

void showMeanPage() {
  float xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal;
  uint16_t modeCount;
  computeStats(xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal, modeCount);
  
  lcd.clear();
  setCursorTracked(0, 0);
  lcd.print(F("+ Mean"));
  setCursorTracked(11, 0);
  lcd.print(F("2/10"));
  
  setCursorTracked(0, 1);
  lcd.print(F("Mean: "));
  printSmallFloat(mean, 6);
  
  updateDataLEDs();
}

void showMinPage() {
  float xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal;
  uint16_t modeCount;
  computeStats(xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal, modeCount);
  
  lcd.clear();
  setCursorTracked(0, 0);
  lcd.print(F("+ Minimum"));
  setCursorTracked(11, 0);
  lcd.print(F("3/10"));
  
  setCursorTracked(0, 1);
  lcd.print(F("Min: "));
  printSmallFloat(xmin, 5);
  
  updateDataLEDs();
}

void showMaxPage() {
  float xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal;
  uint16_t modeCount;
  computeStats(xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal, modeCount);
  
  lcd.clear();
  setCursorTracked(0, 0);
  lcd.print(F("+ Maximum"));
  setCursorTracked(11, 0);
  lcd.print(F("4/10"));
  
  setCursorTracked(0, 1);
  lcd.print(F("Max: "));
  printSmallFloat(xmax, 5);
  
  updateDataLEDs();
}

void showStdDevPage() {
  float xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal;
  uint16_t modeCount;
  computeStats(xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal, modeCount);
  
  lcd.clear();
  setCursorTracked(0, 0);
  lcd.print(F("+ Std Dev"));
  setCursorTracked(11, 0);
  lcd.print(F("5/10"));
  
  setCursorTracked(0, 1);
  lcd.print(F("S:"));
  printSmallFloat(sampSD, 3, 3);
  lcd.print(F(" P:"));
  printSmallFloat(popSD, 10, 3);
  
  updateDataLEDs();
}

void showVariancePage() {
  float xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal;
  uint16_t modeCount;
  computeStats(xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal, modeCount);
  
  lcd.clear();
  setCursorTracked(0, 0);
  lcd.print(F("+ Variance"));
  setCursorTracked(11, 0);
  lcd.print(F("6/10"));
  
  setCursorTracked(0, 1);
  lcd.print(F("S:"));
  printSmallFloat(sampVar, 3, 3);
  lcd.print(F(" P:"));
  printSmallFloat(popVar, 10, 3);
  
  updateDataLEDs();
}

void showMadPage() {
  float xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal;
  uint16_t modeCount;
  computeStats(xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal, modeCount);
  
  lcd.clear();
  setCursorTracked(0, 0);
  lcd.print(F("+ MAD"));
  setCursorTracked(11, 0);
  lcd.print(F("7/10"));
  
  setCursorTracked(0, 1);
  lcd.print(F("MAD: "));
  printSmallFloat(mad, 5);
  
  updateDataLEDs();
}

void showMedianPage() {
  float xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal;
  uint16_t modeCount;
  computeStats(xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal, modeCount);
  
  lcd.clear();
  setCursorTracked(0, 0);
  lcd.print(F("+ Median"));
  setCursorTracked(11, 0);
  lcd.print(F("8/10"));
  
  float minVal = xmin;
  float maxVal = xmax;
  float range = max(1.0f, maxVal - minVal);
  
  uint8_t medPos = (uint8_t)roundf(mapFloat(median, minVal, maxVal, 0, 15));
  
  for (uint8_t c = 0; c < 16; c++) {
    setCursorTracked(c, 0);
    if (c == medPos) {
      lcd.write(byte(7));
    } else {
      lcd.print((c % 4 == 0) ? '.' : ' ');
    }
  }
  
  setCursorTracked(0, 1);
  lcd.print(F("Median: "));
  printSmallFloat(median, 8);
  
  updateDataLEDs();
}

void showModePage() {
  float xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal;
  uint16_t modeCount;
  computeStats(xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal, modeCount);
  
  lcd.clear();
  setCursorTracked(0, 0);
  lcd.print(F("+ Mode"));
  setCursorTracked(11, 0);
  lcd.print(F("9/10"));
  
  // Create a sorted copy of the samples
  float* sorted = new float[n];
  if (sorted == NULL) {
    Serial.println(F("Memory allocation failed for sorting!"));
    setCursorTracked(0, 1);
    lcd.print(F("Memory Error!"));
    return;
  }
  
  for (uint16_t i = 0; i < n; i++) {
    sorted[i] = samples[i];
  }
  
  // Sort the array
  for (uint16_t i = 1; i < n; i++) {
    float key = sorted[i];
    int16_t j = i - 1;
    while (j >= 0 && sorted[j] > key) {
      sorted[j + 1] = sorted[j];
      j--;
    }
    sorted[j + 1] = key;
  }
  
  // Count distinct values and their frequencies
  uint16_t distinctCount = 0;
  for (uint16_t i = 0; i < n; i++) {
    if (i == 0 || fabs(sorted[i] - sorted[i-1]) > 0.0001) {
      distinctCount++;
    }
  }
  
  // Create frequency array
  struct FreqItem {
    float value;
    uint16_t count;
  };
  
  FreqItem* freqs = new FreqItem[distinctCount];
  if (freqs == NULL) {
    Serial.println(F("Memory allocation failed for frequency array!"));
    delete[] sorted;
    setCursorTracked(0, 1);
    lcd.print(F("Memory Error!"));
    return;
  }
  
  // Fill frequency array
  uint16_t index = 0;
  freqs[0].value = sorted[0];
  freqs[0].count = 0;
  
  // Find the frequency for each distinct value
  for (uint16_t i = 0; i < n; i++) {
    if (fabs(sorted[i] - freqs[index].value) <= 0.0001) {
      freqs[index].count += frequencies[i]; // Use the stored frequency
    } else {
      index++;
      freqs[index].value = sorted[i];
      freqs[index].count = frequencies[i];
    }
  }
  
  // Sort frequency array by count (descending)
  for (uint16_t i = 0; i < distinctCount; i++) {
    for (uint16_t j = i + 1; j < distinctCount; j++) {
      if (freqs[j].count > freqs[i].count) {
        FreqItem temp = freqs[i];
        freqs[i] = freqs[j];
        freqs[j] = temp;
      }
    }
  }
  
  // Display mode and frequencies
  setCursorTracked(0, 1);
  
  if (distinctCount == 1) {
    // Only one distinct value
    lcd.print(F("All: "));
    printSmallFloat(freqs[0].value, 6);
    lcd.print("(");
    lcd.print(freqs[0].count);
    lcd.print(")");
  } else {
    // Show top 2 frequencies
    lcd.print("1:");
    printSmallFloat(freqs[0].value, 3);
    lcd.print("(");
    lcd.print(freqs[0].count);
    lcd.print(") ");
    
    if (distinctCount > 1) {
      lcd.print("2:");
      printSmallFloat(freqs[1].value, 11);
      lcd.print("(");
      lcd.print(freqs[1].count);
      lcd.print(")");
    }
  }
  
  // Clean up
  delete[] sorted;
  delete[] freqs;
  
  updateDataLEDs();
}

void showHistogramPage() {
  isHistogramPage = true;
  
  float xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal;
  uint16_t modeCount;
  computeStats(xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal, modeCount);
  
  lcd.clear();
  setCursorTracked(0, 0);
  lcd.print(F("+ Histogram"));
  setCursorTracked(11, 0);
  lcd.print(F("10/10"));
  
  defineHistogramChars();
  
  // Calculate total number of data points (considering frequencies)
  uint32_t totalPoints = 0;
  for (uint16_t i = 0; i < n; i++) {
    totalPoints += frequencies[i];
  }
  
  if (totalPoints <= 5) {
    // Special handling for small datasets
    float* sortedSamples = new float[n];
    if (sortedSamples == NULL) {
      Serial.println(F("Memory allocation failed for sorting!"));
      setCursorTracked(0, 1);
      lcd.print(F("Memory Error!"));
      return;
    }
    
    for (uint8_t i = 0; i < n; i++) {
      sortedSamples[i] = samples[i];
    }
    
    for (uint8_t i = 1; i < n; i++) {
      float key = sortedSamples[i];
      int16_t j = i - 1;
      while (j >= 0 && sortedSamples[j] > key) {
        sortedSamples[j + 1] = sortedSamples[j];
        j--;
      }
      sortedSamples[j + 1] = key;
    }
    
    // For small datasets, show each value as a separate bar
    uint8_t spacing = (16 - n) / (n + 1);
    
    for (uint8_t i = 0; i < n; i++) {
      uint8_t barHeight;
      if (fabs(xmax - xmin) < 0.001) {
        // All values are the same, show full height bars
        barHeight = 8;
      } else {
        // Map value to bar height (1-8)
        barHeight = (uint8_t)mapFloat(sortedSamples[i], xmin, xmax, 1, 8);
        barHeight = constrain(barHeight, 1, 8);
      }
      
      uint8_t col = spacing + i * (spacing + 1);
      
      // Draw the bar
      setCursorTracked(col, 1);
      lcd.write(byte(barHeight - 1));
    }
    
    delete[] sortedSamples;
  } else {
    // For larger datasets, use histogram bins
    uint16_t bins[16] = {0};
    float binWidth;
    
    if (fabs(xmax - xmin) < 0.001) {
      // All values are the same, distribute evenly across bins
      binWidth = 1.0;
      xmax = xmin + 16.0;
    } else {
      binWidth = (xmax - xmin) / 16.0f;
    }
    
    // Count values in each bin (considering frequencies)
    for (uint16_t i = 0; i < n; i++) {
      uint8_t binIndex = min(15, (uint8_t)((samples[i] - xmin) / binWidth));
      bins[binIndex] += frequencies[i];
    }
    
    // Find the maximum count for scaling
    uint16_t maxCount = 0;
    for (uint8_t i = 0; i < 16; i++) {
      if (bins[i] > maxCount) {
        maxCount = bins[i];
      }
    }
    
    if (maxCount == 0) {
      setCursorTracked(0, 1);
      lcd.print(F("No data to display"));
      return;
    }
    
    // Draw the histogram
    for (uint8_t i = 0; i < 16; i++) {
      uint8_t barHeight;
      if (maxCount > 0) {
        float ratio = (float)bins[i] / maxCount;
        barHeight = (uint8_t)round(ratio * 7.0f) + 1;
      } else {
        barHeight = 1;
      }
      barHeight = constrain(barHeight, 1, 8);
      
      setCursorTracked(i, 1);
      lcd.write(byte(barHeight - 1));
    }
  }
  
  updateDataLEDs();
}

void showInfoPage() {
  lcd.clear();
  setCursorTracked(0, 0);
  lcd.print(F("+ Info"));
  
  // Display subpage number as 1/4 to 4/4
  setCursorTracked(12, 0);
  lcd.print(infoSubPage + 1);
  lcd.print(F("/4"));
  
  defineInfoChars();
  
  lcd.setCursor(0, 1);
  lcd.write(byte(infoSubPage));
  lcd.print(F(": "));
  
  infoScrollPos = 0;
  updateInfoPageScroll();
  
  updateDataLEDs();
}

void updateInfoPageScroll() {
  if (currentState != DISPLAYING_STATS || page != 10) return;
  
  // Get text from PROGMEM
  const char* textPtr = (const char*)pgm_read_word(&infoTexts[infoSubPage]);
  
  // Calculate text length
  uint8_t textLen = strlen_P(textPtr);
  
  if (textLen <= 13) {
    lcd.setCursor(3, 1);
    for (uint8_t i = 0; i < textLen; i++) {
      lcd.print((char)pgm_read_byte(textPtr + i));
    }
    for (uint8_t i = textLen; i < 13; i++) {
      lcd.print(" ");
    }
    return;
  }
  
  lcd.setCursor(3, 1);
  
  for (uint8_t i = 0; i < 13; i++) {
    uint8_t pos = (infoScrollPos + i) % textLen;
    lcd.print((char)pgm_read_byte(textPtr + pos));
  }
  
  infoScrollPos = (infoScrollPos + 1) % textLen;
}

void defineHistogramChars() {
  createCharFromProgmem(0, ch_bar_1);
  createCharFromProgmem(1, ch_bar_2);
  createCharFromProgmem(2, ch_bar_3);
  createCharFromProgmem(3, ch_bar_4);
  createCharFromProgmem(4, ch_bar_5);
  createCharFromProgmem(5, ch_bar_6);
  createCharFromProgmem(6, ch_bar_7);
  createCharFromProgmem(7, ch_bar_8);
}

void defineInfoChars() {
  createCharFromProgmem(0, ch_servo);
  createCharFromProgmem(1, ch_led);
  createCharFromProgmem(2, ch_lcd);
  createCharFromProgmem(3, ch_button);
  createCharFromProgmem(4, ch_info);
}

void defineDinoChars() {
  createCharFromProgmem(0, ch_dino);
  createCharFromProgmem(1, ch_cactus);
}

void restoreOriginalChars() {
  createCharFromProgmem(0, ch_small_block);
  createCharFromProgmem(1, ch_full_block);
  createCharFromProgmem(2, ch_pointer);
  createCharFromProgmem(3, ch_arrow_up);
  createCharFromProgmem(4, ch_arrow_down);
  createCharFromProgmem(5, ch_check);
  createCharFromProgmem(6, ch_progress);
  createCharFromProgmem(7, ch_filled_circle);
}

void printSmallFloat(float v, uint8_t colStart, uint8_t decimals) {
  char buf[8];
  dtostrf(v, 0, decimals, buf);
  uint8_t c = min(colStart, 12);
  lcd.setCursor(c, trackedRow);
  lcd.print(buf);
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void moveServoSmooth(Servo &sv, uint8_t target) {
  target = constrain(target, 0, 180);
  sv.write(target);
}

void drawProgressBar(uint8_t col, uint8_t row, float value, float minVal, float maxVal, uint8_t width) {
  uint8_t fullBlocks = map(value, minVal, maxVal, 0, width);
  for (uint8_t i = 0; i < width; i++) {
    setCursorTracked(col + i, row);
    if (i < fullBlocks) {
      lcd.write(byte(1));
    } else {
      lcd.print(" ");
    }
  }
}

void drawArrow(uint8_t col, uint8_t row, bool up) {
  setCursorTracked(col, row);
  lcd.write(byte(up ? 3 : 4));
}

void animateTransition() {
  animating = true;
  animationStart = millis();
  
  lcd.clear();
  setCursorTracked(7, 0);
  lcd.write(byte(6));
  setCursorTracked(8, 0);
  lcd.write(byte(6));
}

void drawMeter(uint8_t col, uint8_t row, float value, float minVal, float maxVal, uint8_t width) {
  uint8_t fullBlocks = map(value, minVal, maxVal, 0, width);
  for (uint8_t i = 0; i < width; i++) {
    setCursorTracked(col + i, row);
    if (i < fullBlocks) {
      lcd.write(byte(1));
    } else {
      lcd.print(" ");
    }
  }
}

void printCentered(const __FlashStringHelper* text, uint8_t row) {
  uint8_t len = strlen_P((const char*)text);
  uint8_t col = (16 - len) / 2;
  setCursorTracked(col, row);
  lcd.print(text);
}

void drawBarGraph(uint8_t col, uint8_t row, float value, float minVal, float maxVal, uint8_t width) {
  uint8_t fullBlocks = map(value, minVal, maxVal, 0, width);
  for (uint8_t i = 0; i < width; i++) {
    setCursorTracked(col + i, row);
    if (i < fullBlocks) {
      lcd.write(byte(1));
    } else {
      lcd.print(" ");
    }
  }
}

void setRGBLED(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(LED_RED, r);
  analogWrite(LED_GREEN, g);
  analogWrite(LED_BLUE, b);
}

void pulseLED(uint8_t pin, uint16_t duration, uint8_t count) {
  for (uint8_t i = 0; i < count; i++) {
    analogWrite(pin, 255);
    delay(duration);
    analogWrite(pin, 0);
    delay(duration);
  }
}

void updateDataLEDs() {
  if (n == 0) {
    setRGBLED(0, 0, 0);
    return;
  }

  float xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal;
  uint16_t modeCount;
  computeStats(xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal, modeCount);
  
  float range = xmax - xmin;
  if (range < 0.001) range = 1.0;

  float center = (xmin + xmax) / 2.0;
  float distanceFromCenter = fabs(mean - center);
  float maxDistance = range / 2.0;
  float normalizedDistance = distanceFromCenter / maxDistance;
  
  uint8_t greenIntensity;
  if (normalizedDistance < 0.1) {
    greenIntensity = 255;
  } else if (normalizedDistance < 0.3) {
    greenIntensity = 180;
  } else if (normalizedDistance < 0.5) {
    greenIntensity = 100;
  } else if (normalizedDistance < 0.7) {
    greenIntensity = 50;
  } else {
    greenIntensity = 20;
  }

  float normalizedSD = sampSD / range;
  normalizedSD = constrain(normalizedSD, 0, 0.5);
  
  uint8_t blueIntensity;
  if (normalizedSD < 0.05) {
    blueIntensity = 20;
  } else if (normalizedSD < 0.15) {
    blueIntensity = 80;
  } else if (normalizedSD < 0.25) {
    blueIntensity = 150;
  } else if (normalizedSD < 0.35) {
    blueIntensity = 200;
  } else {
    blueIntensity = 255;
  }

  float skewness = fabs(mean - median) / range;
  skewness = constrain(skewness, 0, 0.3);
  
  uint8_t redIntensity;
  if (skewness < 0.02) {
    redIntensity = 20;
  } else if (skewness < 0.08) {
    redIntensity = 60;
  } else if (skewness < 0.15) {
    redIntensity = 120;
  } else if (skewness < 0.22) {
    redIntensity = 180;
  } else {
    redIntensity = 255;
  }

  setRGBLED(redIntensity, greenIntensity, blueIntensity);
}

void pulseDataEntryLED() {
  for (uint8_t i = 0; i < 3; i++) {
    setRGBLED(0, 255, 0);
    delay(100);
    setRGBLED(0, 30, 0);
    delay(100);
  }
}

void showCredits() {
  currentState = CREDITS_SCREEN;
  creditsStartTime = millis();
  creditsPhase1Shown = false;
  
  lcd.clear();
  setCursorTracked(0,0);
  lcd.print(F("* Credits"));
  
  setRGBLED(128, 0, 128);
}

void updateCreditsDisplay() {
  unsigned long elapsed = millis() - creditsStartTime;
  
  if (digitalRead(BTN_MODE) == LOW || digitalRead(BTN_ENTER) == LOW) {
    resetToStartMenu();
    return;
  }
  
  if (elapsed < 4000) {
    if (!creditsPhase1Shown) {
      setCursorTracked(0,1);
      lcd.print(F("Created by:    "));
      creditsPhase1Shown = true;
    }
  } else if (elapsed < 10000) {
    if (creditsPhase1Shown) {
      lcd.clear();
      setCursorTracked(0,0);
      lcd.print(F("* Credits"));
      setCursorTracked(0,1);
      lcd.print(F("Aayushman M.K  "));
      creditsPhase1Shown = false;
    }
  } else {
    resetToStartMenu();
    return;
  }
  
  delay(50);
}

void setServosToStats() {
  if (n == 0) return;

  float xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal;
  uint16_t modeCount;
  computeStats(xmin, xmax, mean, mad, sampVar, sampSD, popVar, popSD, median, modeVal, modeCount);

  float minMap = xmin;
  float maxMap = xmax;
  if (fabs(maxMap - minMap) < 1e-6) {
    minMap = mean - 5;
    maxMap = mean + 5;
  }

  uint8_t meanAng = (uint8_t)constrain(mapFloat(mean, minMap, maxMap, 180, 0), 0, 180);
  moveServoSmooth(sMean, meanAng);

  float range = max(1.0f, xmax - xmin);
  uint8_t spreadAng = (uint8_t)constrain(mapFloat(sampSD, 0, range, 180, 0), 0, 180);
  moveServoSmooth(sSpread, spreadAng);
}

void showDinoGame() {
  lcd.clear();
  
  if (dinoGameOver) {
    setCursorTracked(4, 0);
    lcd.print(F("GAME OVER"));
    setCursorTracked(2, 1);
    lcd.print(F("Score: "));
    lcd.print(dinoScore);
  } else {
    // Draw ground
    for (uint8_t i = 0; i < 16; i++) {
      setCursorTracked(i, 1);
      lcd.print("_");
    }
    
    // Draw dino
    setCursorTracked(2, dinoY);
    lcd.write(byte(0));
    
    // Draw cactus
    setCursorTracked(cactusX, 1);
    lcd.write(byte(1));
    
    // Draw score
    setCursorTracked(11, 0);
    lcd.print(F("Score:"));
    lcd.print(dinoScore);
  }
}

void updateDinoGame() {
  if (dinoGameOver) {
    return;
  }
  
  // Update dino position
  if (dinoY == 0) {
    // Dino is jumping
    dinoVelocity--;
    if (dinoVelocity <= -2) {
      dinoY = 1;
      dinoVelocity = 0;
    }
  }
  
  // Update cactus position
  cactusX--;
  if (cactusX == 255) { // Wrapped around
    cactusX = 15;
    dinoScore++;
    
    // Increase difficulty every 5 points
    if (dinoScore % 5 == 0) {
      // Make the game faster by reducing update interval
      // (We'll handle this by changing the update interval in the main loop)
    }
  }
  
  // Check collision
  if (cactusX == 2 && dinoY == 1) {
    dinoGameOver = true;
  }
  
  showDinoGame();
}

void resetDinoGame() {
  dinoY = 1;
  dinoVelocity = 0;
  cactusX = 15;
  dinoScore = 0;
  dinoGameOver = false;
  lastDinoUpdate = 0;
}
