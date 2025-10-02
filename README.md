# 📊 ISC-11 Statistics Calculator: Visual Data Analysis with Arduino

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
10. **Histogram**: Visual distribution of data (Note: The histogram is relative in order to show contrast between datapoints)

### Visual Indicators

- **RGB LED**: Changes color based on data characteristics
  - Green intensity indicates centrality of data
  - Blue intensity represents standard deviation
  - Red intensity shows skewness
- **Servos**: 
  - Mean servo position shows average value
  - Spread servo shows standard deviation

### Images Of Model

![2025-10-02-10-18-53-813](https://github.com/user-attachments/assets/4033d9ef-fb8c-452e-a908-f2ae367bb454)
![2025-10-02-10-31-52-090](https://github.com/user-attachments/assets/a120838f-aa80-42cf-b437-77d23610b364)
![2025-10-02-10-32-09-356](https://github.com/user-attachments/assets/4bf41770-6df1-40af-9a31-41dc5e7ea9d4)
![2025-10-02-10-32-19-893](https://github.com/user-attachments/assets/94c213c1-18b5-4959-bd5c-8345e4e7245d)
![2025-10-02-10-32-28-617](https://github.com/user-attachments/assets/f32421fd-98d7-47ae-94a8-18509b9ff4ec)
![2025-10-02-10-32-37-332](https://github.com/user-attachments/assets/71cd7d78-a360-4e7c-9580-ea7820ceaa21)
![2025-10-02-10-32-43-142](https://github.com/user-attachments/assets/61ae2d6f-f6d7-4a5e-bf0e-1a86b1009206)
![2025-10-02-10-32-54-341](https://github.com/user-attachments/assets/c6237ed6-0c3c-4b52-8a32-275387a498be)
![2025-10-02-10-33-00-495](https://github.com/user-attachments/assets/8c3b365b-d8d9-4e8c-b114-5781240c603d)
![2025-10-02-10-33-05-323](https://github.com/user-attachments/assets/ba45834a-0171-4a30-907c-b2be881df495)
![2025-10-02-10-33-12-219](https://github.com/user-attachments/assets/85d2fcfe-9429-445f-b12e-e1fd614f9057)
![2025-10-02-10-41-34-845](https://github.com/user-attachments/assets/9756efb3-457a-4a47-836f-f445e0d5ddd8)



## 📝 Credits

Created by Aayushman M.K

## 🗒️Note

This project was made for an ISC-11 project of mine for school and therefore is called an ISC-11 Statistics calculator.
