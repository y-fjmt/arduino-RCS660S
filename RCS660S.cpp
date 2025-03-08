/*
 * RC-S660/S sample library for Arduino
 *
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "Arduino.h"
#include "Print.h"
#include "HardwareSerial.h"

#include "RCS660S.h"

/* --------------------------------
 * Constant
 * -------------------------------- */

#define RCS660S_DEFAULT_TIMEOUT  1000

/* --------------------------------
 * Variable
 * -------------------------------- */

/* --------------------------------
 * Prototype Declaration
 * -------------------------------- */

/* --------------------------------
 * Macro
 * -------------------------------- */

/* --------------------------------
 * Function
 * -------------------------------- */

void dbg_print_array(uint8_t* response, uint16_t* responseLen){
    char p_buf[32];
    for (uint8_t i = 0; i < *responseLen; i++){
        sprintf(p_buf, "%02d ",i);
        Serial.print(p_buf);
    }
    Serial.print("\n");
    for (uint8_t i = 0; i < *responseLen; i++){
        sprintf(p_buf, "%02x ",response[i]);
        Serial.print(p_buf);
    }
    Serial.print("\n");
}

/* ------------------------
 * public
 * ------------------------ */

RCS660S::RCS660S()
{
    this->timeout = RCS660S_DEFAULT_TIMEOUT;
}


int RCS660S::initDevice(void)
{
    uint8_t response[RCS660S_MAX_APDU_COMMAND_LEN];
    uint16_t responseLen;

    /* reset device */ 
    if(!apdu((uint8_t*)"\xff\x55\x00\x00", 4, response, &responseLen)) return 0;
    
    return 1;
}


int RCS660S::polling(uint16_t systemCode)
{
    uint8_t response[RCS660S_MAX_CARD_COMMAND_LEN];
    uint8_t responseLen;
    uint16_t resLen;

    /* Manage Session (escape from transparent mode) */
    if(!apdu((uint8_t*)"\xff\xc2\x00\x00\x02\x82\x00\x00", (uint16_t)8, response, &resLen)) return 0;

    /* Manage Session (into transparent mode) */
    if(!apdu((uint8_t*)"\xff\xc2\x00\x00\x02\x81\x00\x00", (uint16_t)8, response, &resLen)) return 0;

    /* switch protocol (set technorogy as Felica) */
    if(!apdu((uint8_t*)"\xff\xc2\x00\x02\x04\x8f\x02\x03\x00\x00", 10, response, &resLen)) return 0;

    /* polling */
    uint8_t buf[5];
    buf[0] = 0x00;
    buf[1] = (uint8_t)((systemCode >> 8) & 0xff);;
    buf[2] = (uint8_t)((systemCode >> 0) & 0xff);
    buf[3] = 0x00;
    buf[4] = 0x00;
    if(!cardCommand(buf, 5, response, &responseLen)){
        return 0;
    }else{
        if(response[0] == 0x01 && responseLen == 17){
            memcpy(this->idm, response + 1, 8);
            memcpy(this->pmm, response + 9, 8);
        }
    }

    return 1;
}

int RCS660S::cardCommand(
    uint8_t* command,
    uint8_t commandLen,
    uint8_t* response,
    uint8_t* responseLen,
    uint32_t timeout /* micro sec */)
{
    uint8_t buf[RCS660S_MAX_CARD_COMMAND_LEN];
    uint8_t res[RCS660S_MAX_APDU_COMMAND_LEN];
    uint16_t resLen;

    uint8_t pos = 0;

    /* set CLA, INS, P1, P2, Le */
    buf[pos++] = 0xff;
    buf[pos++] = 0xc2;
    buf[pos++] = 0x00;
    buf[pos++] = 0x01;
    buf[pos++] = commandLen + 10;

    /* set Timer Data Object */
    buf[pos++] = 0x5f;
    buf[pos++] = 0x46;
    buf[pos++] = 0x04;
    buf[pos++] = (uint8_t)((timeout >>  0) & 0xff);
    buf[pos++] = (uint8_t)((timeout >>  8) & 0xff);
    buf[pos++] = (uint8_t)((timeout >> 16) & 0xff);
    buf[pos++] = (uint8_t)((timeout >> 24) & 0xff);

    /* set RF Command Data Object */
    buf[pos++] = 0x95;
    buf[pos++] = commandLen + 1;
    buf[pos++] = commandLen + 1;
    memcpy(&buf[pos], command, commandLen);
    pos += commandLen;

    /* set Le */
    buf[pos++] = 0x00;

    // execute card command
    if(!apdu(buf, pos, res, &resLen)) return 0;

    if(!memcmp(res, (uint8_t*)"\xc0\x03\x00\x90\x00", 5)){
        *responseLen = res[14]-1;
        memcpy(response, res+15, *responseLen);
    }else{
        return 0;
    }

    return 1;
}

