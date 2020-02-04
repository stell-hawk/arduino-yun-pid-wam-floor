//pid
float SET_VALUE=28.5; // Уставка
float SET_VALUE2; // Уставка
//float PRESENT_VALUE=0; // Текущее
float CYCLE=20; //Цикл
float K_P=10; //П
float K_I=20; //И
float K_D=20;  //Д
//float TIMER_PID; // Внутренний таймер ПИД
float E_1; // Текущее рассогласование
float E_2; // Рассогласование на -1 шаг
float D_T=0; // Приращение на текущем шагу регулирования
float SUM_D_T=50; // Накопленное воздействие %
//float SUM_D_T_S; // Накопленное воздействие с
//int PID_PULSE=CYCLE; // Шаг выполнения 1 цикла
float MILLIS_FLOAT; // Системное время мсек
//float MILLIS_FLOAT_SEK; // Системное время сек
//float MILLIS_FLOAT_1; // Метка времени сек текущего шага
bool PID_DEBUG=1;
float DELTA_PV_PV2=5;
unsigned long relay_time;
long high_level_time=0;
long low_level_time=10000;
float CONSTRAIN=100;
float SUM_E_1 = 0; // Накопленное рассогласование за интервал оценки
float SUM_E_1_ABS = 0; // Накопленный модуль рассогласования за интервал оценки
float TIMER_SUM = 0; // Накопленное рассогласование за интервал оценки
//float SET_TIMER_SUM = 20*60/CYCLE; // Накопленное рассогласование за интервал оценки
float SET_TIMER_SUM = 20*60/10; // Накопленное рассогласование за интервал оценки
float DIFF_SUM; // Накопленное отклонение к базовой уставке подачи
int OUT_ZERO_VALUE;
int OUT_25_VALUE;


//ход штока 2.3 мм.

//первый вариант пид регулятора -не используется
float pid(float PRESENT_VALUE)
{
File dataFile = FileSystem.open("/tmp/pid.txt", FILE_APPEND);  
float POWER; //Текущая мощность ШИМ для информации

  
// Программа
MILLIS_FLOAT = millis();
//MILLIS_FLOAT_SEK = MILLIS_FLOAT / 1000.0; // Перевод системного времени в сек
if(PID_DEBUG)dataFile.println("");
if(PID_DEBUG){dataFile.print("MILLIS_FLOAT: ");dataFile.print(MILLIS_FLOAT);}
if(PID_DEBUG)dataFile.print(String(String(" SET_VALUE:")+SET_VALUE+" K_P:"+K_P+" K_I:"+K_I+" K_D:"+K_D));

E_1 =  SET_VALUE -  PRESENT_VALUE; // Текущее рассогласование измеряемых единиц
if(PID_DEBUG){dataFile.print(" PRESENT_VALUE: ");dataFile.print(PRESENT_VALUE);}
if(PID_DEBUG){dataFile.print(" E_1: ");dataFile.print(E_1);}

if  (K_I == 0.0) // Деление на 0, выключение И
    {
    K_I = 9999.0;
    }

// Расчет управляющего воздействия

D_T = K_P * (E_1 - E_2 + CYCLE * E_2 / K_I ); // Приращение на текущем шагу регулирования %    
if(PID_DEBUG){dataFile.print(" E_2: ");dataFile.print(E_2);}
E_2 = E_1; // Запись рассогласования -1 шаг назад
SUM_D_T = SUM_D_T + D_T; // Накопленный выход регулятора в автоматическом режиме в %
if(PID_DEBUG){dataFile.print(" D_T: ");dataFile.print(D_T);}
SUM_D_T = constrain( SUM_D_T, (0.0), (CONSTRAIN) ); // Ограничение выхода регулятора в автоматическом и ручном режиме в %
if(PID_DEBUG){dataFile.print(" SUM_D_T: ");dataFile.print(SUM_D_T);}

// Управление
//SUM_D_T_S =  SUM_D_T * CYCLE / 100.0; // Время воздействия на текущем шагу регулирования относительно цикла регулирования сек
// Текущая мощность в %
//POWER = SUM_D_T_S;
//if(PID_DEBUG){dataFile.print("SUM_D_T_S: ");dataFile.println(SUM_D_T_S);}
dataFile.close();
return SUM_D_T;
}

