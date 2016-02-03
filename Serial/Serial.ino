/* Команды управления 
============================================================================
Запускаем переодическую отправку значений с 3 аналог. порта каждые 10 секунд
~~~~~~~~~~~~~~~~~~~~~
<beg>9!a&03&10&s<end>
~~~~~~~~~~~~~~~~~~~~~
9 - Кол-во байт передаваемых данных без CRC байта
! - CRC байт
a - тип команды
& - разделитель
03 - Аналоговый порт
10 - переодичность запуска
s - в секундах (m - в минутах, h - в часах)

Отключаем переодическую отправку значений с 3 аналог. порта.
Для этого переодичность запуска ставим 0
~~~~~~~~~~~~~~~~~~~~~
<beg>9!a&03&00&s<end>
~~~~~~~~~~~~~~~~~~~~~

Получить значение с 3 аналог. порта
~~~~~~~~~~~~~~~~
<beg>4щra&3<end>
~~~~~~~~~~~~~~~~


<beg>6љout&03<end>
<beg>4Sin&3<end>
<beg>5Son&03<end>
<beg>6Eoff&03<end>

Считываем значение с 3 цифрового порта (0 или 1)
~~~~~~~~~~~~~~~~
<beg>49rd&3<end>
~~~~~~~~~~~~~~~~

Устанавливаем ШИМ на 3 цифровом порту в значение 100 (диапазон от 0 до 180)
~~~~~~~~~~~~~~~~~~~~
<beg>8дp&03&100<end>
~~~~~~~~~~~~~~~~~~~~
*/

//#define DEBUG
//#define CRC_ENABLE      //Проверка контрольной суммы


#define ANALOG_PINS 6
#define DIGITAL_PINS 14   //Кол-во цифровых входов/выходов
#define PARSE_CELLS 4     //Кол-во ячеек в массиве принимаемых данных
#define DATA_LENGTH 10    //Максимальный размер пакета данных без маркеров и CRC
#define OFF 0
#define MIN 10
#define MID 60
#define MAX 180                 
#define UP true 
#define DOWN false


String startMarker = "<beg>";           // Переменная, содержащая маркер начала пакета
String stopMarker  = "<end>";            // Переменная, содержащая маркер конца пакета
String dataString;            // Здесь будут храниться принимаемые данные
int startMarkerStatus;        // Флаг состояния маркера начала пакета
int stopMarkerStatus;         // Флаг состояния маркера конца пакета
uint8_t dataLength;               // Флаг состояния принимаемых данных
boolean packetAvailable;      // Флаг завершения приема пакета
uint8_t crc_byte;

String parseArray[PARSE_CELLS];            //Распарсенный массив принимаемых данных

char delimiter = '&';             // Разделительный символ в пакете данных

//На каких пинах поддерживается ШИМ ATmega328 3, 5, 6, 9, 10, 11
bool PWMOn[DIGITAL_PINS] = {false,false,false,true,false,true,true,false,false,true,true,true,false,false}; 

int statePins[DIGITAL_PINS];
int cycleStart[DIGITAL_PINS];
int cycleNow[DIGITAL_PINS];
int cycleEnd[DIGITAL_PINS];
unsigned long timerDigitalPin[DIGITAL_PINS];
unsigned long delayDigitalPin[DIGITAL_PINS] = {30,30,30,30,30,30,30,30,30,30,30,30,30,30};


unsigned long timerAnalogPin[ANALOG_PINS] = {0,0,0,0,0,0};
unsigned long delayAnalogPin[ANALOG_PINS] = {0,0,0,0,0,0};


void Reset(){
  dataString = "";           // Обнуляем буфер приема данных
  startMarkerStatus = 0;     // Сброс флага маркера начала пакета
  stopMarkerStatus = 0;      // Сброс флага маркера конца пакета
  dataLength = 0;            // Сброс флага принимаемых данных
  packetAvailable = false;   // Сброс флага завершения приема пакета
  crc_byte = 0;
}


