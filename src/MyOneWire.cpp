#include "MyOneWire.h"




MyOneWire::MyOneWire(int pinNumber){
    MyPin = pinNumber;// PIN ON MCU USED FOR ONE WIRE
}
//====0  ZERO SENDING =====//
void MyOneWire::Output(){
    pinMode(MyPin,OUTPUT);
    digitalWrite(MyPin,0);

}
//====1  OPEN DRAIN INPUT FOR SENDING ONE OR READING BIT  =====//
void MyOneWire::Input(){ 
    pinMode(MyPin,INPUT);

}

// ==== CALLING DIGITAL READ FUNCTION ONCELY ==== //
bool MyOneWire::Read(){
    bool currentStatus;
    currentStatus = digitalRead(MyPin);
    return currentStatus;
}

//!!====== INITIALIZATION OF DEVICES ======!!//
bool MyOneWire::reset(){
    bool status = false;
    
    Output();
    delayMicroseconds(500);
    
    Input();
    delayMicroseconds(40);
    
    if(Read() == LOW){
        delayMicroseconds(200);
        if(Read() == HIGH){
            status = 1;
            //Serial.println("connected");
        }
        else{
            Serial.println("disconnected");
        }   
    }

    delayMicroseconds(100);
    return status;
}
//=======================================================================================================
// SENDING BIT
void MyOneWire::WriteBit(bool bit){
    noInterrupts();
    if(bit==1){
        Output(); 
        delayMicroseconds(15); 
        Input(); 
        interrupts(); 
        delayMicroseconds(80);
    }
    else{
        Output(); 
        delayMicroseconds(45); 
        Input(); 
        interrupts(); 
        delayMicroseconds(50);
    }
}
//=======================================================================================================
//SENDING BYTE
void MyOneWire::WriteByte(uint8_t data){
    for (int i = 0; i<8; i++){
        WriteBit(data & 0x1);
        data>>= 1;
    }
    delayMicroseconds(3000);
}
//=======================================================================================================
// READING BIT
bool MyOneWire::ReadBit(){
    bool bit;
    noInterrupts();

    Output(); 
    delayMicroseconds(4);
    Input();
    delayMicroseconds(14);
    bit = Read();
    interrupts();
    delayMicroseconds(80);
    return bit;

}
//=======================================================================================================
// READING BYTE
uint8_t MyOneWire::ReadByte(){
    uint8_t Byte = 0;
    for(int i = 0; i<8; i++){
        Byte |= ReadBit()<<i;
    }
    delayMicroseconds(3000);
    return Byte;
}

//=======================================================================================================

