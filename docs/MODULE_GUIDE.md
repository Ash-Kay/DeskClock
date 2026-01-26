# Module Development Guide

## Creating New Modules

### 1. Module Interface

Every module must inherit from the base `Module` class and implement these methods:

```cpp
class YourModule : public Module {
public:
    void init() override;                    // Initialize module resources
    void activate() override;                // Called when module becomes active
    void deactivate() override;              // Called when switching away
    void update() override;                  // Called every loop iteration
    const char* getName() override;          // Unique module identifier
    const char* getWebControls() override;   // HTML controls for web interface
    bool handleWebRequest(String request) override; // Handle HTTP requests
};
```

### 2. Module Lifecycle

1. **init()** - Called once during setup, initialize any resources
2. **activate()** - Called when module becomes active, setup display
3. **update()** - Called every loop, handle module logic
4. **deactivate()** - Called when switching away, cleanup
5. **handleWebRequest()** - Process incoming web requests

### 3. Display Access

Use `P->` to access the MD_Parola display object:

```cpp
P->displayText("Hello", PA_CENTER, 50, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
P->displayClear();
P->getGraphicObject()->setPoint(x, y, true);
```

### 4. Web Integration

Return HTML controls that will be injected into the web interface:

```cpp
const char* getWebControls() override {
    return R"(
    <div style="border-left: 5px solid #4CAF50;">
        <h3>🔧 Your Module</h3>
        <button onclick="sendRequest('/yourmodule/action')">Action</button>
    </div>
    )";
}
```

Handle requests matching your module's URLs:

```cpp
bool handleWebRequest(String request) override {
    if (request.indexOf("/yourmodule/action") >= 0) {
        // Handle the action
        activate(); // Activate if needed
        return true;
    }
    return false;
}
```

### 5. Module Types

**Persistent Modules** (like Clock):

- Override `shouldStayActive()` to return `true`
- Remain active by default
- Used for core functionality

**Temporary Modules** (like Pomodoro, Text):

- Auto-deactivate when task complete
- System returns to default module
- Used for specific actions

### 6. Example: Simple Counter Module

```cpp
#ifndef COUNTER_MODULE_H
#define COUNTER_MODULE_H

#include "Module.h"

class CounterModule : public Module {
private:
    int counter = 0;
    unsigned long lastUpdate = 0;

public:
    void init() override {
        counter = 0;
    }

    void activate() override {
        active = true;
        P->displayClear();
        updateDisplay();
    }

    void deactivate() override {
        active = false;
    }

    void update() override {
        if (!active) return;

        // Update every second
        if (millis() - lastUpdate > 1000) {
            counter++;
            updateDisplay();
            lastUpdate = millis();
        }
    }

    const char* getName() override {
        return "Counter";
    }

    const char* getWebControls() override {
        return R"(
        <div style="border-left: 5px solid #FF5722;">
            <h3>🔢 Counter</h3>
            <button onclick="sendRequest('/counter/start')">Start Counter</button>
            <button onclick="sendRequest('/counter/reset')">Reset</button>
        </div>
        )";
    }

    bool handleWebRequest(String request) override {
        if (request.indexOf("/counter/start") >= 0) {
            activate();
            return true;
        }

        if (request.indexOf("/counter/reset") >= 0) {
            counter = 0;
            if (active) updateDisplay();
            return true;
        }

        return false;
    }

private:
    void updateDisplay() {
        char buffer[10];
        sprintf(buffer, "%d", counter);
        P->displayText(buffer, PA_CENTER, 0, 0, PA_PRINT, PA_NO_EFFECT);
    }
};

#endif
```

### 7. Integration Steps

1. Create `YourModule.h` in `modules/` directory
2. Add include to `SmartClock.ino`:
   ```cpp
   #include "modules/YourModule.h"
   ```
3. Add to setup():
   ```cpp
   moduleManager.addModule(new YourModule());
   ```

Module automatically appears in web interface and is ready to use!

### 8. Best Practices

- Keep modules focused on single responsibility
- Handle all cleanup in `deactivate()`
- Use meaningful web URLs (`/modulename/action`)
- Provide clear web control labels
- Test both standalone and integration scenarios
- Document any special hardware requirements
