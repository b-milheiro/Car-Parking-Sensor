# Car Parking Ultrasonic Sensor
<img src="images\CarParkingUltrasonicSensor.png" alt="Ultrasonic Sensor" width="600" height="300">
<div class="image-container">
  <img alt="Static Badge" src="https://img.shields.io/badge/C-blue">
  <img alt="Static Badge" src="https://img.shields.io/badge/Arduino-%232980b9%20">
  <img alt="Static Badge" src="https://img.shields.io/badge/Microchip-red">
  <img alt="Static Badge" src="https://img.shields.io/badge/Sensors-green">
  <img alt="Static Badge" src="https://img.shields.io/badge/Microprocessors-green">
</div>

> A car parking sensor system using a ultrasonic distance sensor.

### Introduction
Using a microcontroller (Atmega328P), an HC-SR04 sensor, LEDs and a Buzzer, we developed a system capable of informing the user of the proximity of an obstacle, as in a Parking Sensor.

## System architecture

<img src="images\Schematic_CarParkingUltrasonicSensor.png" alt="Schematic" width="600" height="300">

This system is composed by: Atmega328P (Arduino UNO); Ultrasonic Sensor - HC-SR04; LEDs - Red, Green and Yellow; 220 Ohm Resistor; Active Buzzer.
In order for the Car Parking Sensor to be working properly, the LEDs and the Buzzer must signal the proximity to an obstacle. 
The LEDs must alternate between the different colors. The Green LED should light up when the car is considered to be at a safe distance. The Yellow LED should light up when there is some proximity, but still no danger of collision. The Red LED should light up when the vehicle and the obstacle are very close. 
The Buzzer must beep more frequently when the vehicle is approaching the obstacle and stop beeping when it's considered to be at a safe distance. 
