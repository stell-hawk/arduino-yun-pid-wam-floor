#define ONE_WIRE_BUS 22                               // шина с датчиками
#define ONE_WIRE_PID 26                               // нога с ПИД регулятором !НА ШИНЕ 1 датчик
#define ONE_WIRE_OUT 24                               // нога с датчиком уличной температуры !НА ШИНЕ 1 датчик
#define RELAY_PIN     23                              // выход с реле
#define feed_index     2                              // номер датчика на линии отвечающего за подачу


#define HIGH_LEVEL     1                              //высокий уровень - состояние открытого реле
#define LOW_LEVEL     0                              //низкий уровень - состояние закрытого реле
#define START_LEVEL     LOW_LEVEL                   //состояние в начале работы
long min_power=85;
long max_power=100;


#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
#include <OneWire.h>
#include <FileIO.h>
#include <Console.h>
#include <DallasTemperature.h>                       // (Датчик температуры DS18B20)
#include <HttpClient.h>
#include "io.h"
#include "pid.h"

#define DS18B20_DELAY    800                          //время преобразования температуры
OneWire ds(ONE_WIRE_PID);
OneWire ds2(ONE_WIRE_OUT);
float temperature = 0; 
float temperature_prev = 0; 
float temperature_out = 0; 
float temperature_out_prev = 0; 
long TEMP_UPDATE_TIME;
long BUS_TEMP_UPDATE_TIME; 
//int OUT_ZERO_VALUE;
//int OUT_25_VALUE;


long lastUpdateTime = -TEMP_UPDATE_TIME; // Переменная для хранения времени последнего считывания с датчика
long bus_lastUpdateTime = -BUS_TEMP_UPDATE_TIME; // Переменная для хранения времени последнего считывания с датчика
bool ds_tosend=0;
bool bus_tosend=0;

  byte numberOfDevices;                                // Количество датчиков DS18B20 на шине 1-wire (Обновляется автоматически)
  DeviceAddress Termometers;                           // Массив с датчиками
  // Размер следующих массивов - предполагаемое максимальное количество датчиков DS18B20.
  // По умолчанию - 30. Нужно больше?! Увеличиваете числа и перезаливаете скетч
  byte na[30];                                         // Массив с порядковыми номерами датчиков температуры DS18B20
  String aa[30];                                       // Массив с HEX-адресами датчиков температуры DS18B20
  float ta[30];                                        // Массив с последней отправленной температурой датчиков DS18B20

 OneWire oneWire(ONE_WIRE_BUS);
 DallasTemperature sensors(&oneWire);


YunServer server;

void setup() {
  // Bridge startup
  Bridge.begin();
 
  pinMode(RELAY_PIN,OUTPUT);
  digitalWrite(RELAY_PIN, START_LEVEL);
  
  // создаем  логи
  //сюда пишутся логи с параметрами PID 
  clear_log("/tmp/pid.txt");
  //сюда пишутся данные о включении и выключении реле 
  clear_log("/tmp/pin.txt");
  //сюда складываются данные с шины датчиков
  clear_log("/tmp/bus.txt");
  //сюда складывается информация о температуре pid-регулятора
  clear_log("/tmp/temp.txt");
  //Сюда складываются данные с уличной температурой
  clear_log("/tmp/out.txt");

  //Чтение основных параметров из файлов преднастройки
  K_P=ReadDataFromFile("K_P");//Коэфициент  усиления регулятора
  K_I=ReadDataFromFile("K_I");//интегральная составляющая регулятора
  OUT_ZERO_VALUE=ReadDataFromFile("OUT_ZERO_VALUE");//температура обратки при нулевой температуре на улице
  OUT_25_VALUE=ReadDataFromFile("OUT_25_VALUE");// температура обратки  при -25 на улице
  
   // Определяем периодичность проверок pid
  CYCLE=ReadDataFromFile("CYCLE");
  TEMP_UPDATE_TIME=CYCLE*1000;
  
  BUS_TEMP_UPDATE_TIME=ReadDataFromFile("BUS_TEMP_UPDATE_TIME");//определяем периодичность проверок на шине
  
  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();
}

