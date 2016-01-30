//#define DEBUG

String sp_startMarker;           // Переменная, содержащая маркер начала пакета
String sp_stopMarker;            // Переменная, содержащая маркер конца пакета
String sp_dataString;            // Здесь будут храниться принимаемые данные
uint8_t sp_data[10];             
int sp_startMarkerStatus;        // Флаг состояния маркера начала пакета
int sp_stopMarkerStatus;         // Флаг состояния маркера конца пакета
uint8_t sp_dataLength;               // Флаг состояния принимаемых данных
boolean sp_packetAvailable;      // Флаг завершения приема пакета
uint8_t crc_byte;

String parseArray[4];            //Распарсенный массив принимаемых данных

char delimiter = '&';

int pwmState[13];

int maxBright = 255;               // Больше число, больше яркость при максимальной подсветке
int midBright = 125;              // Больше число, больше яркость при средней подсветке
int moonBright = 30;              // Больше число, больше яркость при минимальной подсветке
bool up = true;                      
bool down = false;



void sp_SetUp(){
  sp_startMarker = "<bspm>";     // Так будет выглядеть маркер начала пакета
  sp_stopMarker = "<espm>";      // Так будет выглядеть маркер конца пакета
  sp_dataString.reserve(64);     // Резервируем место под прием строки данных
  sp_ResetAll();                 // Полный сброс протокола
}


void sp_ResetAll(){
  sp_dataString = "";           // Обнуляем буфер приема данных
  sp_Reset();                   // Частичный сброс протокола
}


void sp_Reset(){
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
    sp_ResetAll();                    // Полный сброс протокола.
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
       sp_ResetAll();                                                 // Если байты не совпали, то это не маркер. Нас нае****, расходимся. 
      }
    } else {
     // Стартовый маркер прочитан полностью
      if(sp_dataLength <= 0) {                                        // Если длинна пакета не установлена
        sp_dataLength = (int)bufferChar - 48;                          // Значит этот байт содержит длину пакета данных
        #ifdef DEBUG
        Serial.print("sp_dataLength: ");  Serial.println(sp_dataLength);
        #endif
      } else if (crc_byte <= 0) { 
        crc_byte = bufferChar;                                        // Значит этот байт содержит контрольную сумму пакета данных
        #ifdef DEBUG
        Serial.print("crc_byte: ");  Serial.println(crc_byte);
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
                Serial.println("Packet recieve!");
                Serial.println(sp_dataString);
                #endif
                sp_packetAvailable = true;                            // и устанавливаем флаг готовности пакета
              }
            } else {
              sp_ResetAll();                                          // Иначе это не маркер, а х.з. что. Полный ресет.
            }
          }
        }
      } 
    }    
  }
}


void printByte(uint8_t *data) {
  Serial.println("printByte ==================");
  uint8_t length = sp_dataLength;
  Serial.print("length: "); Serial.println(length);
  for (uint8_t i = 0;  i < length; ++i){
    Serial.print(data[i], DEC); Serial.print(" ");
  }
  Serial.println();
  Serial.println("Done =======================");
}


