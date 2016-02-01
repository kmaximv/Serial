/* Команды управления

<beg>6!out&03<end>
<beg>5!in&03<end>
<beg>5!on&03<end>
<beg>6!off&03<end>
<beg>5!rd&03<end>

<beg>8!p&03&100<end>

<beg>4!ra&3<end>
*/

#define DEBUG
//#define CRC_ENABLE


#define DIGITAL_PINS 14   //Кол-во цифровых входов/выходов
#define PARSE_CELLS 4     //Кол-во ячеек в массиве принимаемых данных
#define DATA_LENGTH 10    //Максимальный размер пакета данных без маркеров и CRC
#define OFF 0
#define MIN 10
#define MID 60
#define MAX 180                 
#define UP true 
#define DOWN false


String sp_startMarker = "<beg>";           // Переменная, содержащая маркер начала пакета
String sp_stopMarker  = "<end>";            // Переменная, содержащая маркер конца пакета
String sp_dataString;            // Здесь будут храниться принимаемые данные
uint8_t sp_data[DATA_LENGTH];             
int sp_startMarkerStatus;        // Флаг состояния маркера начала пакета
int sp_stopMarkerStatus;         // Флаг состояния маркера конца пакета
uint8_t sp_dataLength;               // Флаг состояния принимаемых данных
boolean sp_packetAvailable;      // Флаг завершения приема пакета
uint8_t crc_byte;

String parseArray[PARSE_CELLS];            //Распарсенный массив принимаемых данных

char delimiter = '&';             // Разделительный символ в пакете данных

//На каких пинах поддерживается ШИМ ATmega328 3, 5, 6, 9, 10, 11
bool PWMOn[DIGITAL_PINS] = {false,false,false,true,false,true,true,false,false,true,true,true,false,false}; 

int statePins[DIGITAL_PINS];
int cycleStart[DIGITAL_PINS];
int cycleNow[DIGITAL_PINS];
int cycleEnd[DIGITAL_PINS];
unsigned long timers[DIGITAL_PINS];
unsigned long delays[DIGITAL_PINS] = {30,30,30,30,30,30,30,30,30,30,30,30,30,30};



void sp_Reset(){
  sp_dataString = "";           // Обнуляем буфер приема данных
  sp_startMarkerStatus = 0;     // Сброс флага маркера начала пакета
  sp_stopMarkerStatus = 0;      // Сброс флага маркера конца пакета
  sp_dataLength = 0;            // Сброс флага принимаемых данных
  sp_packetAvailable = false;   // Сброс флага завершения приема пакета
  crc_byte = 0;
}


void sp_Send(String data){
  Serial.print(sp_startMarker);         // Отправляем маркер начала пакета
  //Serial.write(data.length());          // Отправляем длину передаваемых данных
  Serial.print(data);                   // Отправляем сами данные
  Serial.println(sp_stopMarker);          // Отправляем маркер конца пакета
}


void serialEvent(){
  sp_Read();                         // Вызов «читалки» принятых данных
  if(sp_packetAvailable){             // Если после вызова «читалки» пакет полностью принят
    ParseCommand();                   // Обрабатываем принятую информацию
    sp_Reset();                    // Полный сброс протокола.
  }
}


