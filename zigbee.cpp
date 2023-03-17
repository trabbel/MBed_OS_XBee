#include "mbed.h"
#include <vector>
#include <algorithm>
#include "zigbee.h"

#define START_DELIMITER 0x7E
#define ESCAPE_CHAR     0x7D

/**
 * Write a xBee API 2 in a buffer
 * 
 * This function assembles a 0x10 transmit request frame and writes it in a given buffer.
 * The frame can be send easily by writing the buffer to a serial port that is connected to 
 * a xBee device. 

 */
int writeFrame(char *frame, int addr16, uint64_t addr64, char *payload, int payloadSize){
    // Define a temporary array of sufficient size for everything that may be escaped
    char tempFrame[(16+payloadSize)];
    // Frame delimiter
    frame[0] = START_DELIMITER;
    // Payload length starting with frame type byte, excluding checksum and neccessary escape characters
    int frame_len = 0x0E + payloadSize;
    tempFrame[0] = (frame_len >> 8) & 0xFF;
    tempFrame[1] = frame_len & 0xFF;
    // API ID (TX request)
    tempFrame[2] = 0x10;
    // Frame ID
    tempFrame[3] = 0x01;
    // 64-bit destination address
    for(int i = 0; i<8; i++){
        tempFrame[4+i] = (addr64 >> (7-i)*8) & 0xFF;
    }
    // 16-bit address
    tempFrame[12] = (addr16 >> 8) & 0xFF;
    tempFrame[13] = (addr16 >> 0) & 0xFF;
    // Broadcast radius
    tempFrame[14] = 0x00;
    // Options
    tempFrame[15] = 0x00;
    // Payload
    memcpy(&tempFrame[16], payload, payloadSize);
    // Checksum is calculated based on the original data (non-escaped payload),
    // starting with the frame type
    char checksum = 0x00;
    for(int i = 2; i< 16 + payloadSize; i++){
        checksum += tempFrame[i];
    }
    tempFrame[16+payloadSize] = 0xFF-checksum;
    // Escape the payload and count how many additional escape characters have to be transitted
    int escapes = escapePayload(tempFrame, frame, payloadSize);
    // Write the checkSum at the end of the frame
    //frame[17+payloadSize + escapes] = 0xFF - checksum;

    for (int i = 0 ; i<18+payloadSize+escapes; i++){
            debug("%02x ", tempFrame[i]);
    }debug("\n");

    // The frame is now written in the buffer and ready to send
    return 18+payloadSize+escapes;
}

int escapePayload(char* payload, char* tx_buf, int payloadSize){
    // Counter for needed escape characters
    int escapes = 0;
    // Where to start writing the payload in the buffer
    int pos = 1;
    // Go through each payload byte
    for (int i = 0; i < 17 + payloadSize; i++){
        // Check if the current byte has to be escaped 
        // 0x7e         | frame delimiter, signifies a new frame
        // 0x7d         | escape character, signifies that the next character is escaped
        // 0x11, 0x13   | software control character
        if (payload[i] == 0x7E || payload[i] == 0x7D || payload[i] == 0x11 || payload[i] == 0x13){
            // If a byte has to be escaped, first write ox7d, followed by the byte XORed with 0x20
            tx_buf[pos++] = 0x7D;
            tx_buf[pos++] = payload[i] ^ 0x20;
            escapes++;
        }else{
            // Otherwise write the byte
            tx_buf[pos++] = payload[i];
        }
    }
    return escapes;
}

parsedFrame readFrame(char *frame, BufferedSerial &serial){
    //char API_ID = 0;
    parsedFrame result = {0x00, 0};
    debug("Frame start\n");

    unsigned char lengthBytes[2];
    
    for (int i = 0; i < 2; i++) {
        if (!serial.readable()>0) {
            debug("Serial not readable with i: %i\n", i);
            return result;
        }
        serial.read(&lengthBytes[i], 1);
        if (lengthBytes[i] == START_DELIMITER) {
            debug("START_DELIMITER, should have been data\n");
            return result;
        }else if(lengthBytes[i] == ESCAPE_CHAR){
            thread_sleep_for(1);
            serial.read(&lengthBytes[i], 1);
            lengthBytes[i] = lengthBytes[i] ^ 0x20;
        }
        thread_sleep_for(1);
    }

    int frameLength = (lengthBytes[0] << 8) + lengthBytes[1];

    //serial.read(&API_ID, 1);
    int payloadSize = frameLength;


    for (int i = 0; i < payloadSize + 1; i++) {
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
    //serial.read(&frame[payloadSize], 1);
    char checksum = 0x00;
    for(int i = 0; i< payloadSize; i++){
        checksum += frame[i];
    }
    if (frame[payloadSize] == 0xFF - checksum){
        debug("Checksum correct!\n");
    }else{
        debug("Checksum wrong! Should have been %02x, was %02x\n", frame[payloadSize], 0xFF -checksum);
        //return result;
    }

    debug("Complete Frame parsed\n");
    result = {frame[0], payloadSize};
    return result;
}