bool crcCheck() {
  sp_dataString.getBytes(sp_data, sp_dataLength + 1); // + 1 для дополнительного символа окончания строки
  #ifdef DEBUG
  printByte(sp_data);
  #endif
  uint8_t crc = crc8_ccitt_block(sp_data, sp_dataLength);
  for (size_t i = 0;  i < 10; ++i){
    sp_data[i] = 0;
  }
  uint8_t crcControl = crc_byte;
  #ifdef DEBUG
  Serial.print("CRC:        ");    Serial.println(crc);    Serial.print("crcControl: ");    Serial.println(crcControl);
  #endif
  if (crc == crcControl) {
    #ifdef DEBUG
    Serial.println("CRC OK!");
    #endif
    return true;
  } else {
    #ifdef DEBUG
    Serial.println("CRC Error!");
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
    Serial.print(crc, DEC); Serial.print(" ");
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

  if (!crcCheck()) {
    return false;
  }

  sp_Send(sp_dataString);

  uint8_t z = 0;
  for ( size_t i = 0; i < sp_dataString.length(); i++ ) {
    if (sp_dataString[i] != delimiter ) {
      parseArray[z] += sp_dataString[i];
    } else if (sp_dataString[i] == delimiter ) {
      z++;
    } 
    if (z > 4) {
      #ifdef DEBUG
      Serial.println("Error Parse Command");
      #endif
      for ( size_t a = 0; a < 3; a++ ) {
        parseArray[a] = "";
      }
      return false;
    }
  }

  #ifdef DEBUG
  for ( size_t p = 0; p < 3; p++ ) {
    Serial.print(parseArray[p]);   Serial.print(" ");
  }
  Serial.println("<--");
  #endif

  if (parseArray[0] == "in") {
    pinMode(parseArray[1].toInt(), INPUT);
    #ifdef DEBUG
    Serial.println("in");
    #endif
  }
  if (parseArray[0] == "out") {
    pinMode(parseArray[1].toInt(), OUTPUT);
    #ifdef DEBUG
    Serial.println("out");
    #endif
  }

  if (parseArray[0] == "readd") {
    int digitalState = digitalRead(parseArray[1].toInt());
    #ifdef DEBUG
    Serial.println("readd");
    #endif
  }
  if (parseArray[0] == "reada") {
    int analogState = analogRead(parseArray[1].toInt());
    #ifdef DEBUG
    Serial.println("reada");
    #endif
  }

  if (parseArray[0] == "on") {
    digitalWrite(parseArray[1].toInt(), HIGH);
    #ifdef DEBUG
    Serial.println("on");
    #endif
  }
  if (parseArray[0] == "off") {
    digitalWrite(parseArray[1].toInt(), LOW);
    #ifdef DEBUG
    Serial.println("off");
    #endif
  }

  if (parseArray[0] == "p") {
    analogWrite(parseArray[1].toInt(), parseArray[2].toInt());
    #ifdef DEBUG
    Serial.println("p");
    #endif
  }

  for ( size_t s = 0; s < 4; s++ ) {
    parseArray[s] = "";
  }
  
  sp_ResetAll();
}


void PWMOn(int pin_num) {

  if (pwmState[pin_num] == 2){
    #ifdef DEBUG
    Serial.println("Mid to Max");
    #endif
    FadeSwitch(pin_num, midBright, maxBright, up);
  } else if (pwmState[pin_num] == 3){
    #ifdef DEBUG
    Serial.println("Min to Max");
    #endif
    FadeSwitch(pin_num, moonBright, maxBright, up);
  } else {
    #ifdef DEBUG
    Serial.println("On");
    #endif
    FadeSwitch(pin_num, 0, maxBright, up);
  }

  pwmState[pin_num] = 1;
}


void PWMMid(int pin_num){
  #ifdef DEBUG
  Serial.println("Mid");
  #endif
  FadeSwitch(pin_num, maxBright, midBright, down);
  pwmState[pin_num] = 2;
}


void PWMMin(int pin_num) {
  #ifdef DEBUG
  Serial.println("Min");
  #endif
  FadeSwitch(pin_num, midBright, moonBright, down);
  pwmState[pin_num] = 3;
}


void PWMOff(int pin_num) {
  #ifdef DEBUG
  Serial.println("Off");
  #endif
  FadeSwitch(pin_num, moonBright, 0, down);
  pwmState[pin_num] = 0;
}


void FadeSwitch (int pin, int x, int y, bool z)
{
  if (z) {
    for (size_t i = x; i <= y; i++) {
      analogWrite(pin, i);
      delay(15);
    }    
  }
  else {
    for (size_t i = x; i >= y; i--) {
      analogWrite(pin, i);
      delay(15);
    }   
  }  
}


void setup() 
{
    //TCCR0B = TCCR0B & 0b11111000 | 0x02; // Устанавливаем ШИМ на 62.5 кГц (пины 5,6)
    //TCCR1B = TCCR1B & 0b11111000 | 0x02; // Устанавливаем ШИМ на 62.5 кГц (пины 9,10)

  Serial.begin(9600);                               // Инициализируем последовательный интерфейс
  sp_SetUp();                                       // Инициализируем протокол.
}


void loop() 
{
  serialEvent();
}