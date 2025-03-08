/*
 * RC-S660/S sample library for Arduino
 *
 */

#include <inttypes.h>

#ifndef RCS660S_H_
#define RCS660S_H_

/* --------------------------------
 * Constant
 * -------------------------------- */

#define RCS660S_MAX_CARD_COMMAND_LEN    254
#define RCS660S_MAX_APDU_COMMAND_LEN    267
#define RCS660S_MAX_CCID_COMMAND_LEN    277

/* --------------------------------
 * Class Declaration
 * -------------------------------- */

class RCS660S
{
public:
    RCS660S();

    int initDevice(void);
    int polling(uint16_t systemCode = 0xffff);
    int cardCommand(
        uint8_t* command,
        uint8_t commandLen,
        uint8_t* response,
        uint8_t* responseLen,
        uint32_t timeout=89000);
    int rfOff(void);
    int push(
        const uint8_t* data,
        uint8_t dataLen);

private:
    int apdu(
        uint8_t* command,
        uint16_t commandLen,
        uint8_t* response,
        uint16_t* responseLen
    );
    int PC_to_RDR_Escape(
        uint32_t dwLength,
        const uint8_t* abData,
        uint8_t* response,
        uint16_t* responseLen);
    void PC_to_RDR_Abord(void);
    int rwCommand(
        const uint8_t* command,
        uint16_t commandLen,
        uint8_t* response,
        uint16_t* responseLen);
    void cancel(void);
    uint8_t calcDCS(
        const uint8_t* data,
        uint16_t len);

    void writeSerial(
        const uint8_t* data,
        uint16_t len);
    int readSerial(
        uint8_t* data,
        uint16_t len);
    void flushSerial(void);

    int checkTimeout(unsigned long t0);

public:
    unsigned long timeout;
    uint8_t idm[8];
    uint8_t pmm[8];
};

#endif /* !RCS660S_H_ */