int RCS660S::rfOff(void)
{
    uint8_t response[RCS660S_MAX_APDU_COMMAND_LEN];
    uint16_t responseLen;
    if(!apdu((uint8_t*)"\xff\xc2\x00\x00\x02\x83\x00\x00", 8, response, &responseLen)) return 0;
    return !memcmp(response, (uint8_t*)"\xc0\x03\x00\x90\x00", 5);
}


/* ------------------------
 * private
 * ------------------------ */

int RCS660S::apdu(
        uint8_t* command,
        uint16_t commandLen,
        uint8_t* response,
        uint16_t* responseLen
){
    uint8_t apduRes[RCS660S_MAX_APDU_COMMAND_LEN];
    uint16_t apduResLen;
    uint8_t sw1 = 0xff;
    uint8_t sw2 = 0xff;

    // execute apdu command
    int ret = this->PC_to_RDR_Escape(commandLen, command, apduRes, &apduResLen);

    if(!ret) return 0;

    if (apduResLen > RCS660S_MAX_APDU_COMMAND_LEN) {
        return 0;
    }
    
    sw1 = apduRes[apduResLen - 2];
    sw2 = apduRes[apduResLen - 1];

    if(sw1 == 0xff && sw2 == 0xff)  {
        // Response has not returned.
        return 0;
    }
    else if(sw1 == 0x90 && sw2 == 0x00){
        *responseLen = apduResLen-2;
        memcpy(response, apduRes, *responseLen);
        return 1;
    }  
    else{
        // Response is returned but error See 4.3 ADPU Error Status for details.
        return 0;
    }
}

int RCS660S::PC_to_RDR_Escape(
        uint32_t dwLength,
        const uint8_t* abData,
        uint8_t* response,
        uint16_t* responseLen)
        /* 下位層のrwCommandのResponseLenが2byteなのでuint16_t */
{
    uint8_t buf[RCS660S_MAX_CCID_COMMAND_LEN];
    uint8_t ccidRes[RCS660S_MAX_CCID_COMMAND_LEN];
    uint16_t ccidResLen;
    uint8_t bSeq = (uint8_t)(random(0, 255) & 0xff);
    
    /* make CCID message */
    buf[0] = 0x6b;
    buf[1] = (uint8_t)((dwLength >>  0) & 0xff);
    buf[2] = (uint8_t)((dwLength >>  8) & 0xff);
    buf[3] = (uint8_t)((dwLength >> 16) & 0xff);
    buf[4] = (uint8_t)((dwLength >> 24) & 0xff);
    buf[5] = 0x00;
    buf[6] = bSeq;
    buf[7] = 0x00; 
    buf[8] = 0x00; 
    buf[9] = 0x00;
    memcpy(&buf[10], abData, dwLength);

    // execute CCID command
    int ret = this->rwCommand(buf, dwLength + 10, ccidRes, &ccidResLen);
    if(!ret) return 0;

    if (ccidResLen > RCS660S_MAX_CCID_COMMAND_LEN) {
        this->PC_to_RDR_Abord();
        return 0;
    }

    if(ccidRes[0] != 0x83 || ccidRes[5] != 0x00 || ccidRes[6] != bSeq){
        return 0;
    }else{
        uint8_t hasError = ccidRes[5] & 0b00000011;
        if(hasError){
            // ccidRes[8] <- error code
            return 0;
        }
    }

    // copy data   
    *responseLen = ((uint16_t)(
        (uint32_t)ccidRes[1] <<  0 |
        (uint32_t)ccidRes[2] <<  8 |
        (uint32_t)ccidRes[3] << 16 |
        (uint32_t)ccidRes[4] << 24));

    memcpy(response, &ccidRes[10], *responseLen);

    return 1;
}

