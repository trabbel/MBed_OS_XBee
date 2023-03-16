#include "mbed.h"
#include <vector>
#include <algorithm>
#include "zigbee.h"

#define START_DELIMITER 0x7E
#define ESCAPE_CHAR     0x7D
#define API_ID_INDEX    3

int writeFrame(char *frame, int addr16, uint64_t addr64, char *payload, int payloadSize){
    char tempFrame[(13+payloadSize)*2];
    // Frame delimiter
    frame[0] = START_DELIMITER;
    // Payload length starting with frame type byte, excluding checksum
    //frame[1] = 0x00;
    //frame[2] = 0x0E + payloadSize;
    int frame_len = 0x0E + payloadSize;
    frame[1] = (frame_len >> 8) & 0xFF;
    frame[2] = frame_len & 0xFF;
    // Frame type (TX request)
    frame[3] = 0x10;
    // Frame ID
    tempFrame[0] = 0x00;
    // 64-bit destination address
    for(int i = 0; i<8; i++){
        tempFrame[1+i] = (addr64 >> (7-i)*8) & 0xFF;
    }
    // 16-bit address
    tempFrame[9] = (addr16 >> 8) & 0xFF;
    tempFrame[10] = (addr16 >> 0) & 0xFF;
    // Broadcast radius
    tempFrame[11] = 0x00;
    // Options
    tempFrame[12] = 0x00;
    // Payload
    memcpy(&tempFrame[13], payload, payloadSize);
    // Checksum
    char checksum = frame[3];
    for(int i = 0; i< 13 + payloadSize; i++){
        checksum += tempFrame[i];
    }

    int escapes = escapePayload(tempFrame, frame, payloadSize);

    frame[17+payloadSize + escapes] = 0xFF - checksum;

            for (int i = 0 ; i<18+payloadSize+escapes; i++)
            {
                debug("%02x ", tempFrame[i]);
            }debug("\n");

    return 18+payloadSize+escapes;
}

int escapePayload(char* payload, char* tx_buf, int payloadSize){
    int escapes = 0;
    int pos = 4;
    for (int i = 0; i < 13 + payloadSize; i++){
        if (payload[i] == 0x7E || payload[i] == 0x7D || payload[i] == 0x11 || payload[i] == 0x13){
            tx_buf[pos] = 0x7D;
            pos++;
            tx_buf[pos] = payload[i] ^ 0x20;
            pos++;
            escapes++;
        }else{
            tx_buf[pos] = payload[i];
            pos++;
        }
    }
    return escapes;
}

int readFramez(char *frame, BufferedSerial &serial){
    int i = 0;
    // Wait for start of frame delimiter

    debug("Frame start\n");
    while(serial.readable()>0){
        serial.read(&frame[i], 1);
        i++;
        thread_sleep_for(1);
    }
    return i;
}

int readFrame(char *frame, BufferedSerial &serial){
    //int pos = 0;
    char API_ID = 0;
    debug("Frame start\n");

    unsigned char lengthBytes[2];
    serial.read(lengthBytes, 2);
    int frameLength = (lengthBytes[0] << 8) + lengthBytes[1];

    serial.read(&API_ID, 1);
    int payloadSize = frameLength -1;

    debug("1\n");

    for (int i = 0; i < payloadSize; i++) {
        if (!serial.readable()>0) {
            debug("Serial not readable with i: %i\n", i);
            return -2;
        }
        serial.read(&frame[i], 1);
        if (frame[i] == START_DELIMITER) {
            debug("START_DELIMITER, should have been data\n");
            return -1;
        }else if(frame[i] == ESCAPE_CHAR){
            thread_sleep_for(1);
            serial.read(&frame[i], 1);
            frame[i] = frame[i] ^ 0x20;
        }
        thread_sleep_for(1);
    }
    serial.read(&frame[payloadSize], 1);
    char checksum = API_ID;
    for(int i = 0; i< payloadSize; i++){
        checksum += frame[i];
    }
    if (frame[payloadSize] == 0xFF - checksum){
        debug("Checksum correct!\n");
    }else{
        debug("Checksum wrong! Was %i, should have been %h\n", 0xFF-checksum, frame[payloadSize]);
    }

    debug("Complete Frame parsed\n");
    return payloadSize;
}