//функция отвечает за установку конкретного значения  на выходе арудино
void pid_relay(float power)
{
  File dataFile3 = FileSystem.open("/tmp/pin.txt", FILE_APPEND);  
  //если пришло новое значение power
  //dataFile3.println(String(String("--- ") + millis() + "power:"+ power));
  if(power>-1)
  {
    high_level_time= power*1000/100*CYCLE;
    low_level_time=(CYCLE*1000)-high_level_time;
    dataFile3.println(String(String("-- ") + millis() + " power:"+power+" high_level_time:"+high_level_time+" low_level_time:"+low_level_time));
    
    //Выставляем высокий уровень
    //если импульс слишком короткий, то делаем через задержку
    if(high_level_time<200)
    {
       if(high_level_time<50)
       {
         digitalWrite(RELAY_PIN, LOW_LEVEL);
         return;
       }
      dataFile3.println(String(String("--- ") + millis() + " Relay UP M1"));
      digitalWrite(RELAY_PIN, HIGH_LEVEL);
      relay_time=millis();
      delay(high_level_time);
      dataFile3.println(String(String("--- ") + millis()+" "+(millis()-relay_time) + " Relay DOWN M1"));
      digitalWrite(RELAY_PIN, LOW_LEVEL);
      return;
    }
    
      digitalWrite(RELAY_PIN, HIGH_LEVEL);
      relay_time=millis();
      dataFile3.println(String(String("--- ") + relay_time + " Relay UP"));
      
  }
  else
  {
      bool PinState=digitalRead(RELAY_PIN);
        //dataFile3.println(String(String(" --- ") + millis()+" "+(millis()-relay_time) + " Relay "+PinState));
      //dataFile3.println(String(String("--- power:") + power+" Pin_state:"+ PinState));
      if(PinState!=LOW_LEVEL&&(millis()-relay_time)>high_level_time)
      {
        dataFile3.println(String(String(" --- ") + millis()+" "+(millis()-relay_time) + " Relay DOWN"));
        digitalWrite(RELAY_PIN, LOW_LEVEL);
      }
  }
  
  dataFile3.close();
}

//Вычисляем нужное значение уставки по погоде
float setvalue(float out)
{
  if(out>10)return 25;
  if(out>5)return 26;
  if(out>0)return 27;
  float value=OUT_ZERO_VALUE-out*(OUT_25_VALUE-OUT_ZERO_VALUE)/25;
  value=(float)round(value*5); 
  value=value/5;
  return value;
};