void RCS660S::PC_to_RDR_Abord(){
    uint8_t buf[RCS660S_MAX_CCID_COMMAND_LEN];
    uint8_t ccidRes[RCS660S_MAX_CCID_COMMAND_LEN];
    uint16_t* ccidResLen;
    uint8_t bSeq = (uint8_t)(random(0, 255) & 0xff);
    
    /* make CCID message */
    buf[0] = 0x72;
    buf[1] = 0x00;
    buf[2] = 0x00;
    buf[3] = 0x00;
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = bSeq;
    buf[7] = 0x00;
    buf[8] = 0x00;
    buf[9] = 0x00;

    // execute ccid command
    this->rwCommand(buf, 10, ccidRes, ccidResLen);
}

int RCS660S::rwCommand(
    const uint8_t* command,
    uint16_t commandLen,
    uint8_t* response,
    uint16_t* responseLen)
{
    int ret;
    uint8_t buf[9];

    flushSerial();

    uint8_t dcs = calcDCS(command, commandLen);

    /* transmit the command */
    buf[0] = 0x00;
    buf[1] = 0x00;
    buf[2] = 0xff;

    /* LEN */
    buf[3] = (uint8_t)((commandLen >> 8) & 0xff);
    buf[4] = (uint8_t)((commandLen >> 0) & 0xff);

    /* LCS */
    buf[5] = (uint8_t)-(buf[3] + buf[4]);
    writeSerial(buf, 6);

    writeSerial(command, commandLen);

    buf[0] = dcs;
    buf[1] = 0x00;
    writeSerial(buf, 2);

    /* receive an ACK */
    ret = readSerial(buf, 7);
    if (!ret || (memcmp(buf, "\x00\x00\xff\x00\x00\xff\x00", 7) != 0)) {
        cancel();
        return 0;
    }

    /* receive a response */
    ret = readSerial(buf, 6);
    if (!ret) {
        cancel();
        return 0;
    } else if  (memcmp(buf, "\x00\x00\xff", 3) != 0) {
        // if not suitable for command frame format
        cancel();
        return 0;
    }

    /* sum check */
    if (((buf[3] + buf[4] + buf[5]) & 0xff) != 0) {
        cancel();
        return 0;
    }

    *responseLen = ((uint16_t)buf[3] << 8) |
                    ((uint16_t)buf[4] << 0);
    
    if (*responseLen > RCS660S_MAX_CCID_COMMAND_LEN) {
        cancel();
        return 0;
    }

    ret = readSerial(response, *responseLen);
    if (!ret) {
        cancel();
        return 0;
    }

    dcs = calcDCS(response, *responseLen);

    ret = readSerial(buf, 2);
    if (!ret || (buf[0] != dcs) || (buf[1] != 0x00)) {
        cancel();
        return 0;
    }

    return 1;
}

void RCS660S::cancel(void)
{
    /* transmit an ACK */
    writeSerial((const uint8_t*)"\x00\x00\xff\x00\x00\xff\x00", 7);
    delay(1);
    flushSerial();
}

uint8_t RCS660S::calcDCS(
    const uint8_t* data,
    uint16_t len)
{
    uint8_t sum = 0;

    for (uint16_t i = 0; i < len; i++) {
        sum += data[i];
    }

    return (uint8_t)-(sum & 0xff);
}

void RCS660S::writeSerial(
    const uint8_t* data,
    uint16_t len)
{
    Serial.write(data, len);
}

int RCS660S::readSerial(
    uint8_t* data,
    uint16_t len)
{
    uint16_t nread = 0;
    unsigned long t0 = millis();

    while (nread < len) {
        if (checkTimeout(t0)) {
            return 0;
        }

        if (Serial.available() > 0) {
            data[nread] = Serial.read();
            nread++;
        }
    }

    return 1;
}

void RCS660S::flushSerial(void)
{
    Serial.flush();
}

int RCS660S::checkTimeout(unsigned long t0)
{
    unsigned long t = millis();

    if ((t - t0) >= this->timeout) {
        return 1;
    }

    return 0;
}