//функция чистит файлы логов при перезапуске системы
void clear_log(String  FileName)
{
  File dataFile = FileSystem.open(FileName.c_str(), FILE_WRITE);  
  dataFile.println(getTimeStamp());
  dataFile.close();
}


//основная функция системы
float detectTemperature(){
  float power=-100;
  //если таймер прошел то шлем запрос на вычисление температуры
  if (millis() - lastUpdateTime > TEMP_UPDATE_TIME)
  {
  lastUpdateTime = millis();
  ds.reset();
  ds.write(0xCC,1);
  ds.write(0x44,1);
  ds2.reset();
  ds2.write(0xCC,1);
  ds2.write(0x44,1);
    byte data[2];
    ds.reset();
    ds.write(0xCC,1);
    ds.write(0xBE,1);
    data[0] = ds.read();
    data[1] = ds.read();
    temperature_prev = temperature;
    temperature =  ((data[1] << 8) | data[0]) * 0.0625;
    
    // вычисляем PID
    ds2.reset();
    ds2.write(0xCC,1);
    ds2.write(0xBE,1);
    data[0] = ds2.read();
    data[1] = ds2.read();
    temperature_out_prev = temperature_out;
    temperature_out =  ((data[1] << 8) | data[0]) * 0.0625;
    SET_VALUE=setvalue(temperature_out);
    power=pid3(temperature,ta[2]);
    // если температура поменялась фиксируем
    //if(temperature_prev!=temperature ||D_T!=0.00)
    {
      File dataFile = FileSystem.open("/tmp/temp.txt", FILE_APPEND);  
      dataFile.print(getTimeStamp());
      dataFile.print(" -> ");
      dataFile.print(SET_VALUE);
      dataFile.print(" -> ");
      dataFile.print(temperature);
      dataFile.print(" -> ");
      dataFile.println(power);
      //dataFile.println(String("http://192.168.220.100/hawk/dragino.php?value=")+temperature+"&SET_VALUE="+SET_VALUE+"&power="+power+"&CONSTRAIN="+CONSTRAIN+"&K_P="+K_P+"&K_I="+K_I+"&K_D="+K_D);
      dataFile.close();
      HttpClient client;
      client.get(String("http://192.168.220.100/hawk/dragino.php?value=")+temperature+"&SET_VALUE="+SET_VALUE+"&SET_VALUE2="+SET_VALUE2+"&power="+power+"&CONSTRAIN="+CONSTRAIN+"&K_P="+K_P+"&K_I="+K_I+"&K_D="+K_D+"&DIFF_SUM="+DIFF_SUM);
      
    }    

    if(temperature_out_prev!=temperature_out)
    {
      File dataFile = FileSystem.open("/tmp/out.txt", FILE_APPEND);  
      dataFile.print(getTimeStamp());
      dataFile.print(" temperature_out -> ");
      dataFile.print(temperature_out);
      dataFile.print(" SET_VALUE -> ");
      dataFile.println(SET_VALUE);
      dataFile.close();
    }
 }
return power;
}


//КОд 


