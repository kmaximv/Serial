

String sp_startMarker;           // Переменная, содержащая маркер начала пакета
String sp_stopMarker;            // Переменная, содержащая маркер конца пакета
String sp_dataString;            // Здесь будут храниться принимаемые данные
int sp_startMarkerStatus;        // Флаг состояния маркера начала пакета
int sp_stopMarkerStatus;         // Флаг состояния маркера конца пакета
int sp_dataLength;               // Флаг состояния принимаемых данных
boolean sp_packetAvailable;      // Флаг завершения приема пакета


String parseArray[3];            //Распарсенный массив принимаемых данных

char delimiter = '&';

int pwmState[11];


int maxBright = 255;               // Больше число, больше яркость при максимальной подсветке
int midBright = 125;              // Больше число, больше яркость при средней подсветке
int moonBright = 30;              // Больше число, больше яркость при минимальной подсветке
bool up = true;                      
bool down = false;




void sp_SetUp()
{
  sp_startMarker = "<bspm>";     // Так будет выглядеть маркер начала пакета
  sp_stopMarker = "<espm>";      // Так будет выглядеть маркер конца пакета
  sp_dataString.reserve(64);     // Резервируем место под прием строки данных
  sp_ResetAll();                 // Полный сброс протокола
}



void sp_ResetAll()
{
  sp_dataString = "";           // Обнуляем буфер приема данных
  sp_Reset();                   // Частичный сброс протокола
}



void sp_Reset()
{
  sp_startMarkerStatus = 0;     // Сброс флага маркера начала пакета
  sp_stopMarkerStatus = 0;      // Сброс флага маркера конца пакета
  sp_dataLength = 0;            // Сброс флага принимаемых данных
  sp_packetAvailable = false;   // Сброс флага завершения приема пакета
}



void sp_Send(String data)
{
  Serial.print(sp_startMarker);         // Отправляем маркер начала пакета
  Serial.write(data.length());          // Отправляем длину передаваемых данных
  Serial.print(data);                   // Отправляем сами данные
  Serial.print(sp_stopMarker);          // Отправляем маркер конца пакета
}



void serialEvent() 
{
 sp_Read();                         // Вызов «читалки» принятых данных
 if(sp_packetAvailable)             // Если после вызова «читалки» пакет полностью принят
 {
  ParseCommand();                   // Обрабатываем принятую информацию
  sp_ResetAll();                    // Полный сброс протокола.
 }
}





void sp_Read()
{
  while(Serial.available() && !sp_packetAvailable)            // Пока в буфере есть что читать и пакет не является принятым
  {
    char bufferChar = Serial.read();                           // Читаем очередной байт из буфера
    if(sp_startMarkerStatus < sp_startMarker.length())        // Если стартовый маркер не сформирован (его длинна меньше той, которая должна быть) 
    {  
       if(sp_startMarker[sp_startMarkerStatus] == bufferChar)   // Если очередной байт из буфера совпадает с очередным байтом в маркере
       {
         sp_startMarkerStatus++;                                // Увеличиваем счетчик совпавших байт маркера
       }
       else
       {
         sp_ResetAll();                                         // Если байты не совпали, то это не маркер. Нас нае****, расходимся. 
       }
    }  
    else
    {
     // Стартовый маркер прочитан полностью
       if(sp_dataLength <= 0)                                 // Если длинна пакета на установлена
       {
         sp_dataLength = atoi(&bufferChar);                        // Значит этот байт содержит длину пакета данных
       }
      else                                                    // Если прочитанная из буфера длинна пакета больше нуля
      {
        if(sp_dataLength > sp_dataString.length())            // Если длинна пакета данных меньше той, которая должна быть
        {
          sp_dataString += (char)bufferChar;                  // прибавляем полученный байт к строке пакета
        }
        else                                                  // Если с длинной пакета данных все нормально
        {
          if(sp_stopMarkerStatus < sp_stopMarker.length())    // Если принятая длинна маркера конца пакета меньше фактической
          {
            if(sp_stopMarker[sp_stopMarkerStatus] == bufferChar)  // Если очередной байт из буфера совпадает с очередным байтом маркера
            {
              sp_stopMarkerStatus++;                              // Увеличиваем счетчик удачно найденных байт маркера
              if(sp_stopMarkerStatus == sp_stopMarker.length())
              {
                // Если после прочтения очередного байта маркера, длинна маркера совпала, то сбрасываем все флаги (готовимся к приему нового пакета)
                Serial.println("Packet recieve!");
                sp_Reset();    
                sp_packetAvailable = true;                        // и устанавливаем флаг готовности пакета
              }
            }
            else
            {
              sp_ResetAll();                                      // Иначе это не маркер, а х.з. что. Полный ресет.
            }
          }
          //
        }
      } 
    }    
  }
}





