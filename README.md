# Smart Desk Clock

ESP32-based smart desk clock with modular architecture, featuring air quality monitoring, LED matrix display, and web-based controls.

## Features

- 🕰️ **Clock Display** - Time/date with timezone support
- 🍅 **Pomodoro Timer** - Focus sessions with visual progress
- 💬 **Text Scrolling** - Custom messages with speed control
- 🌐 **Web Interface** - Remote control via WiFi
- 🔧 **Modular Design** - Plug-and-play feature modules

## Hardware

- ESP32-WROOM-32 microcontroller
- MAX7219 LED Dot Matrix (4 modules, 32x8 pixels)
- PLANTOWER PMSA003-A air quality sensor (planned)

## Quick Start

1. **Setup Configuration:**

   ```bash
   cp config/config.h.example config/config.h
   # Edit config/config.h with your WiFi credentials
   ```

2. **Upload to ESP32:**
   - Open `SmartClock.ino` in Arduino IDE
   - Select ESP32 board and upload

3. **Access Web Interface:**
   - Connect to your WiFi network
   - Open ESP32's IP address in browser
   - Control modules and settings

## Project Structure

```
SmartClock/
├── SmartClock.ino          # Main program
├── config/
│   ├── config.h            # WiFi credentials (git-ignored)
│   └── config.h.example    # Template
├── modules/
│   ├── Module.h            # Base module interface
│   ├── ModuleManager.h     # Module lifecycle management
│   ├── ClockModule.h       # Time/date display
│   ├── PomodoroModule.h    # Timer functionality
│   └── TextModule.h        # Message scrolling
├── docs/
│   └── .copilot.md         # Detailed documentation
└── README.md               # This file
```

## Adding New Modules

1. Create `YourModule.h` in `modules/` directory:

```cpp
#include "Module.h"

class YourModule : public Module {
public:
    void init() override { /* Initialize */ }
    void activate() override { /* Start module */ }
    void deactivate() override { /* Stop module */ }
    void update() override { /* Update logic */ }
    const char* getName() override { return "YourModule"; }
    const char* getWebControls() override { return "<html>..."; }
    bool handleWebRequest(String request) override { /* Handle web */ }
};
```

2. Add to `SmartClock.ino`:

```cpp
#include "modules/YourModule.h"
// In setup():
moduleManager.addModule(new YourModule());
```

Module automatically appears in web interface!

## Pin Connections

```
MAX7219 LED Matrix:
- CLK (Clock): GPIO 18
- DIN (Data): GPIO 23
- CS (Chip Select): GPIO 5

PMSA003-A (Future):
- TX: GPIO 16
- RX: GPIO 17
- Power: 5V, GND
```

## Web API

- `/clock/config?tz=5.5&fmt=24` - Configure timezone and format
- `/pomodoro/start?min=25` - Start timer
- `/pomodoro/stop` - Stop timer
- `/text/show?msg=Hello&speed=50` - Display message
- `/module/ModuleName` - Switch to module

## Libraries Required

- MD_Parola
- MD_MAX72XX
- WiFi (ESP32)
- Time (ESP32)

## License

MIT License - Feel free to modify and extend!