//функция чтения данных датчиком с шины температур
void detectbus()
{
  File dataFile = FileSystem.open("/tmp/bus.txt", FILE_APPEND);  
  
  //if(PID_DEBUG)dataFile.println(String(String(" bus start:")+millis()+" millis - bus_lastUpdateTime:"+(millis() - bus_lastUpdateTime)+" BUS_TEMP_UPDATE_TIME:"+BUS_TEMP_UPDATE_TIME+" bus_tosend:"+ bus_tosend));
// Поиск датчиков температуры DS18B20 на шине 1-wire
  if ((millis() - bus_lastUpdateTime) >= BUS_TEMP_UPDATE_TIME)
  {
  dataFile.println(millis());
  sensors.begin();                                   // Инициализация датчиков DS18B20
  sensors.requestTemperatures();                     // Перед каждым получением температуры надо ее запросить
  sensors.setResolution(Termometers, 12);            // Установка чувствительности на 12 бит 
  numberOfDevices = sensors.getDeviceCount(); 
  for(byte i=0;i<numberOfDevices; i++) {            // цикл для записи в массивы данных
     if(sensors.getAddress(Termometers, i))          // если адрес датчика под индексом i определен, то:
      {        
      String aba[8];                                 // Создаем массив с байтами адреса 
      for (int i=0; i<8; i++)                        // цикл получения бит адреса
        { 
          if (Termometers[i] < 16) { aba[i] = "0"+String(Termometers[i],HEX); } //если бит меньше 16, то дописываем в начале 0, так же конвертируем в шеснадцатиричный код
          else {aba[i] = String(Termometers[i],HEX);}// в противном случае просто конвертируем в 16ричный код и записываем байт в массив
        }
      aa[i] = aba[0]+aba[1]+aba[2]+aba[3]+aba[4]+aba[5]+aba[6]+aba[7]; // Записываем полный адрес в массив адресов aa[], в ячейку i.      
    if(PID_DEBUG){
      dataFile.print("aa[");
      dataFile.print(i);
      dataFile.print("]: ");
      dataFile.println(aa[i]);
      }
     }
   
   }
   bus_tosend=1;
   bus_lastUpdateTime = millis();
 }
  //dataFile.print("middle bus ");//6374 //7282
  //dataFile.println(millis());
  
  if(bus_tosend && ((millis() - bus_lastUpdateTime) >= DS18B20_DELAY))
  {
    bus_tosend=0;
    // Получение и обработка температуры
    for(byte i=0;i<numberOfDevices; i++)                 // цикл для получения, обработки, округления и отправки температуры каждого датчика DS18B20
        { 
           float tempC = sensors.getTempCByIndex(i);
           ta[i]=tempC;
          if(PID_DEBUG){
            dataFile.print("ta[");
            dataFile.print(i);
            dataFile.print("]: ");
            dataFile.println(ta[i]);
          }
        }
    }
  
  //dataFile.print("end bus ");//8071
  //dataFile.println(millis());
  //dataFile3.println(getTimeStamp());
  dataFile.close();

}  

