#include "ds1302.h"

// Public Functions

void DS1302::init(uint8_t p_IO, uint8_t p_CLK, uint8_t p_CE)
{
    //save pins in local struct
    this->ce = p_CE;
    this->clk = p_CLK;
    this->io = p_IO;

    //Clear 'CLOCK HALT' flag
    uint8_t temp = readByte(REG_SECS);
    temp &= 0x7F;
    writeByte(REG_SECS, temp);

    //Clear 'WRITE PROTECT' flag
    writeByte(REG_WP, 0x00);
}

void DS1302::writeByte(uint8_t addr, uint8_t dat)
{
    //IO line setup
    pinMode(this->io, OUTPUT);
    digitalWrite(this->io, LOW);
    delayMicroseconds(1);

    //Start of Data Transfer
    digitalWrite(this->ce, HIGH);
    delayMicroseconds(1);

    uint8_t command_byte = 0b10000000 | (addr & 0x3F)<<1;
    send(command_byte);
    send(dat);

    //end of Data Transfer
    delayMicroseconds(1);
    digitalWrite(this->ce, LOW);
}

uint8_t DS1302::readByte(uint8_t addr)
{
    uint8_t temp;

    //IO line setup
    pinMode(this->io, OUTPUT);
    digitalWrite(this->io, LOW);
    delayMicroseconds(1);

    //Start of Data Transfer
    digitalWrite(this->ce, HIGH);
    delayMicroseconds(1);

    uint8_t command_byte = 0b10000001 | (addr & 0x3F)<<1;
    send(command_byte);

    pinMode(this->io, INPUT);
    delayMicroseconds(1);

    temp = read();

    //end of Data Transfer
    delayMicroseconds(1);
    digitalWrite(this->ce, LOW);

    return temp;
}

void DS1302::enableCharging()
{
    uint8_t addr = REG_CHRG;
    uint8_t dat = 0b10100101; // 1 diode, 2k
    writeByte(addr, dat);
}

void DS1302::setTime(mytimeinfo timestruct)
{
    uint8_t h10, h1, m10, m1, s10, s1;
    uint8_t d1, d10, mo1, mo10, y1, y10;
    h1 = timestruct.hours % 10;
    h10 = (timestruct.hours - h1) / 10;
    m1 = timestruct.minutes % 10;
    m10 = (timestruct.minutes - m1) / 10;
    s1 = timestruct.seconds % 10;
    s10 = (timestruct.seconds - s1) / 10;
    d1 = timestruct.date % 10;
    d10 = (timestruct.date - d1) / 10;
    mo1 = (timestruct.month+1) % 10;
    mo10 = ((timestruct.month+1) - mo1) / 10;
    y1 = timestruct.year % 10;
    y10 = (timestruct.year - y1) / 10;

    uint8_t seconds =  s10 << 4 | s1;
    uint8_t minutes =  m10 << 4 | m1;
    uint8_t hours   =  h10 << 4 | h1;
    uint8_t date    =  d10 << 4 | d1;
    uint8_t month   = mo10 << 4 | mo1;
    uint8_t year    =  y10 << 4 | y1;

    writeByte(REG_SECS, seconds);
    writeByte(REG_MINS, minutes);
    writeByte(REG_HOUR, hours);
    writeByte(REG_DATE, date);
    writeByte(REG_MONT, month);
    writeByte(REG_WDAY, timestruct.wday);
    writeByte(REG_YEAR, year);
}


mytimeinfo DS1302::getTime()
{
    mytimeinfo temp;

    uint8_t seconds = readByte(REG_SECS);
    uint8_t minutes = readByte(REG_MINS);
    uint8_t hours   = readByte(REG_HOUR);
    uint8_t date    = readByte(REG_DATE);
    uint8_t month   = readByte(REG_MONT);
    uint8_t year    = readByte(REG_YEAR);
    temp.wday       = readByte(REG_WDAY);

    temp.seconds = (seconds >> 4) * 10 + (seconds & 0x0F);
    temp.minutes = (minutes >> 4) * 10 + (minutes & 0x0F);
    temp.hours   = (hours >> 4)   * 10 + (hours & 0x0F);
    temp.date    = (date >> 4)    * 10 + (date & 0x0F);
    temp.month   = (month >> 4)   * 10 + (month & 0x0F);
    temp.year    = (year >> 4)    * 10 + (year & 0x0F);

    return temp;
}

// Private Functions

void DS1302::send(uint8_t dat)
{
    //Send Byte Data on positive Edges
    for(uint8_t i = 0; i < 8; i++)
    {
        digitalWrite(this->io, (dat >> i)&0x01);
        delayMicroseconds(1);
        digitalWrite(this->clk, HIGH);
        delayMicroseconds(1);
        digitalWrite(this->clk, LOW);
    }
}

uint8_t DS1302::read()
{
    //Receive Byte Data on negative Edges
    uint8_t temp = 0;

    for(uint8_t i = 0; i < 8; i++)
    {
        temp |= digitalRead(this->io) << i;
        if(i < 7)
        {
            digitalWrite(this->clk, HIGH);
            delayMicroseconds(1);
            digitalWrite(this->clk, LOW);
            delayMicroseconds(1);
        }
    }
    return temp;
}