int writeFrame(char *frame, int addr16, uint64_t addr64, char *payload, int payloadSize);
int readFrame(char *frame, BufferedSerial &serial);
int escapePayload(char* payload, char* tx_buf, int payloadSize);
