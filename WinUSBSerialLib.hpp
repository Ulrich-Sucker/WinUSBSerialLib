#pragma region Docs
// ====================================================================================
// 	File Name:
//	Author:		Ulrich Sucker      
// -------------------------------------------------------------------------------------
//   - Version 00.02 - 2024-**-**     
//      readCOMPortName
//      readBaudRate
// -------------------------------------------------------------------------------------
// 	Version 00.01 - 2024-02-27
//   - Base  Function: 
//      SerialPort::SerialPort
//      SerialPort::~SerialPort
//      SerialPort::readSerialPort
//      SerialPort::readSerialPortUntil
//      SerialPort::writeSerialPort
//      SerialPort::writeSerialPort
//      SerialPort::isConnected
//      SerialPort::closeSerial
// -------------------------------------------------------------------------------------
#define SW2_Version "00.01.003"
// =====================================================================================
#pragma endregion


#pragma once

#define ARDUINO_WAIT_TIME 2000
#define MAX_DATA_LENGTH 1024

#include <windows.h>
#include <iostream>
#include <string>

class SerialPort {
private:
    std::string readBuffer;
    HANDLE handler;
    bool connected;
    COMSTAT status;
    DWORD errors;
public:
    explicit SerialPort(std::string& portName, std::string& baudRate);
    ~SerialPort();
    int readSerialPort(const char* buffer, unsigned int buf_size);
    int readSerialPortUntil(std::string& payload, std::string& until);
    bool writeSerialPort(const char* buffer, unsigned int buf_size);
    bool writeSerialPort(std::string payload);
    bool isConnected();
    void closeSerial();
};
