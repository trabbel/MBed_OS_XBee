struct parsedFrame{
    char frameID;
    int length;
};

int writeFrame(char *frame, int addr16, uint64_t addr64, char *payload, int payloadSize);
parsedFrame readFrame(char *frame, BufferedSerial &serial);
int escapePayload(char* payload, char* tx_buf, int payloadSize);