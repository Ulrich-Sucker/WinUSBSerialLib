#pragma region Docs
// ====================================================================================
// 	File Name:
//	Author:		Ulrich Sucker      
// -------------------------------------------------------------------------------------
// 	Version 01.05 - 2024-03-23
//   - Erkennung und Behandlung falscher Werte für Baudrate und COM-Port verbessert
// -------------------------------------------------------------------------------------
// 	Version 01.04 - 2024-03-19
//   - Aus einer COM-Port-Nummer wird ein korrekter COM-Port-String generiert
// -------------------------------------------------------------------------------------
// 	Version 00.30 - 2024-03-18
//   - Bei COM-Port- und Baud-Abfrage und Hilfstexte eingebaut
// -------------------------------------------------------------------------------------
// 	Version 00.00 - 2024-02-27
//   - Base
// -------------------------------------------------------------------------------------
#define WinUSBSerialLib_cpp_Version "01.04.011"
// =====================================================================================
#pragma endregion

#include <windows.h>
#include <iostream>
#include <string>
#include <regex>

#include "WinUSBSerialLib.hpp"

/* ==== SerialPort::SerialPort =========================================================
  FUNCTION:		SerialPort::SerialPort
  
  DESCRIPTION:  
  Herstellen einer Verbindung zu einem über die Schnittstellen des Bettriebssystems
  zu einem seriellen Port/ COM-Port.
  
  INPUT:    
  1.	std::string& portName 
            
  2.	std::string& BaudRate
  
  OUTPUT:   
		HANDLE 
  -------------------------------------------------------------------------------------- */     
