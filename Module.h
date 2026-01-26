#ifndef MODULE_H
#define MODULE_H

#include <MD_Parola.h>
#include <WiFiClient.h>

// Base class for all modules
class Module {
public:
    virtual ~Module() {}
    
    // Module lifecycle
    virtual void init() = 0;
    virtual void update() = 0;
    virtual void activate() = 0;
    virtual void deactivate() = 0;
    
    // Module info
    virtual const char* getName() = 0;
    virtual const char* getWebControls() = 0;
    
    // Web request handling
    virtual bool handleWebRequest(String request) { return false; }
    
    // Display access
    void setDisplay(MD_Parola* display) { P = display; }
    
    // Status
    virtual bool isActive() { return active; }
    virtual bool shouldStayActive() { return false; } // Override for persistent modules
    
protected:
    MD_Parola* P = nullptr;
    bool active = false;
};

#endif // MODULE_H