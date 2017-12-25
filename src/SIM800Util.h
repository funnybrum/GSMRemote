void ToggleSIM800PowerState();
bool SendCommandAndVerifyResponse(char* cmd, char* response, unsigned int timeout=1000);
unsigned int SendCommandAndReadResponse(char* cmd, char* resp, unsigned int timeout=1000, bool flush=true);