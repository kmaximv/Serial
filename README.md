# Serial 

Прошивка для Arduino Pro Mini. Используется для управления Pro Mini по Serial интерфейсу.

## Примеры команд управления по UART

#### Запускаем переодическую отправку значений с 3 аналог. порта каждые 10 секунд
```bash
<beg>9!a&03&10&s<end>
```
9 - Кол-во байт передаваемых данных без CRC байта
! - CRC байт
a - тип команды
& - разделитель
03 - Аналоговый порт
10 - переодичность запуска
s - в секундах (m - в минутах, h - в часах)

#### Отключаем переодическую отправку значений с 3 аналог. порта.
Для этого переодичность запуска ставим 0
```bash
<beg>9!a&03&00&s<end>
```

#### Получить значение с 3 аналог. порта
```bash
<beg>4!ra&3<end>
```

#### Считываем значение с 3 цифрового порта (0 или 1)
```bash
<beg>4!rd&3<end>
```

#### Устанавливаем ШИМ на 3 цифровом порту в значение 100 (диапазон от 0 до 180)
```bash
<beg>8!p&03&100<end>
```

```bash
<beg>6!out&03<end>			//pinMode(3, OUTPUT);
<beg>4!in&3<end>			//pinMode(3, INPUT);
<beg>5!on&03<end>			//digitalWrite(3, HIGH);
<beg>6!off&03<end>			//digitalWrite(3, LOW);
```


## Настройка прошивки Arduino Pro Mini

Прошивать будем при помощи Arduino UNO.Сначала нужно загрузить в Arduino UNO скетч ArduinoISP изменив в нем скорость порта Serial на 19200

[![ArduinoISP](/screenshots/ArduinoISP.png)](/screenshots/ArduinoISP.png)

Затем редактируем файл boards.txt. Нужно установить скорость 19200 и загрузчик optiboot.

C:\Program Files (x86)\Arduino\hardware\arduino\avr\boards.txt
```bash
## Arduino Pro or Pro Mini (3.3V, 8 MHz) w/ ATmega328
## --------------------------------------------------
pro.menu.cpu.8MHzatmega328=ATmega328 (3.3V, 8 MHz)

pro.menu.cpu.8MHzatmega328.upload.maximum_size=30720
pro.menu.cpu.8MHzatmega328.upload.maximum_data_size=2048
pro.menu.cpu.8MHzatmega328.upload.speed=19200

pro.menu.cpu.8MHzatmega328.bootloader.low_fuses=0xFF
pro.menu.cpu.8MHzatmega328.bootloader.high_fuses=0xDA
pro.menu.cpu.8MHzatmega328.bootloader.extended_fuses=0x05
pro.menu.cpu.8MHzatmega328.bootloader.file=optiboot/optiboot_atmega328-Mini.hex

pro.menu.cpu.8MHzatmega328.build.mcu=atmega328p
pro.menu.cpu.8MHzatmega328.build.f_cpu=8000000L
```

##Подключение

UNO -> Pro Mini
- +5v -> Vcc
- GND -> GND
- D10 -> RST
- D11 -> D11
- D12 -> D12
- D13 -> D13


## Прошивка

При прошивке, к Serial порту Arduino UNO не должно быть ничего подключено.

- Передёргиваем питание Arduino UNO
- Прошиваем Загрузчик на Pro Mini
- Прошиваем скетч на Pro Mini

Если сразу прошивать скетч, то выдает ошибку контрольной суммы.


## Arduino UNO в качастве USB-UART

  UNO 	-> Pro Mini
- +5v   -> Vcc
- GND   -> GND
- 0(RX) -> TX0
- 1(TX) -> RXI
- RST - | Перемычка 
- GND - |