SerialPort::SerialPort(std::string& portNameSting, std::string& BaudRate) 
{
    this->connected = false;
    char* PortNameChar = portNameSting.data();
    int intBaudRate = stoi(BaudRate);

    // Create & open COM I/O device. This returns a handle to the COM device
    this->handler = CreateFileA(static_cast<LPCSTR>(PortNameChar),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
        
    // Use Win32 API function to get last error. It returns an integer error code. There
    // are about 16,000 error codes, we only care about two (is device conneted?, and is
    // it already being used by other app?)
    DWORD errMsg = GetLastError();
    
    // The win32 error code for ERROR_FILE_NOT_FOUND is DWORD 2
    if (errMsg == 2)
    {
      printf("Dude, serously, you need to, like, plug the device in first \n");
    }
    // The win32 error code for ERROR_FILE_NOT_FOUND is DWORD 5
    else if (errMsg == 5)
    {
      printf("Dude, another device is already using the device\n");
    }
       
    //if (this->handler == INVALID_HANDLE_VALUE) {
    //    if (GetLastError() == ERROR_FILE_NOT_FOUND) {
    //        std::cerr << "ERROR: Handle was not attached.Reason : " << portName << " not available\n";
    //    } else {
    //        std::cerr << "ERROR!!!\n";
    //    }
    else {
        DCB dcbSerialParameters = { 0 };
        if (!GetCommState(this->handler, &dcbSerialParameters)) 
        {
            std::cerr << "ERROR: Failed to get current serial parameters\n";
        } else {
            dcbSerialParameters.BaudRate = intBaudRate;
            dcbSerialParameters.ByteSize = 8;
            dcbSerialParameters.StopBits = ONESTOPBIT;
            dcbSerialParameters.Parity = NOPARITY;
            dcbSerialParameters.fDtrControl = DTR_CONTROL_ENABLE;
            if (!SetCommState(handler, &dcbSerialParameters)) 
            {
                std::cout << "ALERT: could not set serial port parameters\n";
            } else {
                this->connected = true;
                PurgeComm(this->handler, PURGE_RXCLEAR | PURGE_TXCLEAR);
                Sleep(ARDUINO_WAIT_TIME);
            }
        }
    }
}   
// ---- SerialPort::SerialPort ---------------------------------------------------------


// ==== ~SerialPort ====================================================================
SerialPort::~SerialPort() {
    if (this->connected) {
        this->connected = false;
        CloseHandle(this->handler);
    }
}   // ---- ~SerialPort ----------------------------------------------------------------


/* ==== SerialPort::readSerialPort =====================================================
  DESCRIPTION:  
  Die Funktion holt beim Betriebssytem die über den COM-Port emfangenen Zeichen und 
  füllte den beim Aufruf übergebenen lokalen Buffer mit den empfangenen Zeichen.

  Wurden weniger Zeichen empfangen, als der Buffer gross/lang ist, dann werden alle 
  empfangenen Zeichen in den Buffer geschrieben. Der Speicher des Betriebssystems ist
  anschließend leer.
  
  Wurden mehr Zeichen empfangen, als in den lokalen Buffer passen, dann wird der 
  Buffer bis zum Ende gefuellt und es verbleiben Zeichen im Speicher des 
  Betriebssystems.
  
  INPUT:    
  1. const char* buffer    Buffer, der vom Aufrufenden Programm bereit gestellt
                           werden muss.
  2. unsigned int buf_size Laenge des Buffers.
  
  OUTPUT:   
  1. int bytesRead: Anzahl der gelesenen Zeichen.
     0 :
     
     bytesRead < buf_size : 
        Alle Zeichen, die das Betriebssystem zwischen gespeichert hat, wurden an 
        das aufrufenende Programm übergeben
     
     bytesRead = buf_size : 
        Moeglicher Weise befinden sich noch Zeichen im 
        Zwischenspeicher des Betriebssystems

  2. const char* buffer
  
  EXAMPLE 1:
  Nur ein Zeichen einlesen:
    char charBuffer[1];
    recBuffLen = MyPort.readSerialPort(charBuffer, 1);

    if (recBuffLen > 0) {
      std::cout << charBuffer[0];
    }
  -------------------------------------------------------------------------------------- */
int SerialPort::readSerialPort(const char* buffer, unsigned int buf_size) {
    DWORD bytesRead{};
    unsigned int toRead = 0;  // Anzahl der Zeichen, die eingelesen werden sollen
    ClearCommError(this->handler, &this->errors, &this->status);
    if (this->status.cbInQue > 0) {
        if (this->status.cbInQue > buf_size) {
            toRead = buf_size;
        } else {
            toRead = this->status.cbInQue;
        }
    }
    // ****
    else {
        //std::cout << "NULL" << std::endl;
    }
    // ****
    memset((void*)buffer, 0, buf_size);
    if (ReadFile(this->handler, (void*)buffer, toRead, &bytesRead, NULL)) {
        return bytesRead;
    }
    return 0;
}   
// ---- SerialPort::readSerialPort -----------------------------------------------------


/* ==== SerialPort::readSerialPortUntil ================================================
  FUNCTION:  SerialPort::readSerialPortUntil
  
  DESCRIPTION:  
  
  INPUT:    
  
  OUTPUT:   
  
  EXAMPLE 1:
  
  -------------------------------------------------------------------------------------- */
int SerialPort::readSerialPortUntil(std::string& payload, std::string& until) {
    int untilLen = until.length(), len;
    do {
        char buffer[MAX_DATA_LENGTH];
        int bytesRead = this->readSerialPort(buffer, MAX_DATA_LENGTH);
        if (bytesRead == 0) {
            int endIndex = this->readBuffer.find(until);
            if (endIndex != -1) {
                int index = endIndex + untilLen;
                payload = this->readBuffer.substr(0, index);
                this->readBuffer = this->readBuffer.substr(index);
            }
            return bytesRead;
        }
        this->readBuffer += std::string(buffer);
        len = this->readBuffer.length();
    } while (this->readBuffer.find(until) == std::string::npos);
    int index = this->readBuffer.find(until) + untilLen;
    payload = this->readBuffer.substr(0, index);
    this->readBuffer = this->readBuffer.substr(index);
    return index;
}   
// ---- SerialPort::readSerialPortUntil ------------------------------------------------


/* ==== SerialPort::writeSerialPort ====================================================
// Sending provided buffer to serial port;
// returns true if succeed, false if not
	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
  FUNCTION:  SerialPort::readSerialPortUntil
  
  DESCRIPTION:  
  
  INPUT:    
  
  OUTPUT:   
  
  EXAMPLE 1:
  
  -------------------------------------------------------------------------------------- */
  bool SerialPort::writeSerialPort(const char* buffer, unsigned int buf_size) {
    DWORD bytesSend;
    if (!WriteFile(this->handler, (void*)buffer, buf_size, &bytesSend, 0)) {
        ClearCommError(this->handler, &this->errors, &this->status);
        return false;
    }
    return true;
}   
// ---- writeSerialPort ----------------------------------------------------------------


/* ==== SerialPort::writeSerialPort ====================================================
	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  FUNCTION:  SerialPort::readSerialPortUntil
  
  DESCRIPTION:  
  
  INPUT:    
  
  OUTPUT:   
  
  EXAMPLE 1:
  
  -------------------------------------------------------------------------------------- */
bool SerialPort::writeSerialPort(std::string payload) {
    return this->writeSerialPort(payload.c_str(), payload.length());
}   
// ---- SerialPort::writeSerialPort ----------------------------------------------------


/* ==== SerialPort::isConnected ========================================================
// Checking if serial port is connected
	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  FUNCTION:  SerialPort::readSerialPortUntil
  
  DESCRIPTION:  
  
  INPUT:    
  
  OUTPUT:   
  
  EXAMPLE 1:
  
  -------------------------------------------------------------------------------------- */
  bool SerialPort::isConnected() {
    if (!ClearCommError(this->handler, &this->errors, &this->status)) {
        this->connected = false;
    }
    return this->connected;
}   
// ---- SerialPort::isConnected --------------------------------------------------------


/* ==== SerialPort::closeSerial ========================================================
	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  FUNCTION:  SerialPort::readSerialPortUntil
  
  DESCRIPTION:  
  
  INPUT:    
  
  OUTPUT:   
  
  EXAMPLE 1:
  
  -------------------------------------------------------------------------------------- */
  void SerialPort::closeSerial() {
    CloseHandle(this->handler);
}   
// ---- SerialPort::closeSerial --------------------------------------------------------


/* ==== readCOMPortName ================================================================
  DESCRIPTION:  
  Ziel der Funktion ist es einen geprüften COM-Port-String zurück zu liefern.
  Dazu überprüft die Funktion einen übergebenen String oder liest einen String von der
  Console ein.
  Weiterhin kann sie einen Eingabe- oder Hilfstext ausgeben.
  
  INPUT:
  1. std::string& PortName	Eine Zahl größer als 0
  2. std::string HelpMsg	Ein (HIlfs-) Text, der die Eingabe erleichtern soll
  
  OUTPUT:
  PortName COM3 oder \\.\COM12
  True   wenn ein korrekter Wert übergeben oder eingelsen wurde
  False  sonst

  -------------------------------------------------------------------------------------- */
  bool readCOMPortName( std::string& PortName, std::string HelpMsg = "COM Port:\t" )
{
    /* ---- COM-Port-Nr einlesen, wenn keine Zahl uebergeben wurde oder Zahl = 0 ------- */
	if (( (PortName.length() == 0) || (!std::regex_match (PortName, std::regex("\\d*"))) ) || (PortName == "0"))
	{
		std::string inputStr;
		std::cout << HelpMsg;
		std::cin >> inputStr;

        PortName = inputStr;
	}
    /* ---- Port Nr Pruefen ob Zahl > 0 ------------------------------------------------ */
    if (std::regex_match (PortName, std::regex("\\d*") ))
    {
        // ---- Input ist Zahl --------------------------
        unsigned int PortNr = stoi(PortName);
        if ( PortNr == 0 ) 
        {
            return false; // Es gibt keinen Port 0
        }
        else if (( 0 < PortNr ) && ( PortNr < 10 ))
        {
            PortName = "COM" + PortName;
        } else {
            PortName = "\\\\.\\COM" + PortName;
        }
        return true;
    }
    else 
    {
        PortName = "";
        return false;        // Input ist keine Zahl
    }
} // ---- readCOMPortName --------------------------------------------------------------

/* ==== readBaudRate ===================================================================
  DESCRIPTION:  
  Ziel der Funktion ist es, eine geprüfte Baudrate zurück zu geben
  
  INPUT:
  1. std::string& BaudRate	Eine valide Baudrate 9600, 19200, ...
  2. std::string HelpMsg	Ein (HIlfs-) Text, der die Eingabe erleichtern soll   
  
  OUTPUT:
  PortName 9600
  True   wenn ein korrekter Wert übergeben oder eingelsen wurde
  False  sonst   

  -------------------------------------------------------------------------------------- */ 
bool readBaudRate( std::string& BaudRate, std::string HelpMsg = "Baud Rate:\t")
{
    /* ---- Liste erlaubert Baudraten -------------------------------------------------- */
	std::string BaudRates[] = {"9600", "19200", "57600", "115200"};

    /* ---- Baudrate einlesen, wenn keine Zahl uebergeben wurde ------------------------ */
	if ((BaudRate.length() == 0) || (!std::regex_match (BaudRate, std::regex("\\d+") )))
	{
		std::cout << HelpMsg;
		std::cin >> BaudRate;
	}

    /* ---- Pruefen, ob korrekte Baudrate uebergeben wurde ----------------------------- */
    for (int i = 0; i < BaudRates->length(); i ++)
	{
		if (BaudRates[i] == BaudRate)
		{
			return true;  // Eingegebene Baudrate findet sich in Liste erlaubter Baudraten
		}
	}
	BaudRate = "";
	return false;
} // ---- readBaudRate -----------------------------------------------------------------
