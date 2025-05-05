# Air Quality Monitoring Application

## Description
This application monitors air quality data from monitoring stations, displaying measurements and air quality indices. It uses the GIOS (Polish Environmental Protection Inspectorate) API to fetch real-time data and stores it locally in JSON format.

## Features
- Fetches and displays air quality stations
- Shows sensor data for each station
- Displays measurement history with charts
- Presents air quality indices with color-coded indicators
- Stores data locally in JSON format
- Supports filtering by city and location

## Data Structure
The application works with 4 main data types:
1. `Station` - monitoring station information
2. `Sensor` - measurement sensors at each station
3. `Measurement` - actual measurement values
4. `AirQualityIndex` - calculated air quality indices

## JSON Storage
Data is saved to `air_quality_data.json` in the application data directory with the following structure:
{
"index":{...},
"stacje":[...],
"stanowiska":[...]
}

## Dependencies
- Qt 5.15+ (Core, Network, Charts)
- C++17

## Setup
1. Clone the repository
2. Build with Qt Creator
3. Run the application

## Usage
1. Stations are automatically loaded at startup
2. Click on a station to view its sensors
3. Select a sensor to see measurements
4. Use date filters to view historical data

## File Structure
- `ApiHandler.h/cpp` - handles API communication
- `JsonBaseManager.h/cpp` - manages local JSON storage
- `MainWindow.h/cpp` - main application UI
- Model classes (`Station`, `Sensor`, `Measurement`, `AirQualityIndex`)

## Notes
- The application checks for internet connectivity
- If offline, it uses locally stored data
- Data is automatically saved when fetched from API