void Send(String data){
  String packet = startMarker;      // Отправляем маркер начала пакета
  packet += String(data.length());  // Отправляем длину передаваемых данных
  packet += String((char)crcCalc(data));  // Отправляем контрольную сумму данных
  packet += data;                   // Отправляем сами данные
  packet += stopMarker;             // Отправляем маркер конца пакета
  Serial.print(packet);
  Serial.println();
}


void serialEvent(){
  Read();                         // Вызов «читалки» принятых данных
  if(packetAvailable){             // Если после вызова «читалки» пакет полностью принят
    ParseCommand();                   // Обрабатываем принятую информацию
    Reset();                    // Полный сброс протокола.
  }
}


void Read()
{
  while(Serial.available() && !packetAvailable) {                   // Пока в буфере есть что читать и пакет не является принятым
    uint8_t bufferChar = Serial.read();                               // Читаем очередной байт из буфера
    if(startMarkerStatus < startMarker.length()) {              // Если стартовый маркер не сформирован (его длинна меньше той, которая должна быть) 
      if(startMarker[startMarkerStatus] == bufferChar) {        // Если очередной байт из буфера совпадает с очередным байтом в маркере
       startMarkerStatus++;                                        // Увеличиваем счетчик совпавших байт маркера
      } else {
       Reset();                                                 // Если байты не совпали, то это не маркер. Нас нае****, расходимся. 
      }
    } else {
     // Стартовый маркер прочитан полностью
      if(dataLength <= 0) {                                        // Если длинна пакета не установлена
        dataLength = (int)bufferChar - 48;                          // Значит этот байт содержит длину пакета данных
        #ifdef DEBUG
        Serial.println();   Serial.println();
        Serial.print(F("dataLength: "));  Serial.println(dataLength);
        #endif
      } else if (crc_byte <= 0) { 
        crc_byte = bufferChar;                                        // Значит этот байт содержит контрольную сумму пакета данных
        #ifdef DEBUG
        Serial.print(F("crc_byte: "));  Serial.println(crc_byte);
        #endif
      } else {                                                        // Если прочитанная из буфера длинна пакета больше нуля
        if(dataLength > dataString.length()) {                  // Если длинна пакета данных меньше той, которая должна быть
          dataString += (char)bufferChar;                          // прибавляем полученный байт к строке пакета
        } else {                                                      // Если с длинной пакета данных все нормально
          if(stopMarkerStatus < stopMarker.length()) {          // Если принятая длинна маркера конца пакета меньше фактической
            if(stopMarker[stopMarkerStatus] == bufferChar) {    // Если очередной байт из буфера совпадает с очередным байтом маркера
              stopMarkerStatus++;                                  // Увеличиваем счетчик удачно найденных байт маркера
              if(stopMarkerStatus == stopMarker.length()) {
                #ifdef DEBUG
                Serial.println(F("Packet recieve!"));
                Serial.println(dataString);
                #endif
                packetAvailable = true;                            // и устанавливаем флаг готовности пакета
              }
            } else {
              Reset();                                          // Иначе это не маркер, а х.з. что. Полный ресет.
            }
          }
        }
      } 
    }    
  }
}


void printByte(uint8_t *data) {
  Serial.println();
  Serial.println(F("printByte =================="));
  uint8_t length = dataLength;
  Serial.print(F("length: ")); Serial.println(length);
  for (uint8_t i = 0;  i < length; ++i){
    Serial.print(data[i], DEC); Serial.print(F(" "));
  }
  Serial.println();
  Serial.print(F("Done ======================="));  Serial.println();
}


