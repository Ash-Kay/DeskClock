#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include "Module.h"
#include <vector>

class ModuleManager {
private:
    std::vector<Module*> modules;
    Module* activeModule = nullptr;
    Module* defaultModule = nullptr;
    MD_Parola* display = nullptr;
    
    unsigned long lastAutoSwitch = 0;
    const unsigned long AUTO_SWITCH_DELAY = 3000; // 3 seconds after module finishes

public:
    ModuleManager() {}
    
    ~ModuleManager() {
        for (Module* module : modules) {
            delete module;
        }
    }
    
    void setDisplay(MD_Parola* P) {
        display = P;
        for (Module* module : modules) {
            module->setDisplay(P);
        }
    }
    
    void addModule(Module* module) {
        if (display) {
            module->setDisplay(display);
        }
        module->init();
        modules.push_back(module);
        
        // First module with shouldStayActive becomes default
        if (defaultModule == nullptr && module->shouldStayActive()) {
            defaultModule = module;
            activateModule(module);
        }
    }
    
    void update() {
        // Update active module
        if (activeModule) {
            activeModule->update();
            
            // Check if module finished and should auto-switch
            if (!activeModule->isActive() && !activeModule->shouldStayActive()) {
                if (millis() - lastAutoSwitch > AUTO_SWITCH_DELAY) {
                    switchToDefault();
                }
            }
        }
        
        // Update all modules for background tasks
        for (Module* module : modules) {
            if (module != activeModule) {
                module->update();
            }
        }
    }
    
    bool handleWebRequest(String request) {
        // Try each module to handle the request
        for (Module* module : modules) {
            if (module->handleWebRequest(request)) {
                if (module->isActive()) {
                    activateModule(module);
                }
                return true;
            }
        }
        
        // Handle module switching
        if (request.indexOf("/module/") >= 0) {
            for (Module* module : modules) {
                String modulePath = "/module/" + String(module->getName());
                if (request.indexOf(modulePath) >= 0) {
                    activateModule(module);
                    return true;
                }
            }
        }
        
        return false;
    }
    
    String generateWebControls() {
        String controls = "";
        
        // Add module selector
        controls += "<div style=\"border-left: 5px solid #4CAF50;\">";
        controls += "<h3>📱 Modules</h3>";
        
        for (Module* module : modules) {
            String moduleName = module->getName();
            controls += "<button onclick=\"sendRequest('/module/" + moduleName + "')\">";
            if (module == activeModule) {
                controls += "✓ " + moduleName;
            } else {
                controls += moduleName;
            }
            controls += "</button>";
        }
        controls += "</div>";
        
        // Add each module's controls
        for (Module* module : modules) {
            controls += module->getWebControls();
        }
        
        return controls;
    }
    
    Module* getActiveModule() {
        return activeModule;
    }
    
    const std::vector<Module*>& getModules() const {
        return modules;
    }

private:
    void activateModule(Module* module) {
        if (activeModule && activeModule != module) {
            activeModule->deactivate();
        }
        
        activeModule = module;
        if (activeModule) {
            activeModule->activate();
        }
        
        lastAutoSwitch = millis();
    }
    
    void switchToDefault() {
        if (defaultModule && defaultModule != activeModule) {
            activateModule(defaultModule);
        }
    }
};

#endif // MODULE_MANAGER_H