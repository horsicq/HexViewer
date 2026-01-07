#ifndef PLUGINTYPES_H
#define PLUGINTYPES_H

struct PluginInfo {
    char name[128];
    char version[32];
    char author[64];
    char description[256];
    char path[512];
    char language[16];
    bool enabled;
    bool loaded;
    bool canDisassemble; 
    bool canAnalyze;      
    bool canTransform;    
};

#endif
