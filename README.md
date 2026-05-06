![QR](Screenshot%20from%202026-05-06%2021-48-41.png)

_   _  _______    _   _  ____  
 | \ | || ____|\ \/ /| | | |/ ___| 
 |  \| ||  _|   \  / | | | |\___ \                 
 | |\  || |___  /  \ | |_| | ___) |
 |_| \_||_____|/_/\_\ \___/ |____/ 
                                   
  >-- LiDAR Room 3D Mapping --<

System Architecture Overview

This project integrates a web-based client interface, a cloud database, wireless microcontrollers, and custom FPGA hardware to manage a ceiling-mounted LiDAR scanning system. The architecture is divided into the following core components:
Client Side

The client application provides a web-based control interface allowing users to send scanning commands, monitor system status, and interact with the scanner. It communicates asynchronously with the backend to ensure a responsive user experience while the hardware executes complex routines.
Firebase Backend

The system utilizes Firebase to synchronize state across the client interface and the physical hardware. The database is structured into specific branches to maintain organized data flow:

    Commands: Stores incoming user requests (e.g., start scan, stop, reset). The ESPs listen to this branch to trigger hardware events.

    Sensor Data: Holds the live, continuous data stream uploaded by the hardware (distance metrics, positional data).

    System Status: Maintains the current operational state of the microcontrollers, FPGA, and motor endpoints for client-side monitoring.

Microcontrollers (ESP Network)

The system employs two ESP microcontrollers to distribute the network and sensor processing loads efficiently:

    Control Node (ESP 1): Acts as the primary bridge between the Firebase backend and the FPGA. It listens for user inputs, fetches the latest commands, and sends the critical "Start" signal to the Altera FPGA to begin the scanning sequence.

    Sensor Node (ESP 2): Dedicated to high-speed data acquisition. It interfaces directly with the LiDAR sensor to capture distance measurements and reads the MPU6050 accelerometer/gyroscope to track the exact tilt, orientation, and movement of the assembly.

FPGA (Altera) & Hardware Control

An Altera FPGA, programmed in VHDL, acts as the central hardware controller. It manages the strict, microsecond-level timing requirements of the electromechanical components. Upon receiving the start command from ESP 1, the FPGA's internal state machines orchestrate:

    Stepper Motor: Executes precise rotational sequences (e.g., timed 360-degree sweeps) for the scanner base.

    Servo Motor: Drives the vertical tilt mechanism, coordinating with the rotation to capture comprehensive spatial data.

    Buzzer: Provides immediate audible feedback for system readiness, state changes, and alerts.

https://ouix1.github.io/nexus/home

https://ouix1.github.io/nexus/start

