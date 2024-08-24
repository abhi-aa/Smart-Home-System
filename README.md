# Smart Home Monitoring System

## Overview

This project is a Smart Home Monitoring System designed to automate and optimize various household tasks. It includes three main components:

1. **Automated Blinds**: Controls the opening and closing of blinds based on environmental conditions such as sunlight intensity.
2. **Irrigation System**: Manages the watering of plants based on soil moisture levels and weather conditions.
3. **Temperature/Humidity Monitoring**: Monitors indoor temperature and humidity levels, providing real-time data to ensure optimal living conditions.

## Project Structure

The project is organized into the following folders:

- **Automated Blinds**
  - Contains Arduino code responsible for controlling the blinds.
  - Includes various algorithms and flowcharts to illustrate the logic and operation of the system.

- **Irrigation System**
  - Contains Arduino code for automating the watering process.
  - Includes different algorithms and flowcharts that outline the decision-making process for when and how much to irrigate.

- **Temperature/Humidity Monitoring**
  - Contains Arduino code that reads data from the DHT11 sensor to monitor temperature and humidity.
  - Includes algorithms and flowcharts that detail the data acquisition and processing methods.

## Requirements

- **Hardware**: Arduino boards, sensors (e.g., photoresistor sensors for the blinds, moisture sensors for the irrigation system, DHT11 sensor for temperature/humidity), actuators (e.g., motors for the blinds, pumps for irrigation).
- **Software**: Arduino IDE for coding and uploading the scripts to the microcontrollers.


## How It Works

- **Automated Blinds**: The system uses a light sensor to detect the intensity of sunlight. Based on predefined thresholds, it triggers the motor to open or close the blinds.
- **Irrigation System**: The system monitors soil moisture levels using a sensor. Depending on the moisture level and weather conditions, it activates a pump to water the plants.

## How It Works

- **Automated Blinds**: The system uses a light sensor to detect the intensity of sunlight. Based on predefined thresholds, it triggers the motor to open or close the blinds.
- **Irrigation System**: The system monitors soil moisture levels using a sensor. Depending on the moisture level and weather conditions, it activates a pump to water the plants.
- **Temperature/Humidity Monitoring**: The system uses a DHT11 sensor to measure indoor temperature and humidity. The data is periodically read and displayed, ensuring the environment remains comfortable.

## Flowcharts & Algorithms

Each folder contains detailed flowcharts and algorithms explaining the logic behind the operation of the automated blinds and irrigation system. These visual aids are intended to provide a clear understanding of how each component functions.