void sp_Read()
{
  while(Serial.available() && !sp_packetAvailable) {                   // Пока в буфере есть что читать и пакет не является принятым
    uint8_t bufferChar = Serial.read();                               // Читаем очередной байт из буфера
    if(sp_startMarkerStatus < sp_startMarker.length()) {              // Если стартовый маркер не сформирован (его длинна меньше той, которая должна быть) 
      if(sp_startMarker[sp_startMarkerStatus] == bufferChar) {        // Если очередной байт из буфера совпадает с очередным байтом в маркере
       sp_startMarkerStatus++;                                        // Увеличиваем счетчик совпавших байт маркера
      } else {
       sp_Reset();                                                 // Если байты не совпали, то это не маркер. Нас нае****, расходимся. 
      }
    } else {
     // Стартовый маркер прочитан полностью
      if(sp_dataLength <= 0) {                                        // Если длинна пакета не установлена
        sp_dataLength = (int)bufferChar - 48;                          // Значит этот байт содержит длину пакета данных
        #ifdef DEBUG
        Serial.println();   Serial.println();
        Serial.print(F("sp_dataLength: "));  Serial.println(sp_dataLength);
        #endif
      } else if (crc_byte <= 0) { 
        crc_byte = bufferChar;                                        // Значит этот байт содержит контрольную сумму пакета данных
        #ifdef DEBUG
        Serial.print(F("crc_byte: "));  Serial.println(crc_byte);
        #endif
      } else {                                                        // Если прочитанная из буфера длинна пакета больше нуля
        if(sp_dataLength > sp_dataString.length()) {                  // Если длинна пакета данных меньше той, которая должна быть
          sp_dataString += (char)bufferChar;                          // прибавляем полученный байт к строке пакета
        } else {                                                      // Если с длинной пакета данных все нормально
          if(sp_stopMarkerStatus < sp_stopMarker.length()) {          // Если принятая длинна маркера конца пакета меньше фактической
            if(sp_stopMarker[sp_stopMarkerStatus] == bufferChar) {    // Если очередной байт из буфера совпадает с очередным байтом маркера
              sp_stopMarkerStatus++;                                  // Увеличиваем счетчик удачно найденных байт маркера
              if(sp_stopMarkerStatus == sp_stopMarker.length()) {
                #ifdef DEBUG
                Serial.println(F("Packet recieve!"));
                Serial.println(sp_dataString);
                #endif
                sp_packetAvailable = true;                            // и устанавливаем флаг готовности пакета
              }
            } else {
              sp_Reset();                                          // Иначе это не маркер, а х.з. что. Полный ресет.
            }
          }
        }
      } 
    }    
  }
}


void printByte(uint8_t *data) {
  Serial.println(F("printByte =================="));
  uint8_t length = sp_dataLength;
  Serial.print(F("length: ")); Serial.println(length);
  for (uint8_t i = 0;  i < length; ++i){
    Serial.print(data[i], DEC); Serial.print(F(" "));
  }
  Serial.println();
  Serial.println(F("Done ======================="));
}


bool crcCheck() {
  sp_dataString.getBytes(sp_data, sp_dataLength + 1); // + 1 для дополнительного символа окончания строки
  #ifdef DEBUG
  printByte(sp_data);
  #endif
  uint8_t crc = crc8_ccitt_block(sp_data, sp_dataLength);
  for (size_t i = 0;  i < DATA_LENGTH; ++i){
    sp_data[i] = 0;
  }
  uint8_t crcControl = crc_byte;
  #ifdef DEBUG
  Serial.print(F("CRC:        "));    Serial.println(crc);    Serial.print(F("crcControl: "));    Serial.println(crcControl);
  #endif
  if (crc == crcControl) {
    #ifdef DEBUG
    Serial.println(F("CRC OK!"));
    #endif
    return true;
  } else {
    #ifdef DEBUG
    Serial.println(F("CRC Error!"));
    #endif
    return false;
  }
}


/* Вспомогательная функция вычисления CRC для массива байтов */
uint8_t crc8_ccitt_block(uint8_t *data, size_t length){
  uint8_t crc = 0;
  for (size_t i = 0; i < length; ++i) {
    crc = crc8_ccitt(crc, data[i]);
    #ifdef DEBUG
    Serial.print(crc, DEC); Serial.print(F(" "));
    #endif
  }
  return crc;
}


uint8_t crc8_ccitt(uint8_t crc, uint8_t data){
  return crc8(crc, data, 0x07);
}


uint8_t crc8(uint8_t crc, uint8_t data, uint8_t polynomial){
  crc ^= data;
  for (size_t i = 0; i < 8; ++i){
    crc = (crc << 1) ^ ((crc & 0x80) ? polynomial : 0);
  }
  return crc;
}