//SEARCHING ROM 
void MyOneWire::SearchRom(){
 
    // 1. ОЧИСТКА И СБРОС (Строго в самом начале!)
    // =========================================================================
    totalDevices = 0;
    memset(allRom, 0, sizeof(allRom)); // Обнуляем весь массив от фантомных адресов
    lastDiscrepancy = -1;
    lastDeviceFlag = false;
    memset(lastROM, 0, sizeof(lastROM));



    while (!lastDeviceFlag) {
        if(reset()){
            uint8_t rom[8] = {0}; // Создаем локальный черновик перед циклом
            WriteByte(0xF0);// Команда Search ROM (0xF0)
            bool error = false;
            //totalDevices = 0;
            lastZero = -1;
            bool deviceFlag = false;
            
            
            for (uint8_t bit_index = 0; bit_index < 64; bit_index++) {
                uint8_t bit = ReadBit();       // Читаем прямой бит
                uint8_t bit_inv = ReadBit();   // Читаем инверсный бит

                // 1. Если оба бита 1 — обрыв связи / нет устройств
                if (bit == 1 && bit_inv == 1) {
                    Serial.println("disconnected");
                    error = true;
                    break;
                }
                
                uint8_t dir; // Переменная для выбранного направления (0 или 1)

                // 2. КОЛЛИЗИЯ (оба бита 0): на шине есть и 0, и 1
                if (bit == 0 && bit_inv == 0) {

                    
                    

                    
                    
                    if (bit_index < lastDiscrepancy) {
                            // Идем по старому маршруту из lastROM
                        dir = (lastROM[bit_index / 8] >> (bit_index % 8)) & 1;
                            
                            // Если старый маршрут вел в 0, обновляем lastZero
                        if (dir == 0) {
                            lastZero = bit_index;
                        }
                    } 
                    else if (bit_index == lastDiscrepancy) {
                            // На самой развилке сворачиваем в 1
                        dir = 1;
                    } 
                    else { // bit_index > lastDiscrepancy
                            // На новой территории всегда выбираем 0
                        dir = 0;
                        lastZero = bit_index; // Запоминаем развилку
                        deviceFlag = true;
                    }
                }
                    
                
                // 3. Одиночный бит (коллизии нет, выбор однозначен)
                else {
                    dir = bit;
                    
                }

                // --- ОБЩИЙ ШАГ ДЛЯ ВСЕХ ВЕТОК ---
                // Записываем выбранный бит в текущий rom и обновляем lastROM
                if (dir == 1) {
                    rom[bit_index / 8] |= (1 << (bit_index % 8));
                    
                } else {
                    rom[bit_index / 8] &= ~(1 << (bit_index % 8));
                
                }

                // Отправляем выбранный бит обратно на шину
                WriteBit(dir); 
            }

            // Выполняем только при успешном завершении всех 64 бит
            if (!error) {
                //В случае если была колизия, и если индекс старой колизии не совпадает с новой
                if(lastDiscrepancy != lastZero&& deviceFlag){
                    lastDiscrepancy = lastZero;
                    deviceFlag = false;
                    
                }
                else{
                    lastDiscrepancy = -1;
                    lastDeviceFlag = true; // Дошли до конца шины, останавливаем while
                }
                    
                
                if (memcmp(lastROM, rom, 8) != 0){
                    bool isAlreadySaved = false;

                    // 1. Проверяем, существует ли уже текущий rom в нашем массиве allRom
                    for (uint8_t i = 0; i < totalDevices; i++) {
                        if (memcmp(allRom[i], rom, 8) == 0) {
                            isAlreadySaved = true; // Нашли совпадение!
                            break;
                        }
                    }

                    // 2. Записываем только если устройства еще НЕТ в базе
                    if (!isAlreadySaved && totalDevices < MAX_DEVICES) {
                        for (uint8_t j = 0; j < 8; j++) {
                            devices[totalDevices].rom[j] = rom[j]; // Заполняем структуру!
                            allRom[totalDevices][j] = rom[j];
                            lastROM[j] = rom[j];
                        }
                        totalDevices++; // Увеличиваем счетчик только для РЕАЛЬНО НОВОГО устройства
                    }
                
                } 
            
        
            }
            else{
                break;
            }
        print();
        }
    
    else{
        Serial.println("disconnected");
    }
    
    }
}


void MyOneWire::MatchRom(){
    WriteByte(0x55);// Команда Match ROM
    
}


bool MyOneWire::ID_select(uint8_t index) {
    if (reset()) { // 1. Проверяем импульс сброса
        MatchRom(); // 2. Команда Match ROM
        for (uint8_t i = 0; i < 8; i++) { // 3. Передаем 8 байт адреса
            WriteByte(devices[index].rom[i]);
            
        }
        Serial.println("Устройство успешно выбрано");
        return true; // Устройство успешно выбрано
       
    }
    Serial.println("Шина молчит / нет ответа");
    return false; // Шина молчит / нет ответа
    
}


void MyOneWire::readBytes(uint8_t* buffer, uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        buffer[i] = ReadByte();
    }
}

