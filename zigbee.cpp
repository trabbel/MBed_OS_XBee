#include "mbed.h"
#include <vector>
#include <algorithm>
#include "zigbee.h"

#define START_DELIMITER 0x7E
#define ESCAPE_CHAR     0x7D

int writeFrame(char *frame, int addr16, uint64_t addr64, char *payload, int payloadSize){
    // Define a temporary array of sufficient size for everything that may be escaped
    char tempFrame[(13+payloadSize)];
    // Frame delimiter
    frame[0] = START_DELIMITER;
    // Payload length starting with frame type byte, excluding checksum and neccessary escape characters
    int frame_len = 0x0E + payloadSize;
    frame[1] = (frame_len >> 8) & 0xFF;
    frame[2] = frame_len & 0xFF;
    // Frame type (TX request)
    frame[3] = 0x10;
    // Frame ID
    tempFrame[0] = 0x01;
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
    // Checksum is calculated based on the original data (non-escaped payload),
    // starting with the frame type
    char checksum = frame[3];
    for(int i = 0; i< 13 + payloadSize; i++){
        checksum += tempFrame[i];
    }
    // Escape the payload and count how many additional escape characters have to be transitted
    int escapes = escapePayload(tempFrame, frame, payloadSize);
    // Write the checkSum at the end of the frame
    frame[17+payloadSize + escapes] = 0xFF - checksum;

        for (int i = 0 ; i<18+payloadSize+escapes; i++)
        {
            debug("%02x ", tempFrame[i]);
        }debug("\n");

    //delete[] &tempFrame;
    // The frame is now written in the buffer and ready to send. Return the frame size in bytes
    return 18+payloadSize+escapes;
}

int escapePayload(char* payload, char* tx_buf, int payloadSize){
    // Counter for needed escape characters
    int escapes = 0;
    // Where to start writing the payload in the buffer
    int pos = 4;
    for (int i = 0; i < 13 + payloadSize; i++){
        if (payload[i] == 0x7E || payload[i] == 0x7D || payload[i] == 0x11 || payload[i] == 0x13){
            tx_buf[pos++] = 0x7D;
            tx_buf[pos++] = payload[i] ^ 0x20;
            escapes++;
        }else{
            tx_buf[pos++] = payload[i];
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

parsedFrame readFrame(char *frame, BufferedSerial &serial){
    char API_ID = 0;
    parsedFrame result = {0x00, 0};
    debug("Frame start\n");

    unsigned char lengthBytes[2];
    serial.read(lengthBytes, 2);
    int frameLength = (lengthBytes[0] << 8) + lengthBytes[1];

    serial.read(&API_ID, 1);
    /*if(API_ID != 0x90){
        return -4;
    }*/
    int payloadSize = frameLength -1;


    for (int i = 0; i < payloadSize; i++) {
        if (!serial.readable()>0) {
            debug("Serial not readable with i: %i\n", i);
            return result;
        }
        serial.read(&frame[i], 1);
        if (frame[i] == START_DELIMITER) {
            debug("START_DELIMITER, should have been data\n");
            return result;
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
        debug("Checksum wrong! Should have been %02x\n", frame[payloadSize]);
        return result;
    }

    debug("Complete Frame parsed\n");
    result = {API_ID, payloadSize};
    return result;
}