#pragma once
#include <Arduino.h>


#define MAX_DEVICES 5 // Максимальное число датчиков, которые библиотека может запомнить
#define MAX_PAYLOAD_SIZE 26// Запас под самый длинный пакет на шине


// Коды семейств 1-Wire
#define FAMILY_DS18B20  0x28
#define FAMILY_A2D      0xD2
#define FAMILY_RTAFHD3  0x12
 

struct OneWireDevice {
    uint8_t rom[8];
    uint8_t familyCode;
    bool isActive;
    uint8_t payload[MAX_PAYLOAD_SIZE]; // Универсальный сырой массив данных
    uint8_t payloadLen;                 // Длина прочитанного пакета
};

class MyOneWire{


    private:
//======== Varibles ========
    int MyPin;

    int8_t lastZero = -1;
    int8_t lastDiscrepancy = -1;
    uint8_t lastROM[8];
    bool lastDeviceFlag = false;
    
    // Массив для хранения всех найденных адресов прямо внутри класса
    uint8_t allRom[MAX_DEVICES][8]; 

    uint8_t totalDevices = 0;        // Сколько датчиков реально нашли
    uint8_t deviceCount = 0;
    bool deviceFlag;
    uint8_t deviceNumber;

    //Operational functions at bits / bytes
    bool reset();
    void Output();// Send zero to slave
    void Input();// listen answer from slave
    bool Read();// digitalRead command
    bool ReadBit();
    void WriteBit(bool bit);



  
    
    void print();

    

    
    // Метод отправки команды Match ROM и выбора датчика
    void MatchRom();
    
    void readBytes(uint8_t* buffer, uint8_t count); // Вспомогательное последовательное чтение
    bool checkActivity(uint8_t index);             // Проверка на 0xFF
    

//==================PUBLIC===================//


    public:
    //public Functions
    OneWireDevice devices[MAX_DEVICES];
    void WriteByte(uint8_t data);
    uint8_t ReadByte();
    MyOneWire(int pinNumber);// get number of 1-Wire bus
    void SearchRom();
    int ScanBus();
    bool ID_select(uint8_t deviceNumber);
    bool pollDevice(uint8_t index);
};