//
bool dataCommand(YunClient client) {
  String name="";
  int value=0;
  // Read pin number
  name =client.readStringUntil('/');
  name.trim();

  //отображаем все термометры 
  if(name=="sensors")
  {
        client.print("pid : ");
        client.println(temperature);
        client.print("out : ");
        client.println(temperature_out);
     for(byte i=0;i<numberOfDevices; i++) {
     if(sensors.getAddress(Termometers, i))          // если адрес датчика под индексом i определен, то:
      {  
        client.print(aa[i]);
        client.print(" : ");
        client.println(ta[i]);
      }
     }
    return true;
  }
  //отображаем все переменные
  if(name=="values")
  {
    client.print("SET_VALUE : ");client.println(SET_VALUE);
    client.print("K_P : ");client.println(K_P);
    client.print("K_I : ");client.println(K_I);
    client.print("K_D : ");client.println(K_D);
    client.print("CYCLE : ");client.println(CYCLE);
    client.print("SUM_D_T : ");client.println(SUM_D_T);
    client.print("DELTA_PV_PV2 : ");client.println(DELTA_PV_PV2);
    client.print("BUS_TEMP_UPDATE_TIME : ");client.println(BUS_TEMP_UPDATE_TIME);
    client.print("min_power : ");client.println(min_power);
    client.print("OUT_ZERO_VALUE : ");client.println(OUT_ZERO_VALUE);
    client.print("OUT_25_VALUE : ");client.println(OUT_25_VALUE);
    return true;
  }

  
  // If the next character is a '/' it means we have an URL
  // with a value like: "/data/K_P/1"
  //данный код позволяет считывать состояние конкретной переменной и изменять её
  value=client.parseFloat();
  if (value) {
    client.print(F("value named "));
    client.print(name);
    client.print(F(" set to "));
    if(name=="K_P")
    {
      K_P=value;
      client.println(K_P);
    }
    if(name=="SET_VALUE")
    {
      SET_VALUE=value;
      client.println(SET_VALUE);
    }  
    if(name=="K_I")
    {
      K_I=value;
      client.println(K_I);
    }  
    if(name=="K_D")
    {
      K_D=value;
      client.println(K_D);
    }  
    if(name=="CYCLE")
    {
      CYCLE=value;
      TEMP_UPDATE_TIME=CYCLE*1000;
      client.println(CYCLE);
    }  
    if(name=="SUM_D_T")
    {
      SUM_D_T=value;
      client.println(SUM_D_T);
    }  
    if(name=="min_power")
    {
      min_power=value;
      client.println(min_power);
    }      
    if(name=="max_power")
    {
      max_power=value;
      client.println(max_power);
    }      
    if(name=="DELTA_PV_PV2")
    {
      DELTA_PV_PV2=value;
      client.println(DELTA_PV_PV2);
    }              
    if(name=="OUT_ZERO_VALUE")
    {
      OUT_ZERO_VALUE =value;
      client.println(OUT_ZERO_VALUE);
    }                  
    if(name=="OUT_25_VALUE")
    {
      OUT_25_VALUE=value;
      client.println(OUT_25_VALUE);
    }                  
    if(name=="BUS_TEMP_UPDATE_TIME")
    {
      BUS_TEMP_UPDATE_TIME=value;
      client.println(BUS_TEMP_UPDATE_TIME);
    }                      
  } 
  else {
  client.print(F("value named "));
  client.print(name);
  client.print(F(" not changed and have value "));
    if(name=="K_P")
    {
      client.println(K_P);
    }
    if(name=="SET_VALUE")
    {
      client.println(SET_VALUE);
    }  
    if(name=="K_I")
    {
      client.println(K_I);
    }  
    if(name=="K_D")
    {
      client.println(K_D);
    }      
    if(name=="CYCLE")
    {
      client.println(CYCLE);
    }  
    if(name=="SUM_D_T")
    {
      client.println(SUM_D_T);
    }  
    if(name=="DELTA_PV_PV2")
    {
      client.println(DELTA_PV_PV2);
    }      
    if(name=="OUT_ZERO_VALUE")
    {
      client.println(OUT_ZERO_VALUE);
    }                  
    if(name=="OUT_25_VALUE")
    {
      client.println(OUT_25_VALUE);
    }                      
    if(name=="BUS_TEMP_UPDATE_TIME")
    {
      client.println(BUS_TEMP_UPDATE_TIME);
    }                        
  }
}

void process(YunClient client) {
  // read the command
  String command = client.readStringUntil('/');

  // is "digital" command?
  if (command == "digital") {
    digitalCommand(client);
  }

  // is "analog" command?
  if (command == "analog") {
    analogCommand(client);
  }

  // is "mode" command?
  if (command == "mode") {
    modeCommand(client);
  }
  // is "data" command?

  if (command == "data") {
    dataCommand(client);
  }

}

void loop() {
  // Get clients coming from server
  YunClient client = server.accept();

  // There is a new client?
  if (client) {
    process(client);
    client.stop();
  }
  float power=detectTemperature();
  //if(power>-1)
  pid_relay(power);
  detectbus();
  delay(10); // Poll every 50ms
}


float ReadDataFromFile(String name)
{
  //float ReturnValue;
  String Str="";
  String FileName="/root/";
  FileName+=name;
  File dataFile = FileSystem.open(FileName.c_str(), FILE_READ); 
  if (dataFile) {
    while (dataFile.available()) {
      Str+=char(dataFile.read());
    }
    dataFile.close();
  }
  return Str.toFloat();
}