// второй вариант регулятора - не используется
float pid2(float PRESENT_VALUE,float PRESENT_VALUE2)
{
  float power; //Текущая мощность ШИМ для информации
  File dataFile = FileSystem.open("/tmp/pid.txt", FILE_APPEND);  
  if (PRESENT_VALUE2==0.0){dataFile.println("skip");return;}
  float DELTA=DELTA_PV_PV2+K_P*(-PRESENT_VALUE+SET_VALUE);
  SET_VALUE2=SET_VALUE+DELTA;
  //if(PID_DEBUG)dataFile.print(String(millis() + String(" SET_VALUE:")+SET_VALUE+" PRESENT_VALUE:"+PRESENT_VALUE+" DELTA_PV_PV2:"+DELTA_PV_PV2+" DELTA:"+DELTA+" PRESENT_VALUE2:"+PRESENT_VALUE2+" pin state " ));
  E_1 =  SET_VALUE -  PRESENT_VALUE; // Текущее рассогласование измеряемых единиц
  float D_T_2;
  if(D_T!=0)D_T_2=D_T;
  D_T = E_1 - E_2; // Приращение на текущем шагу регулирования %    
  E_2 = E_1; // Запись рассогласования -1 шаг назад

  //Анализатор профиля нагрузки
  float E_1_ABS = abs(E_1); // Модуль текущего рассогласования
  SUM_E_1 = SUM_E_1 + E_1; // Накопленное рассогласование за интервал оценки
  SUM_E_1_ABS = SUM_E_1_ABS + E_1_ABS; // Накопленный модуль рассогласования за интервал оценки
  float DIV_SUM; // Отношение накопленных рассогласований
  TIMER_SUM = TIMER_SUM + 1; // Таймер циклов
  if(TIMER_SUM >= SET_TIMER_SUM) //Время анализа профиля истекло
  {
    if(SUM_E_1_ABS==0.0) // Расчет отношения накопленного рассогласования 
    {
    DIV_SUM = 0.0; // Если деление на 0
    }
    else
    {
    DIV_SUM = SUM_E_1 / SUM_E_1_ABS; // Если всё ок
    }
  DIFF_SUM = DIFF_SUM + DIV_SUM; // Расчет накопленного отклонения к базовой уставке подачи
  SUM_E_1 = 0.0; //Сбросить накопленное рассогласование
  SUM_E_1_ABS = 0.0; //Сбросить накопленное рассогласование
  TIMER_SUM = 0.0; //Сбросить таймер
  } 
 
  
  if(PRESENT_VALUE<SET_VALUE && PRESENT_VALUE2<SET_VALUE2)
  {
  power=max_power;
  }
  else
  {
  power=min_power;
  }
    if(PRESENT_VALUE>SET_VALUE && E_1>-0.5 && (D_T>0 || (D_T==0 && D_T_2>0)))
      {
        power=max_power;
    }  
  if(PID_DEBUG)dataFile.println(String(millis() + String(" SET_VALUE:")+SET_VALUE+" PRESENT_VALUE:"+PRESENT_VALUE+" DIFF_SUM:"+DIFF_SUM+" DELTA_PV_PV2:"+DELTA_PV_PV2+" DELTA:"+DELTA +" SET_VALUE2:"+SET_VALUE2+" D_T:"+D_T+" D_T2:"+D_T_2+" PRESENT_VALUE2:"+PRESENT_VALUE2+" power: " + power ));
  return power;
}

// использующийся регулятор
float pid3(float PRESENT_VALUE,float PRESENT_VALUE2)
{
  float power; //Текущая мощность ШИМ для информации
  File dataFile = FileSystem.open("/tmp/pid.txt", FILE_APPEND);  
  if (PRESENT_VALUE2==0.0){dataFile.println("skip");return;}
  
  //ПИ-регулятор уставки подачи
  E_1 = SET_VALUE -  PRESENT_VALUE; // Текущее рассогласование
  K_I = constrain(K_I, 1.0, 10000.0); // Ограничение K_I 1...10000.0 сек на всякий случай
  D_T = K_P*(E_1 - E_2 + 10.0*E_2/K_I); // Приращение на текущем шагу регулирования в цикле 10 секунд
  E_2 = E_1; // Запись рассогласования -1 шаг назад 
  SET_VALUE2 = SET_VALUE2 + D_T;
  SET_VALUE2 = constrain(SET_VALUE2, SET_VALUE, 50.0); //Ограничение уставки подачи от SET_VALUE до 50.0 градусов  

  if(PRESENT_VALUE2<SET_VALUE2)
  {
  power=max_power;
  }
  else
  {
  power=min_power;
  } 
  if(PID_DEBUG)dataFile.println(String(millis() + String(" SET_VALUE:")+SET_VALUE+" PRESENT_VALUE:"+PRESENT_VALUE+" PRESENT_VALUE2:"+PRESENT_VALUE2 +" SET_VALUE2:"+SET_VALUE2+" D_T:"+D_T+" K_P:"+K_P+" K_I:"+K_I+" E_1:"+E_1+" E_2:"+E_2+" power: " + power ));
  return power;
}