bool ParseCommand() {

  #ifdef CRC_ENABLE
  if (!crcCheck()) {
    return false;
  }
  #endif

  sp_Send(sp_dataString);

  uint8_t z = 0;
  for ( size_t i = 0; i < sp_dataString.length(); i++ ) {
    if (sp_dataString[i] != delimiter ) {
      parseArray[z] += sp_dataString[i];
    } else if (sp_dataString[i] == delimiter ) {
      z++;
    } 
    if (z > PARSE_CELLS) {
      #ifdef DEBUG
      Serial.println(F("Error Parse Command"));
      #endif
      for ( size_t a = 0; a < 3; a++ ) {
        parseArray[a] = "";
      }
      return false;
    }
  }

  #ifdef DEBUG
  for ( size_t p = 0; p < 3; p++ ) {
    Serial.print(parseArray[p]);   Serial.print(F(" "));
  }
  Serial.println(F("<--"));
  #endif

  if (parseArray[0] == "in") {
    pinMode(parseArray[1].toInt(), INPUT);
    #ifdef DEBUG
    Serial.print(F("Set INPUT on pin: ")); Serial.println(parseArray[1]);
    #endif
  }

  if (parseArray[0] == "out") {
    pinMode(parseArray[1].toInt(), OUTPUT);
    statePins[parseArray[1].toInt()] = OFF;
    #ifdef DEBUG
    Serial.print(F("Set OUTPUT on pin: ")); Serial.println(parseArray[1]);
    #endif
  }

  if (parseArray[0] == "rd") {
    int digitalState = digitalRead(parseArray[1].toInt());
    sp_dataString += delimiter;
    sp_dataString += digitalState;
    sp_Send(sp_dataString); 
    #ifdef DEBUG
    Serial.print(F("Read digital pin: ")); Serial.println(parseArray[1]);
    Serial.print(F("State: ")); Serial.println(digitalState);
    #endif
  }

  if (parseArray[0] == "ra") {
    int analogState = analogRead(parseArray[1].toInt());
    sp_dataString += delimiter;
    sp_dataString += analogState;
    sp_Send(sp_dataString); 
    #ifdef DEBUG
    Serial.print(F("Read analog pin: ")); Serial.println(parseArray[1]);
    Serial.print(F("State: ")); Serial.println(analogState);
    #endif
  }

  if (parseArray[0] == "on") {
    digitalWrite(parseArray[1].toInt(), HIGH);
    statePins[parseArray[1].toInt()] = MAX;
    #ifdef DEBUG
    Serial.print(F("Set HIGH on pin: ")); Serial.println(parseArray[1]);
    #endif
  }

  if (parseArray[0] == "off") {
    digitalWrite(parseArray[1].toInt(), LOW);
    statePins[parseArray[1].toInt()] = OFF;
    #ifdef DEBUG
    Serial.print(F("Set LOW on pin: ")); Serial.println(parseArray[1]);
    #endif
  }

  if (parseArray[0] == "p") {
    #ifdef DEBUG
    Serial.print(F("Set PWM on pin: ")); Serial.println(parseArray[1]);
    #endif
    PWMChange(parseArray[1].toInt(), parseArray[2].toInt());
  }

  for ( size_t s = 0; s < PARSE_CELLS; s++ ) {
    parseArray[s] = "";
  }
  
  sp_Reset();
}


void PWMChange(int pin, int bright){
  if (PWMOn[pin] == true){
    if (statePins[pin] < bright){
      FadeSwitch(pin, statePins[pin], bright, UP);
    } else {
      FadeSwitch(pin, statePins[pin], bright, DOWN);
    } 
    statePins[pin] = bright;
  } else {
    #ifdef DEBUG
    Serial.print(F("PWM not support on pin: ")); Serial.println(pin);
    #endif
  }
}


void FadeSwitchDelay(int pin){
  if (millis() - timers[pin] >= delays[pin] && cycleNow[pin] != cycleEnd[pin]){
    timers[pin] = millis();
    float rad = DEG_TO_RAD * cycleNow[pin];    //convert 0-360 angle to radian (needed for sin function)
    int sinOut = constrain((sin(rad) * 128) + 128, 0, 255); //calculate sin of angle as number between 0 and 255
    analogWrite(pin, sinOut);
    cycleNow[pin] = cycleNow[pin] + 1;
  }
}


void FadeSwitch (int pin, int x, int y, bool z){
  if (z){
    x = 270 + x;
    y = map(y, 0, 180, 180, 0);
    y = 450 - y;
  } else {
    x = map(x, 0, 180, 180, 0);
    x = 90 + x;
    y = 270 - y;
  }
  cycleStart[pin] = x;
  cycleNow[pin] = cycleStart[pin];
  cycleEnd[pin] = y;
}


void FadeSwitchLoop(){
  for ( size_t i = 0; i < DIGITAL_PINS; i++ ){
    if (PWMOn[i] == true){
      FadeSwitchDelay(i);
    }
  }
}

void setup() 
{
    //TCCR0B = TCCR0B & 0b11111000 | 0x02; // Устанавливаем ШИМ на 62.5 кГц (пины 5,6)
    //TCCR1B = TCCR1B & 0b11111000 | 0x02; // Устанавливаем ШИМ на 62.5 кГц (пины 9,10)

  sp_dataString.reserve(32);     // Резервируем место под прием строки данных
  sp_Reset();                    // Полный сброс протокола

  Serial.begin(9600);            // Инициализируем последовательный интерфейс
}


void loop() 
{
  FadeSwitchLoop();
  serialEvent();
}