// Если ВСЕ прочитанные байты равны 0xFF, считаем, что датчик отвалился
bool MyOneWire::checkActivity(uint8_t index) {
    for (uint8_t i = 0; i < devices[index].payloadLen; i++) {
        if (devices[index].payload[i] != 0xFF) {
            devices[index].isActive = true;
            return true; // Нашли хотя бы один реальный бит данных
        }
    }
    devices[index].isActive = false;
    return false;
}


bool MyOneWire::pollDevice(uint8_t index) {
    
    Serial.print("Start polling index: ");
    Serial.println(index);
    
    if (index >= totalDevices) {
        Serial.println("Index out of bounds!");
        return false;
    }
    // ...
   

    uint8_t family = devices[index].rom[0];

    switch (family) {

        // --- ТЕРМОМЕТР DS18B20 (9 байт Scratchpad) ---
        case FAMILY_DS18B20: {
            if (!ID_select(index)) { devices[index].isActive = false; return false; }
            WriteByte(0x44); // Convert T

            if (!ID_select(index)) { devices[index].isActive = false; return false; }
            WriteByte(0xBE); // Read Scratchpad

            devices[index].payloadLen = 9; // Читаем полный скратчпад
            readBytes(devices[index].payload, 9);
            
            break;
        }

        // --- ДАТЧИК A2D (5 байт данных) ---
        case FAMILY_A2D: {
            if (!ID_select(index)) { devices[index].isActive = false; return false; }
            WriteByte(0xC0); // Команда запроса A2D (замени на свою)
            WriteByte(0x00);
            WriteByte(0x40);

            devices[index].payloadLen = 5; // Читаем ровно 5 байт A2D
            readBytes(devices[index].payload, 5);
            
            break;
        }

        // --- ДАТЧИК RTAFHD (например, 7 байт данных) ---
        case FAMILY_RTAFHD3: {
            if (!ID_select(index)) { devices[index].isActive = false; return false; }
            WriteByte(0xF5); // Команда чтения RTAFHD
            WriteByte(0x8C);
        // WriteByte(0xFF);
          //  WriteByte(0xCB);
            

            devices[index].payloadLen = 2; // Фиксируем 7 байт
            readBytes(devices[index].payload, 2);
            WriteByte(0x2A);
            reset();
            WriteByte(0x40); // Команда чтения RTAFHD
            WriteByte(0x36);
            WriteByte(0x42); // Команда чтения RTAFHD
            

            devices[index].payloadLen = 26; // Фиксируем 7 байт
            readBytes(devices[index].payload, 26);
            break;
        }

        default:
            devices[index].isActive = false;
            return false;
    }
    
    
    // 1. Проверяем, живой ли датчик после чтения
    bool isAlive = checkActivity(index); 
    
    // 2. Если живой — красиво печатаем его данные
    if (isAlive) {
        Serial.print("Device [");
        Serial.print(index);
        Serial.print("] Family: ");
        Serial.print(family, HEX);
        Serial.print(" | Payload: ");
        
        // Тот самый цикл с ведущим нулем для красивого HEX-вывода
        for (uint8_t i = 0; i < devices[index].payloadLen; i++) {
            if (devices[index].payload[i] < 0x10) Serial.print("0");
            Serial.print(devices[index].payload[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
    else{
        Serial.println("error");
    }
    
    // 3. Возвращаем итоговый статус
    return isAlive;
} 





void MyOneWire::print(){
    Serial.print("totalDevises - ");
    Serial.println(totalDevices); 
    for (int j = 0; j < totalDevices; j++) {
    Serial.print("Device No- ");
    Serial.print(j);
    Serial.print(": ");

    for (uint8_t i = 0; i < 8; i++) {
        // Добавляем ведущий 0 для красивого выравнивания (например, 0F вместо F)
        if (allRom[j][i] < 0x10) Serial.print("0"); 
        
        Serial.print(allRom[j][i], HEX); // Выводим байт в HEX
        Serial.print(" ");               // Пробел МЕЖДУ байтами
    }
    delay(1000);
    Serial.println(); // Перенос строки только ПОСЛЕ того, как напечатаны все 8 байт
    }
 
} 