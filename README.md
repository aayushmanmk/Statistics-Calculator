# ISC-11 Statistics Calculator with RGB LED Data Visualization

[![Arduino](https://img.shields.io/badge/Arduino-Uno-blue.svg)](https://www.arduino.cc/)
[![Status](https://img.shields.io/badge/Status-Complete-brightgreen.svg)](https://github.com)

An advanced Arduino-based statistics calculator that provides comprehensive data analysis with visual feedback through servos, RGB LED, and LCD display.

## 🌟 Features

- 📊 **Comprehensive Statistics**: Calculate count, mean, min, max, standard deviation, variance, MAD, median, and mode
- 📈 **Data Visualization**: Histogram display on LCD with custom characters
- 🎮 **Interactive Interface**: Navigate through menus using UP, DOWN, ENTER, and MODE buttons
- 🌈 **RGB LED Feedback**: Color-coded indicators representing data distribution characteristics
- 📐 **Servo Visualization**: Two servos showing mean position and data spread
- 📋 **Sample Data**: Pre-loaded example datasets including stock prices and height statistics

## 🛠️ Hardware Requirements

- Arduino board (Uno recommended)
- 16x2 I2C LCD Display (address 0x27)
- Two Servo Motors
- RGB LED (common cathode)
- Four Push Buttons
- Breadboard and Jumper Wires

### Pin Connections

Component       | Arduino Pin
----------------|------------
LCD SDA         | A4
LCD SCL         | A5
Servo (Mean)    | D9
Servo (Spread)  | D10
RGB Red         | D6
RGB Green       | D7
RGB Blue        | D8
Button UP       | D2
Button DOWN     | D3
Button ENTER    | D4
Button MODE     | D5


## 💾 Installation

1. Download the latest Arduino IDE
2. Install required libraries:
   - `Wire.h` (built-in)
   - `LiquidCrystal_I2C.h`
   - `Servo.h`
3. Clone this repository or download the ZIP file
4. Open the `Code.ino` file in Arduino IDE
5. Upload the sketch to your Arduino board

## 📖 Usage Guide (Check Manual.html for comprehensive guide)

### Main Menu

- **Enter Data**: Input your own data points with frequencies
- **Examples**: Choose from pre-loaded datasets (AAPL, NVDA, NKE, GOOGL, SONY, TTWO, Height data)
- **Help**: Basic navigation instructions
- **Credits**: Project information
- **Info**: Hardware component details

### Navigation

- **UP/DOWN**: Navigate menus, adjust values
- **ENTER**: Select option, confirm values
- **MODE**: Next page, return to previous menu
- **Long Press MODE**: Return to main menu

### Statistics Pages

The calculator displays 10 different statistical measures:

1. **Count**: Number of data points and total values
2. **Mean**: Average value
3. **Minimum**: Smallest value in dataset
4. **Maximum**: Largest value in dataset
5. **Standard Deviation**: Sample and population SD
6. **Variance**: Sample and population variance
7. **MAD**: Mean Absolute Deviation
8. **Median**: Middle value with visual indicator
9. **Mode**: Most frequent value(s)
10. **Histogram**: Visual distribution of data

### Visual Indicators

- **RGB LED**: Changes color based on data characteristics
  - Green intensity indicates centrality of data
  - Blue intensity represents standard deviation
  - Red intensity shows skewness
- **Servos**: 
  - Mean servo position shows average value
  - Spread servo shows standard deviation

## 📝 Credits

Created by Aayushman M.K