// Стандартный сетап.
void setup() 
{
    //TCCR0B = TCCR0B & 0b11111000 | 0x02; // Устанавливаем ШИМ на 62.5 кГц (пины 5,6)
    //TCCR1B = TCCR1B & 0b11111000 | 0x02; // Устанавливаем ШИМ на 62.5 кГц (пины 9,10)

	pinMode(5, OUTPUT);
	pinMode(6, OUTPUT);
	pinMode(9, OUTPUT);
	pinMode(10, OUTPUT);

  Serial.begin(9600);                               // Инициализируем последовательный интерфейс
  sp_SetUp();                                       // Инициализируем протокол.
  
}

// Стандартный цикл
void loop() 
{
  serialEvent();
  //delay(100);
}

//<bspm>p5on<espm>

bool ParseCommand() {
  uint8_t z = 0;
  for ( int i = 0; i < sp_dataString.length(); i++ ) {
    if (sp_dataString[i] != delimiter ) {
      parseArray[z] += sp_dataString[i];
    } else if (sp_dataString[i] == delimiter ) {
      z++;
    } 
    if (z > 4) {
      Serial.println("Error Parse Command");
      for ( int a = 0; a < 3; a++ ) {
        parseArray[a] = "";
      }
      return false;
    }
  }

  for ( int p = 0; p < 3; p++ ) {
    Serial.print(parseArray[p]);   Serial.print(" ");
  }
  Serial.println("<--");


  if(sp_dataString == "p5on") {
    sp_Send(sp_dataString);
    PWMOn(5);      
  }
  if(sp_dataString == "p5off") {
    sp_Send(sp_dataString);
    PWMOff(5);      
  }

  for ( uint8_t s = 0; s < 3; s++ ) {
    parseArray[s] = "";
  }

  
}



void PWMOn(int pin_num) {

    if (pwmState[pin_num] == 2)
    {
	    Serial.println("Mid to Max");
	    FadeSwitch(pin_num, midBright, maxBright, up);
    }
    else if (pwmState[pin_num] == 3)
    {
	    Serial.println("Min to Max");
	    FadeSwitch(pin_num, moonBright, maxBright, up);
    }
    else
    {
		Serial.println("On");
		FadeSwitch(pin_num, 0, maxBright, up);
    }

	pwmState[pin_num] = 1;
}


void PWMMid(int pin_num) {

    Serial.println("Mid");
    FadeSwitch(pin_num, maxBright, midBright, down);
	pwmState[pin_num] = 2;
}


void PWMMin(int pin_num) {

    Serial.println("Min");
    FadeSwitch(pin_num, midBright, moonBright, down);
	pwmState[pin_num] = 3;
}


void PWMOff(int pin_num) {

    Serial.println("Off");
    FadeSwitch(pin_num, moonBright, 0, down);
	pwmState[pin_num] = 0;
}



void FadeSwitch (int pin, int x, int y, bool z)
{
  if (z)
  {
    for (int i = x; i <= y; i++)
    {
      analogWrite(pin, i);
      delay(15);
    }    
  }
  else
  {
    for (int i = x; i >= y; i--)
    {
      analogWrite(pin, i);
      delay(15);
    }   
  }  
}



/* Вычисляем CRC для первых трёх байт буфера */
// uint8_t crc = crc8_ccitt_block(packet, sizeof(packet) - 1);

/* Вспомогательная функция вычисления CRC для массива байтов */
uint8_t crc8_ccitt_block(const uint8_t *data, size_t length)
{
 uint8_t crc = 0;

 for (size_t i = 0; i < length; ++i)
    crc = crc8_ccitt(crc, data[i]);

 return crc;
}

uint8_t crc8_ccitt(uint8_t crc, uint8_t data)
{
 return crc8(crc, data, 0x07);
}

uint8_t crc8(uint8_t crc, uint8_t data, uint8_t polynomial)
{
 crc ^= data;

 for (int i = 0; i < 8; ++i)
    crc = (crc << 1) ^ ((crc & 0x80) ? polynomial : 0);

 return crc;
}