bool crcCheck() {
  uint8_t crc = crcCalc(dataString);
  #ifdef DEBUG
  Serial.print(F("CRC:        "));    Serial.println(crc);    Serial.print(F("crcControl: "));    Serial.println(crc_byte);
  #endif
  if (crc == crc_byte) {
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


/* Вспомогательная функция вычисления CRC для строки */
uint8_t crcCalc(String dataStr){

  uint8_t data[DATA_LENGTH];
  size_t length = dataStr.length();
  dataStr.getBytes(data, length + 1); // + 1 для дополнительного символа окончания строки

  #ifdef DEBUG
  printByte(data);
  #endif

  uint8_t crc = 0;
  for (size_t i = 0; i < length; ++i) {
    crc = crc8_ccitt(crc, data[i]);
    #ifdef DEBUG
      Serial.print(crc, DEC); Serial.print(F(" "));
    #endif
  }
  #ifdef DEBUG
    Serial.println();
  #endif

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

  Send(dataString);

  uint8_t z = 0;
  for ( size_t i = 0; i < dataString.length(); i++ ) {
    if (dataString[i] != delimiter ) {
      parseArray[z] += dataString[i];
    } else if (dataString[i] == delimiter ) {
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
    dataString += delimiter;
    dataString += digitalState;
    Send(dataString); 
    #ifdef DEBUG
    Serial.print(F("Read digital pin: ")); Serial.println(parseArray[1]);
    Serial.print(F("State: ")); Serial.println(digitalState);
    #endif
  }

  if (parseArray[0] == "ra") {
    SendAnalogValue(parseArray[1].toInt());
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

  if (parseArray[0] == "a") {
    #ifdef DEBUG
    Serial.print(F("analog pin: "));  Serial.print(parseArray[1]);
    Serial.print(F("  delay read: "));  Serial.print(parseArray[2]);
    Serial.print(F(" "));             Serial.println(parseArray[3]);
    #endif

    timerAnalogPin[parseArray[1].toInt()] = millis();
    if (parseArray[2].toInt() == 0){
      delayAnalogPin[parseArray[1].toInt()] = 0;    // если delay 0, отключаем переодическую отправку данных с аналог порта
    } else if (parseArray[3] == "s"){
      delayAnalogPin[parseArray[1].toInt()] = parseArray[2].toInt() * 1000;
    } else if (parseArray[3] == "m"){
      delayAnalogPin[parseArray[1].toInt()] = parseArray[2].toInt() * 1000 * 60;
    } else if (parseArray[3] == "h"){
      delayAnalogPin[parseArray[1].toInt()] = parseArray[2].toInt() * 1000 * 60 *60;
    }
  }


  for ( size_t s = 0; s < PARSE_CELLS; s++ ) {
    parseArray[s] = "";
  }
  
  Reset();
}


void AnalogLoop(){
  for ( size_t i = 0; i < ANALOG_PINS; i++ ){
    if (delayAnalogPin[i] != 0){
      AnalogReadDelay(i);
    }
  }
}

void SendAnalogValue(int pin){
  int val = analogRead(pin);
  String data = "av";
  data += delimiter;
  data += String(pin);
  data += delimiter;
  data += String(val);
  Send(data); 
  #ifdef DEBUG
    Serial.print(F("Read analog pin: ")); Serial.println(pin);
    Serial.print(F("State: ")); Serial.println(val);
  #endif
}


void AnalogReadDelay(int pin){
  if (millis() - timerAnalogPin[pin] >= delayAnalogPin[pin]){
    timerAnalogPin[pin] = millis();
    SendAnalogValue(pin);
  }
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
  if (millis() - timerDigitalPin[pin] >= delayDigitalPin[pin] && cycleNow[pin] != cycleEnd[pin]){
    timerDigitalPin[pin] = millis();
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

  dataString.reserve(32);     // Резервируем место под прием строки данных
  Reset();                    // Полный сброс протокола

  Serial.begin(115200);            // Инициализируем последовательный интерфейс
}


void loop() 
{
  FadeSwitchLoop();
  AnalogLoop();
  serialEvent();
}