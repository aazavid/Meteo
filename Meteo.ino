/* Meteo.ino
* Обеспечивает:
*   Отображение графика температуры по 30 показаниям.
* Разработчик - Инженер- программист Завидонов Антон 2016г.
*/
#include <Adafruit_GFX.h>    // Core graphics library
#include "Andersmmg_TFTLCD.h" // Hardware-specific library
#include "Wire.h"
#include "SparkFunBME280.h"
#include "SPI.h"
#include "TouchScreen.h"

#include <TimeLib.h>
#include <DS1307RTC.h>

#include <SoftwareSerial.h>
#include <DFPlayer_Mini_Mp3.h>



//-----------------_ Макросы для экрана _-------------------

#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

//----------------_ Макросы для тачскрина _------------------

#define YP A1  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 7   // can be a digital pin
#define XP 6   // can be a digital pin

#define MINPRESSURE 10
#define MAXPRESSURE 1000
//---------------_ Макросы для карты _------------------------

#define SD_CS 10

//------------_ Макросы для датчика CO2 _------------------------

#define VOLTAGE_REGULATOR_DIGITAL_OUT_PIN 22
#define MQ7_ANALOG_IN_PIN 8

#define MQ7_HEATER_5_V_TIME_MILLIS 60000
#define MQ7_HEATER_1_4_V_TIME_MILLIS 90000

#define GAS_LEVEL_READING_PERIOD_MILLIS 1000

//---------------_ Макросы для цветов _-----------------------

#define COLOR_LCD            0x00F1
#define COLOR_LINE           0xFFE0
#define COLOR_LINE_RANGE     0xFFFF
#define COLOR_LINE_GRAF      0x888 
#define COLOR_AXIS           0xF800
#define COLOR_PIXS           0xF801
#define COLOR_LINE_FRAME     0xF75
#define COLOR_BUTTON_SELECT  0xF75
#define COLOR_BUTTON         0xF010
#define COLOR_TEXT           0xFFFF
#define COLOR_ITEM           0xFF00
//--------------_ Обявления классов _--------------------------

BME280 mySensor;
Andersmmg_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TSPoint p;

//--------------_ Обявления переменых _--------------------------

bool g = false;
int Xo = 80, Yo =150; // координаты для тачпада
double x, y;

double kX = 320/(932.0 - Xo);
double kY = 240/(883.0 - Yo);

int delay_puls = 100;   // задержка между касанием

int *xTemp;
int *yTemp;
int *gTemp;
int *yPres;
int *yHum;
int *yCO2;


unsigned long startMillis;
unsigned long switchTimeMillis;
boolean heaterInHighPhase;


int count = 0;
int main_count = 0;  // счётчик отображения показаний на рабочем столе
int cordX0 = 12;
int corddifY = 297;
int cordY0 = 12;
int corddifX = 177;

int depenition = 0;
int param_index = 0;

int point_graf = 245; //общее количество точек
int point_data = 30; // количество точек для усреднения
int heigh_graf = cordX0 + corddifX - 20, k = 3; // высота поля и коэфициент заполнения
                             // heght = h - t*k 
                             
int hours, minutes, seconds;
int days, months, years;
double temp, pres, hum, CO2;
const char *monthName[12] = { 
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

//--------------_ Переменные для меню настроек _--------------------------------
int gnum = 0;           // номер страницы меню
int alarm_hours  = 6;
int alarm_minuts = 30;
int volume_alarm = 3;
int melody_alarm = 2;
  
bool music = true;
int melody_menu = 1;
int interval_sensor = 1;

 
void setup(void) 
{
   
      
    tft.reset();
    tft.setTextSize(2);
    uint16_t identifier = tft.readID();
    tft.begin(identifier);
    tft.setRotation(3);
    
    pinMode(VOLTAGE_REGULATOR_DIGITAL_OUT_PIN, OUTPUT);
         startMillis = millis();
         turnHeaterHigh();
         
    xTemp = get_arr(point_graf);//x
    yTemp = get_m(point_data + 10);
    gTemp = get_arr(point_graf);
    yHum  = get_m(point_data + 10);
    yPres = get_m(point_data + 10);
    yCO2  = get_m(point_data + 10);
    //***Driver settings********************************//
	//commInterface can be I2C_MODE or SPI_MODE
	//specify chipSelectPin using arduino pin names
	//specify I2C address.  Can be 0x77(default) or 0x76
	
	//For I2C, enable the following and disable the SPI section
	mySensor.settings.commInterface = I2C_MODE;
	mySensor.settings.I2CAddress = 0x76;
	
	//For SPI enable the following and dissable the I2C section
	//mySensor.settings.commInterface = SPI_MODE;
	//mySensor.settings.chipSelectPin = 10;


	//***Operation settings*****************************//
	
	//renMode can be:
	//  0, Sleep mode
	//  1 or 2, Forced mode
	//  3, Normal mode
	mySensor.settings.runMode = 3; //Normal mode
	
	//tStandby can be:
	//  0, 0.5ms
	//  1, 62.5ms
	//  2, 125ms
	//  3, 250ms
	//  4, 500ms
	//  5, 1000ms
	//  6, 10ms
	//  7, 20ms
	mySensor.settings.tStandby = 0;
	
	//filter can be off or number of FIR coefficients to use:
	//  0, filter off
	//  1, coefficients = 2
	//  2, coefficients = 4
	//  3, coefficients = 8
	//  4, coefficients = 16
	mySensor.settings.filter = 0;
	
	//tempOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
	mySensor.settings.tempOverSample = 1;

	//pressOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
        mySensor.settings.pressOverSample = 1;
	
	//humidOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
	mySensor.settings.humidOverSample = 1;
	
	
	//Calling .begin() causes the settings to be loaded
	delay(10);  //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
	mySensor.begin();

        Serial2.begin(9600);
        Serial.begin(9600); 
        mp3_set_serial (Serial2);
        delay(100);	//set Serial for DFPlayer-mini mp3 module 
	mp3_set_volume (60);
        delay(100);
      
  
}
    
//----------------------------------------------------------------    
//---------------------_ ГЛАВНЫЙ ЦИКЛ _---------------------------
//----------------------------------------------------------------
void loop(void) 
{
    
    if(!g)
    {
      
        clean_touch();
        tft.fillRect(0,0, 320, 240, COLOR_LCD); 
        
        // чертим рамку
        tft.drawFastHLine(10,   10,   301,  COLOR_LINE_FRAME);
        tft.drawFastHLine(10,   190,  301,  COLOR_LINE_FRAME);
        tft.drawFastVLine(10,   10,   180,  COLOR_LINE_FRAME);
        tft.drawFastVLine(310,  10,   180,  COLOR_LINE_FRAME);
        
        tft.drawFastHLine(11,   11,   300,  COLOR_LINE_FRAME);
        tft.drawFastHLine(11,   191,  300,  COLOR_LINE_FRAME);
        tft.drawFastVLine(11,   11,   180,  COLOR_LINE_FRAME);
        tft.drawFastVLine(311,  11,   180,  COLOR_LINE_FRAME);
        
        // чертим кнопки
         draw_button(1);
         
         mp3_play(1);
         delay(100);
          
        
        // от разного цвета кнопок разное меню
        g = true;
    }
    
    //-----------------------_ Сбор Данных _------------------------
    
     temp  = mySensor.readTempC();
     pres  = mySensor.readFloatPressure()/133.332; // 750 мм.р.ст. = 99 991,5 Па
     hum   = mySensor.readFloatHumidity();
     tmElements_t tm;  
    
      if(heaterInHighPhase){
      // 5v phase of cycle. see if need to switch low yet
        if(millis() > switchTimeMillis)
        {
          turnHeaterLow();
        }
      }
      else {
      // 1.4v phase of cycle. see if need to switch high yet
        if(millis() > switchTimeMillis)
        {
          turnHeaterHigh();
        }
      }
  
      CO2 = analogRead(MQ7_ANALOG_IN_PIN);
 
    
     if (RTC.read(tm)) 
     {
       hours   = tm.Hour;
       minutes = tm.Minute;
       seconds = tm.Second;
       days    = tm.Day;
       months  = tm.Month;
       years   = tmYearToCalendar(tm.Year);
       
       if(minutes %5 ==0) // каждые 5 минут сохраняем показания
       { 
         yTemp[depenition]  = temp*k;
         yPres[depenition] = pres/8; 
         yHum[depenition]   = hum*0.5;
         yCO2[depenition] = CO2/5;
         depenition++;
       }   
      
     }
     
     
    if(depenition >= point_data+9)    // как только насобирали количество показаний датчика.
    {    
        depenition = 0;
    }
    
    //--------------------------------------------------------------
     p = ts.getPoint(); 
     
     if (p.z > MINPRESSURE && p.z < MAXPRESSURE) // произошло нажатие
     {
         x = (p.x - Xo)*kX;
         y = (p.y - Yo)*kY;
         
         if((y > 208)&&(y < 240))
         {
             if((x > 12)&&(x < 45)) // если касание на кнопке часы
             {
             
                 clean_touch();
                 draw_button(1);
                 tft.fillRect(cordX0, cordY0, corddifY, corddifX,  COLOR_LCD);
                 tft.fillRect(290,200, 30,20, COLOR_LCD);
                 param_index = 0;
                 draw_idex_param(param_index);
                 
                 mp3_play(1);
                 delay(100);
                 
             
             }
             if((x > 55)&&(x < 90)) // если касание на кнопке температура
             {
                 clean_touch();
                 
                 mp3_stop();
                 delay(100);
                
                 tft.fillRect(cordX0, cordY0, corddifY, corddifX, COLOR_LCD);
                 tft.fillRect(290,200, 30,20, COLOR_LCD);
                 param_index = 1;
                 draw_idex_param(param_index);
                 draw_button(2);
                 select_button();
                 drawFrameGrafh();
                 drawLabel(param_index);
                 my_func(xTemp, yTemp, point_data, point_graf, gTemp);
                 draw(gTemp, point_graf);
                 
                 
                 
                 
             }
             if((x > 99)&&(x < 135)) // если касание на кнопке давление
             {
             
                 clean_touch();
                 
                 mp3_stop();
                 delay(100);
                 
                 tft.fillRect(cordX0, cordY0, corddifY, corddifX,  COLOR_LCD);
                 tft.fillRect(290,200, 30,20, COLOR_LCD);
                 param_index = 2;
                 draw_idex_param(param_index);
                 draw_button(3);
                 select_button();
                 drawFrameGrafh();
                 drawLabel(param_index);
                 my_func(xTemp, yPres, point_data, point_graf, gTemp);
                 draw(gTemp, point_graf);
                 
             }
             if((x > 145)&&(x < 185)) // если касание на кнопке влажности
             {
             
                 clean_touch();
                 
                 mp3_stop();
                 delay(100);
                 
                 tft.fillRect(cordX0, cordY0, corddifY, corddifX, COLOR_LCD);
                 tft.fillRect(290,200, 30,20, COLOR_LCD);
                 param_index = 3;
                 draw_idex_param(param_index);
                 draw_button(4);
                 select_button();
                 drawFrameGrafh();
                 drawLabel(param_index);
                 my_func(xTemp, yHum, point_data, point_graf, gTemp);
                 draw(gTemp, point_graf);
                 
             }
             if((x > 193)&&(x < 228)) // если касание на кнопке СО2
             {
             
                 clean_touch();
                 
                 mp3_stop();
                 delay(100);
                 
                 tft.fillRect(cordX0, cordY0, corddifY, corddifX, COLOR_LCD);
                 tft.fillRect(290,200, 30,20, COLOR_LCD);
                 param_index = 4;
                  draw_idex_param(param_index);
                 draw_button(5);
                 drawFrameGrafh();
                 drawLabel(param_index);
                 my_func(xTemp, yCO2, point_data, point_graf, gTemp);
                 draw(gTemp, point_graf);
                  
             }
             if((x > 235)&&(x < 270)) // если касание на кнопке меню
             {
             
                 clean_touch();
                 
                 mp3_stop();
                 delay(100);
                 
                 tft.fillRect(cordX0, cordY0, corddifY, corddifX, COLOR_LCD);
                 draw_button(6);
                 param_index = 6;
                 draw_menu();
                 while(1)
                 {
                   delay(delay_puls);
                   p = ts.getPoint(); 
     
                   if (p.z > MINPRESSURE && p.z < MAXPRESSURE) // произошло нажатие
                   {
                       x = (p.x - Xo)*kX;
                       y = (p.y - Yo)*kY; 
                   
                   if(((y > 34)&&(y < 68))&&((x > 20)&&(x < 240))) // если ещё раз нажали кнопку меню
                                                                      // открываем следующую страницу
                   {
                       clean_touch();
                       
                       tft.fillRect(cordX0, cordY0, corddifY, corddifX, COLOR_LCD);
                       draw_menu();
                   }
                   if(((y > 80)&&(y < 120))&&((x > 20)&&(x < 240)))  // первая кнопка
                   {
                     while(1)
                     {
                       delay(delay_puls);
                       
                       p = ts.getPoint(); 
     
                       if (p.z > MINPRESSURE && p.z < MAXPRESSURE) // произошло нажатие
                       {
                       x = (p.x - Xo)*kX;
                       y = (p.y - Yo)*kY;
                       
                       tft.drawRoundRect(cordX0 + 5, cordY0 + 1*(49)+20, corddifY - (cordX0), 45, 6, COLOR_BUTTON);
                       tft.drawRoundRect(cordX0 + 6, cordY0 + 1*(49)+19, corddifY - (cordX0)-2, 45, 6, COLOR_BUTTON);
                       
                       if ((x > 320)&&((y > 140)&&(y < 175))) break;
                       }
                     }
                     
                   }
                   if(((y > 140)&&(y < 170))&&((x > 20)&&(x < 240)))  // вторая кнопка
                   {
                     while(1)
                     {
                       delay(delay_puls);
                       p = ts.getPoint(); 
     
                       if (p.z > MINPRESSURE && p.z < MAXPRESSURE) // произошло нажатие
                       {
                       x = (p.x - Xo)*kX;
                       y = (p.y - Yo)*kY;
                       
                       tft.drawRoundRect(cordX0 + 5, cordY0 + 2*(49)+20, corddifY - (cordX0), 45, 6, COLOR_BUTTON);
                       tft.drawRoundRect(cordX0 + 6, cordY0 + 2*(49)+19, corddifY - (cordX0)-2, 45, 6, COLOR_BUTTON);
                       
                       if ((x > 320)&&((y > 140)&&(y < 175))) break;
                       }
                     }
                   }
                   if(((y > 208)&&(y < 240))&&((x > 235)&&(x < 270)))  // третья кнопка
                   {
                     while(1)
                     {
                       delay(delay_puls);
                       p = ts.getPoint(); 
     
                       if (p.z > MINPRESSURE && p.z < MAXPRESSURE) // произошло нажатие
                       {
                       x = (p.x - Xo)*kX;
                       y = (p.y - Yo)*kY;
                       
                       tft.drawRoundRect(cordX0 + 5, cordY0 + 1*(49)+20, corddifY - (cordX0), 45, 6, COLOR_BUTTON);
                       tft.drawRoundRect(cordX0 + 6, cordY0 + 1*(49)+19, corddifY - (cordX0)-2, 45, 6, COLOR_BUTTON);
                       
                       if ((x > 320)&&((y > 140)&&(y < 175))){delay(delay_puls); break;}
                       }
                     }
                   }
                   if ((x > 320)&&((y > 140)&&(y < 175))) {break;} // выход
                   } 
                 
                 }
             }
            
         }
         
     }  // конец отработки касаний, выход в общий цикл
     
     // для отладки касания расскоментировать
     /*
     if((x>1)&&(y>1))
     {
       Serial.print(x);
       Serial.print(" - ");
       Serial.println(y);
     }                      // */
     delay(delay_puls); 
     count++;
     if(count>10){
     clean_touch();
     draw_idex_param(param_index);
     count = 0;
     }
     
  
}

//---------------------------------------------------------------------------------------------------------------------------------------
//-------------_ Обявление массива под данные для графика _-----------------

int * get_m(int p)
{
 int * g;
 g = new int[p];
 
 for(int i = 0; i < p; i++) 
 {
  
   g[i] =  ((rand()%10)+20)*k;
 }
 return g;
 
}
//----------------------------------------------------------------
int *get_arr(const int p)
{
  int * k;
  k = new int[p];
  
  for(int i = 0; i < p; i++)
  {
    k[i] =  (point_graf / p) ;
  
  }
  
  
  return k;
}

//----------------------------------------------------------------
void my_func(int * masX, int * masY, int count, int n, int* masR)
{
 int s = n / count * 2;
 
  int i;
  int q = 0;
  for(i = 0; i < n; i += s)
  {
      for(int j = 0; j < s; j++)
      {
        double f = (double)j / (double)s; 
        double t = 1 - f; 
        masR[i+j] = t * t  * (double)masY[i / s + q] + 2 * f * t * (double)masY[((i / s) + q+1)] 
                 + f * f * (double)masY[((i / s) + q+2)]; 
            
                    
      }
      
      q++;
         
  }
 
 
}
//-------------------_ Отрисовка меню настроек _-----------------
void draw_menu(void)
{
  int items = 3;     // количество строк меню
  int ofsetx = 5;
  int ofsety = 20;
  int setspace = 4;  // растояние между строками меню
  int heigh = 45;
  int ofsetext = 18;
  
  
 if(gnum ==0)
 { 
   for(int i = 0; i < items; i++)
   {
    
      tft.drawRoundRect(cordX0 + ofsetx, cordY0 + i*(heigh+setspace)+ofsety, corddifY - (cordX0), heigh, 6, COLOR_ITEM);
      tft.drawRoundRect(cordX0 + ofsetx+1, cordY0 + i*(heigh + setspace)-1+ofsety, corddifY - (cordX0)-2, heigh, 6, COLOR_ITEM);
    
   }
   tft.fillRect(290,200, 30,20, COLOR_LCD);
   tft.setCursor(290, 200);
   tft.print(gnum+1);
   gnum++;
   
   tft.setCursor(cordX0 + ofsetext, cordY0 + 36);
   tft.print("Alarm clock");
   tft.setCursor(cordX0 + 200, cordY0 + 36);
   tft.print(alarm_hours);
   tft.print(" : ");
   tft.print(alarm_minuts);
   
   tft.setCursor(cordX0 + ofsetext, cordY0 + 36 + 1*(heigh+ setspace));
   tft.print("Volume alarm");
   tft.setCursor(cordX0 + 230, cordY0 + 36+ 1*(heigh+ setspace));
   tft.print(volume_alarm);
   
   tft.setCursor(cordX0 + ofsetext, cordY0 + 36 + 2*(heigh+ setspace));
   tft.print("Melody alarm");
   tft.setCursor(cordX0 + 230, cordY0 + 36+ 2*(heigh+ setspace));
   tft.print(melody_alarm);
 }
 else
 { 
   for(int i = 0; i < items; i++)
   {
    
      tft.drawRoundRect(cordX0 + ofsetx, cordY0 + i*(heigh+setspace)+ofsety, corddifY - (cordX0), heigh, 6, COLOR_ITEM);
      tft.drawRoundRect(cordX0 + ofsetx+1, cordY0 + i*(heigh + setspace)-1+ofsety, corddifY - (cordX0)-2, heigh, 6, COLOR_ITEM);
    
   }
   tft.fillRect(290,200, 30,20, COLOR_LCD);
   tft.setCursor(290, 200);
   tft.print(gnum+1);
   gnum = 0;
   
   tft.setCursor(cordX0 + ofsetext, cordY0 + 36);
   tft.print("On/Off music");
   tft.setCursor(cordX0 + 230, cordY0 + 36);
   if(music) tft.print("On");
   else  tft.print("Off");
   
   tft.setCursor(cordX0 + ofsetext, cordY0 + 36 + 1*(heigh+ setspace));
   tft.print("Melody menu");
   tft.setCursor(cordX0 + 230, cordY0 + 36+ 1*(heigh+ setspace));
   tft.print(melody_menu);
   
   tft.setCursor(cordX0 + ofsetext, cordY0 + 36 + 2*(heigh+ setspace));
   tft.print("Interval sensor");
   tft.setCursor(cordX0 + 230, cordY0 + 36+ 2*(heigh+ setspace));
   tft.print(interval_sensor);
 }
 
}
//----------------------------------------------------------------

//------------------------_ Отрисовка осей _----------------------
void drawFrameGrafh()
{ 
  int margin = 5, margin_l = 20;
  tft.drawFastVLine(cordX0 + margin_l, (cordY0 + margin_l), (corddifX - 2 * margin_l),  COLOR_AXIS);
  tft.drawFastHLine(cordX0 + margin_l, (cordX0+ corddifX - margin_l), (corddifY - 2 * margin_l),  COLOR_AXIS);
}
//----------------------------------------------------------------
//------------_ Отрисовка подписей графика температуры _-----------------
void drawLabel(int num)
{
  tft.setTextSize(1);
  int ofSetX = 5, ofSetText = 4;
  switch(num)
  {
    case 1: // температура
    {  
       
      tft.setCursor(ofSetX+cordX0,cordY0+ofSetText);
      tft.print("T, C");
      tft.setCursor(ofSetX+cordX0, (heigh_graf - 40*k- ofSetText));  // h - высота поля
      tft.print("40");
      tft.setCursor(ofSetX+cordX0, (heigh_graf - 30*k- ofSetText));
      tft.print("30");
      tft.setCursor(ofSetX+cordX0, (heigh_graf - 20*k- ofSetText));
      tft.print("20");
      tft.setCursor(ofSetX+cordX0, (heigh_graf - 10*k- ofSetText));
      tft.print("10");
 
      tft.setCursor(ofSetX*2+cordX0+5, 175);
      tft.print("0");
    
      tft.setCursor(250,175);
      tft.print("t, hour");
      break;
    }
    case 2: // давление
    {
      tft.setCursor(ofSetX+cordX0,cordY0+ofSetText);
      tft.print("P, mm");
      tft.setCursor(1+cordX0, (heigh_graf - 40*k- ofSetText));  // h - высота поля
      tft.print("967");
      tft.setCursor(1+cordX0, (heigh_graf - 30*k- ofSetText));
      tft.print("725");
      tft.setCursor(1+cordX0, (heigh_graf - 20*k- ofSetText));
      tft.print("483");
      tft.setCursor(1+cordX0, (heigh_graf - 10*k- ofSetText));
      tft.print("241");
 
      tft.setCursor(ofSetX*2+cordX0, 175);
      tft.print("0");
    
      tft.setCursor(250,175);
      tft.print("t, hour");
      break;
    }
    case 3: // влажность
    {
      tft.setCursor(ofSetX+cordX0,cordY0+ofSetText);
      tft.print("T, C");
      
      tft.setCursor(ofSetX+cordX0, (heigh_graf - 30*k- ofSetText));
      tft.print("100");
      tft.setCursor(ofSetX+cordX0, (heigh_graf - 20*k- ofSetText));
      tft.print("80");
      tft.setCursor(ofSetX+cordX0, (heigh_graf - 10*k- ofSetText));
      tft.print("40");
 
      tft.setCursor(ofSetX*2+cordX0, 175);
      tft.print("0");
    
      tft.setCursor(250,175);
      tft.print("t, hour");
      break;
    }
    case 4: // CO2
    {
      tft.setCursor(ofSetX+cordX0,cordY0+ofSetText);
      tft.print("T, C");
      tft.setCursor(ofSetX+cordX0, (heigh_graf - 40*k- ofSetText));  // h - высота поля
      tft.print("40");
      tft.setCursor(ofSetX+cordX0, (heigh_graf - 30*k- ofSetText));
      tft.print("30");
      tft.setCursor(ofSetX+cordX0, (heigh_graf - 20*k- ofSetText));
      tft.print("20");
      tft.setCursor(ofSetX+cordX0, (heigh_graf - 10*k- ofSetText));
      tft.print("10");
 
      tft.setCursor(ofSetX*2+cordX0, 175);
      tft.print("0");
    
      tft.setCursor(250,175);
      tft.print("t, hour");
      break;
    }
    default:{break;}
  }
   tft.setTextSize(2);
}

void draw(int * f, int count)
{
  int begin = cordX0 + 22;
   
    for(int i = begin,  s = 0; s <= count; s++, i++)
    {
     
      tft.drawFastVLine(i, (heigh_graf-f[s]), (heigh_graf -(heigh_graf-f[s])), COLOR_LINE);
      tft.drawPixel(i, (heigh_graf-f[s]), COLOR_PIXS);
      tft.drawPixel(i, ((heigh_graf-f[s])-1), COLOR_PIXS);
      
      if((s%5 != 0)&&(s%5 < 4))
      {
        for(int j = heigh_graf + 1; j >= heigh_graf-f[s]; j -= 10)
        {
          tft.drawPixel(i, j,  COLOR_LINE_GRAF);
        }
      }
      delay(10);
    
    }

}
//----------------------------------------------------------------
void select_button(void)
{
    //tft.fillRoundRect(285, 192, 35, 35, 5, COLOR_LINE_FRAME);
    
}
//--------------_ отрисовка текущего показателя _-----------------
void draw_idex_param(int num)
{
  
  
  //tft.setTextColor(COLOR_LINE_FRAME);
  tft.setTextSize(2);
  
  switch(num)
  {
    case 0: // главное меню
    {
       
       if(seconds < 3)tft.fillRect(120, 65, 90, 40, COLOR_LCD);
       if((minutes == 0)&&(seconds < 4))tft.fillRect(60, 65, 90, 40, COLOR_LCD);
       if((hours == 0)&&(minutes == 0)&&(seconds < 4)) tft.fillRect(25, 85, 180, 30, COLOR_LCD);
       if((seconds %10 ==0)||(seconds %10 < 2)){tft.fillRect(200, 65, 60, 40, COLOR_LCD);}
       else{tft.fillRect(225, 65, 30, 40, COLOR_LCD);}
       
       
       // время
       tft.setCursor(60, 65);
       tft.setTextSize(4);
       tft.setTextColor(0xFF0); // цвет
       if(hours < 10)tft.print("0");
       tft.print(hours);
       tft.print(":");
       if(minutes < 10)tft.print("0");
       tft.print(minutes);
       tft.print(":");
       if(seconds < 10)tft.print("0");
       tft.print(seconds);
       
       // дата
       tft.setTextSize(2);
       tft.setCursor(90, 25);
       tft.print(days);
       tft.print("_");
       tft.print(monthName[months]);
       tft.print("_");
       tft.print(years);
       
       
       //показания датчиков
       if(main_count == 0)
       {
         tft.fillRect(60, 120, 80, 20, COLOR_LCD);
         
         tft.setCursor(30, 120);
         tft.print("T = ");
         tft.print(temp);
         tft.println(" C;");
         
         tft.fillRect(70, 150, 120, 20, COLOR_LCD);
         tft.setCursor(40, 150);
         tft.print("P = ");
         tft.print(pres);
         tft.print(" mm.rt.st");
         
         tft.fillRect(200, 120, 80, 20, COLOR_LCD);
         tft.setCursor(170, 120);
         tft.print("H = ");
         tft.print(hum);
         tft.print(" %");
       }
       main_count++;
       if(main_count == 10) main_count = 0;
       break;
    }
    case 1:
    {
       tft.fillRect(125, 35, 110, 20, COLOR_LCD);
       tft.drawRect(90, 30, 150, 25, COLOR_LINE_FRAME);
       tft.setCursor(95, 35);
       tft.print("T = ");
       tft.print(temp);
       tft.println(" C");
       main_count = 0;
       break;
    }
    case 2:
    {
       tft.fillRect(75, 35, 210, 20, COLOR_LCD);
       tft.drawRect(45, 30, 250, 25, COLOR_LINE_FRAME);
       tft.setCursor(45, 35);
       tft.print("P = ");
       tft.print(pres);
       tft.print(" mm.rt.st");
       main_count = 0;
       break;
    }
    case 3:
    {
       tft.fillRect(125, 35, 110, 20, COLOR_LCD);
       tft.drawRect(90, 30, 150, 25, COLOR_LINE_FRAME);
       tft.setCursor(95, 35);
       tft.print("H = ");
       tft.print(hum);
       tft.print(" %");
       main_count = 0;
       break;
    }
    case 4:
    {
       tft.fillRect(135, 35, 110, 20, COLOR_LCD);
       tft.drawRect(90, 30, 150, 25, COLOR_LINE_FRAME);
       tft.setCursor(115, 35);
       tft.print("CO2 = ");
       tft.print(CO2/10);
       tft.print(" %");
       main_count = 0;
       break;
    }
    
    default: break;
  }
   tft.setTextColor(0xFFFF);
   tft.setTextSize(2); 
}
//----------------------------------------------------------------
//-----------------_ отрисовка кнопки _---------------------------
//----------------------------------------------------------------
void draw_button(int num) //номер выбранной кнопки
{
  tft.fillRect(10,192, 280, 60, COLOR_LCD);
  switch (num){
      case 1:
         { 
           // выделенная кнопка
             // очищаем линию для вкладки
           tft.drawFastHLine(11,   190,  45,        COLOR_LCD);
           tft.drawFastHLine(11,   191,  45,        COLOR_LCD);
           tft.drawFastHLine(55,   190,  311-55,        COLOR_LINE_FRAME);
           tft.drawFastHLine(55,   191,  311-55,        COLOR_LINE_FRAME);
           
             // отрисовываем контур
           tft.drawFastVLine(10,   190,   39,        COLOR_LINE_FRAME);
           tft.drawFastVLine(55,   190,   39,        COLOR_LINE_FRAME);
           tft.drawFastHLine(17,   236,   31,        COLOR_LINE_FRAME);
           tft.drawLine     (10,   229,   17,   236, COLOR_LINE_FRAME);
           tft.drawLine     (48,   236,   55,   229, COLOR_LINE_FRAME);
           
           
           tft.drawFastVLine(11,   190,   39,        COLOR_LINE_FRAME);
           tft.drawFastVLine(54,   190,   39,        COLOR_LINE_FRAME);
           tft.drawFastHLine(17,   235,   31,        COLOR_LINE_FRAME);
           tft.drawLine     (11,   229,   18,   236, COLOR_LINE_FRAME);
           tft.drawLine     (47,   236,   54,   229, COLOR_LINE_FRAME);
           
             // текст кнопки
           tft.drawCircle   (33,   210,   15,        COLOR_LINE_FRAME),
           tft.drawFastVLine(33,   195,   13,        COLOR_LINE_FRAME);
           tft.drawLine     (33,   210,   40,   205, COLOR_LINE_FRAME);
           // остальные кнопки
             
             // температура
             // отрисовываем контур
             
           tft.drawFastVLine(56,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(101,  192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(56,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(56,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(57,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(100,  193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(56,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(56,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(60, 206);
           tft.print("T,C");
           
             // давление
             // отрисовываем контур
             
           tft.drawFastVLine(102,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(147,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(102,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(102,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(103,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(146,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(103,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(103,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(115, 206);
           tft.print("P");
           
             // влажность
             // отрисовываем контур
             
           tft.drawFastVLine(148,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(192,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(148,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(148,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(149,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(191,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(149,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(149,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(165, 206);
           tft.print("D");
           
             // CO2
             // отрисовываем контур
             
           tft.drawFastVLine(193,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(237,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(193,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(193,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(194,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(236,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(194,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(194,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(200, 206);
           tft.print("CO2");
           
             // меню настроек 
             // отрисовываем контур
             
           tft.drawFastVLine(238,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(282,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(238,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(238,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(239,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(281,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(239,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(239,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
            tft.drawFastHLine(246,   201,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   202,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   203,   28,        COLOR_BUTTON);
           
            tft.drawFastHLine(246,   209,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   210,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   211,   28,        COLOR_BUTTON);
             
            tft.drawFastHLine(246,   217,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   218,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   219,   28,        COLOR_BUTTON);
           
           break;
         }
         case 2:
         { 
           
             // часы
             // отрисовываем контур
           tft.drawFastVLine(10,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(55,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(10,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(10,   229,   45,        COLOR_BUTTON);
           
           
           tft.drawFastVLine(11,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(54,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(10,   193,   45,        COLOR_BUTTON);
           tft.drawFastHLine(10,   228,   45,        COLOR_BUTTON);
           
           
             // текст кнопки
           tft.drawCircle   (33,   210,   15,        COLOR_BUTTON),
           tft.drawFastVLine(33,   195,   13,        COLOR_BUTTON);
           tft.drawLine     (33,   210,   40,   205, COLOR_BUTTON);
           // остальные кнопки
             
             // температура
             // выделенная кнопка
             // очищаем линию для вкладки
           tft.drawFastHLine(56,   190,  45,        COLOR_LCD);
           tft.drawFastHLine(56,   191,  45,        COLOR_LCD);
           tft.drawFastHLine(10,   190,  47,        COLOR_LINE_FRAME);
           tft.drawFastHLine(10,   191,  47,        COLOR_LINE_FRAME);
           tft.drawFastHLine(101,  190,  311-101,        COLOR_LINE_FRAME);
           tft.drawFastHLine(101,  191,  311-101,        COLOR_LINE_FRAME);
             // отрисовываем контур
             
           tft.drawFastVLine(56,   190,   39,        COLOR_LINE_FRAME);
           tft.drawFastVLine(101,  190,   39,        COLOR_LINE_FRAME);
           tft.drawFastHLine(63,   236,   31,        COLOR_LINE_FRAME);
           tft.drawLine     (56,   229,   63,  236,  COLOR_LINE_FRAME);
           tft.drawLine     (94,   236,   101, 229,  COLOR_LINE_FRAME);
                      
           tft.drawFastVLine(57,   191,   39,        COLOR_LINE_FRAME);
           tft.drawFastVLine(100,  191,   39,        COLOR_LINE_FRAME);
           tft.drawFastHLine(64,   236,   31,        COLOR_LINE_FRAME);
           tft.drawLine     (57,   229,   64,  236,  COLOR_LINE_FRAME);
           tft.drawLine     (93,   236,   100, 229,  COLOR_LINE_FRAME);
           
            // текст кнопки
           tft.setCursor(60, 206);
           tft.print("T,C");
           
             // давление
             // отрисовываем контур
             
           tft.drawFastVLine(102,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(147,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(102,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(102,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(103,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(146,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(103,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(103,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(115, 206);
           tft.print("P");
           
             // влажность
             // отрисовываем контур
             
           tft.drawFastVLine(148,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(192,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(148,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(148,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(149,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(191,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(149,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(149,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(165, 206);
           tft.print("D");
           
             // CO2
             // отрисовываем контур
             
           tft.drawFastVLine(193,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(237,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(193,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(193,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(194,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(236,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(194,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(194,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(200, 206);
           tft.print("CO2");
           
             // меню настроек 
             // отрисовываем контур
             
           tft.drawFastVLine(238,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(282,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(238,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(238,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(239,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(281,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(239,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(239,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
            tft.drawFastHLine(246,   201,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   202,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   203,   28,        COLOR_BUTTON);
           
            tft.drawFastHLine(246,   209,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   210,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   211,   28,        COLOR_BUTTON);
             
            tft.drawFastHLine(246,   217,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   218,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   219,   28,        COLOR_BUTTON);
           
           break;
         }
         case 3:
         { 
           // часы
             // отрисовываем контур
           tft.drawFastVLine(10,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(55,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(10,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(10,   229,   45,        COLOR_BUTTON);
           
           
           tft.drawFastVLine(11,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(54,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(10,   193,   45,        COLOR_BUTTON);
           tft.drawFastHLine(10,   228,   45,        COLOR_BUTTON);
           
           
             // текст кнопки
           tft.drawCircle   (33,   210,   15,        COLOR_BUTTON),
           tft.drawFastVLine(33,   195,   13,        COLOR_BUTTON);
           tft.drawLine     (33,   210,   40,   205, COLOR_BUTTON);
           // остальные кнопки
             
             // температура
             // отрисовываем контур
             
           tft.drawFastVLine(56,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(101,  192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(56,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(56,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(57,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(100,  193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(56,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(56,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(60, 206);
           tft.print("T,C");
           
             // давление
              // выделенная кнопка
             // очищаем линию для вкладки
           tft.drawFastHLine(102,   190,  45,        COLOR_LCD);
           tft.drawFastHLine(102,   191,  45,        COLOR_LCD);
           tft.drawFastHLine(10,    190,  92,        COLOR_LINE_FRAME);
           tft.drawFastHLine(10,    191,  92,        COLOR_LINE_FRAME);
           tft.drawFastHLine(147,  190,  311-147,        COLOR_LINE_FRAME);
           tft.drawFastHLine(147,  191,  311-147,        COLOR_LINE_FRAME);
             // отрисовываем контур
             
           tft.drawFastVLine(102,   190,   39,         COLOR_LINE_FRAME);
           tft.drawFastVLine(147,   190,   39,         COLOR_LINE_FRAME);
           tft.drawFastHLine(109,   236,   31,         COLOR_LINE_FRAME);
           tft.drawLine     (102,   229,   109,  236,  COLOR_LINE_FRAME);
           tft.drawLine     (140,   236,   147,  229,  COLOR_LINE_FRAME);
                      
           tft.drawFastVLine(103,   190,   39,         COLOR_LINE_FRAME);
           tft.drawFastVLine(146,   190,   39,         COLOR_LINE_FRAME);
           tft.drawFastHLine(109,   235,   31,         COLOR_LINE_FRAME);
           tft.drawLine     (103,   229,   110,  236,  COLOR_LINE_FRAME);
           tft.drawLine     (139,   236,   146,  229,  COLOR_LINE_FRAME);
           
            // текст кнопки
           tft.setCursor(115, 206);
           tft.print("P");
           
             // влажность
             // отрисовываем контур
             
           tft.drawFastVLine(148,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(192,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(148,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(148,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(149,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(191,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(149,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(149,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(165, 206);
           tft.print("D");
           
             // CO2
             // отрисовываем контур
             
           tft.drawFastVLine(193,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(237,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(193,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(193,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(194,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(236,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(194,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(194,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(200, 206);
           tft.print("CO2");
           
             // меню настроек 
             // отрисовываем контур
             
           tft.drawFastVLine(238,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(282,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(238,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(238,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(239,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(281,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(239,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(239,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
            tft.drawFastHLine(246,   201,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   202,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   203,   28,        COLOR_BUTTON);
           
            tft.drawFastHLine(246,   209,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   210,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   211,   28,        COLOR_BUTTON);
             
            tft.drawFastHLine(246,   217,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   218,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   219,   28,        COLOR_BUTTON);
           
           break;
         }
         case 4:
         { 
           // часы
             // отрисовываем контур
           tft.drawFastVLine(10,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(55,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(10,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(10,   229,   45,        COLOR_BUTTON);
           
           
           tft.drawFastVLine(11,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(54,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(10,   193,   45,        COLOR_BUTTON);
           tft.drawFastHLine(10,   228,   45,        COLOR_BUTTON);
           
           
             // текст кнопки
           tft.drawCircle   (33,   210,   15,        COLOR_BUTTON),
           tft.drawFastVLine(33,   195,   13,        COLOR_BUTTON);
           tft.drawLine     (33,   210,   40,   205, COLOR_BUTTON);
           // остальные кнопки
             
             // температура
             // отрисовываем контур
             
           tft.drawFastVLine(56,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(101,  192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(56,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(56,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(57,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(100,  193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(56,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(56,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(60, 206);
           tft.print("T,C");
           
             // давление
             // отрисовываем контур
             
           tft.drawFastVLine(102,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(147,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(102,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(102,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(103,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(146,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(103,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(103,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(115, 206);
           tft.print("P");
           
             // влажность
                // выделенная кнопка
             // очищаем линию для вкладки
           tft.drawFastHLine(148,   190,  45,        COLOR_LCD);
           tft.drawFastHLine(148,   191,  45,        COLOR_LCD);
           tft.drawFastHLine(10,    190,  137,        COLOR_LINE_FRAME);
           tft.drawFastHLine(10,    191,  137,        COLOR_LINE_FRAME);
           tft.drawFastHLine(192,  190,  311-192,        COLOR_LINE_FRAME);
           tft.drawFastHLine(192,  191,  311-192,        COLOR_LINE_FRAME);
             // отрисовываем контур
             
           tft.drawFastVLine(148,   190,   39,         COLOR_LINE_FRAME);
           tft.drawFastVLine(192,   190,   39,         COLOR_LINE_FRAME);
           tft.drawFastHLine(155,   236,   31,         COLOR_LINE_FRAME);
           tft.drawLine     (148,   229,   155,  236,  COLOR_LINE_FRAME);
           tft.drawLine     (186,   236,   193,  229,  COLOR_LINE_FRAME);
                      
           tft.drawFastVLine(149,   190,   39,         COLOR_LINE_FRAME);
           tft.drawFastVLine(191,   190,   39,         COLOR_LINE_FRAME);
           tft.drawFastHLine(155,   235,   31,         COLOR_LINE_FRAME);
           tft.drawLine     (149,   229,   156,  236,  COLOR_LINE_FRAME);
           tft.drawLine     (185,   236,   192,  229,  COLOR_LINE_FRAME);
                      
            // текст кнопки
           tft.setCursor(165, 206);
           tft.print("D");
           
             // CO2
             // отрисовываем контур
             
           tft.drawFastVLine(193,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(237,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(193,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(193,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(194,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(236,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(194,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(194,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(200, 206);
           tft.print("CO2");
           
             // меню настроек 
             // отрисовываем контур
             
           tft.drawFastVLine(238,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(282,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(238,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(238,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(239,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(281,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(239,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(239,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
            tft.drawFastHLine(246,   201,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   202,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   203,   28,        COLOR_BUTTON);
           
            tft.drawFastHLine(246,   209,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   210,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   211,   28,        COLOR_BUTTON);
             
            tft.drawFastHLine(246,   217,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   218,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   219,   28,        COLOR_BUTTON);
           
           break;
         }
         case 5:
         { 
             // часы
             // отрисовываем контур
           tft.drawFastVLine(10,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(55,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(10,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(10,   229,   45,        COLOR_BUTTON);
           
           
           tft.drawFastVLine(11,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(54,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(10,   193,   45,        COLOR_BUTTON);
           tft.drawFastHLine(10,   228,   45,        COLOR_BUTTON);
           
             // текст кнопки
           tft.drawCircle   (33,   210,   15,        COLOR_BUTTON),
           tft.drawFastVLine(33,   195,   13,        COLOR_BUTTON);
           tft.drawLine     (33,   210,   40,   205, COLOR_BUTTON);
           // остальные кнопки
             
             // температура
             // отрисовываем контур
             
           tft.drawFastVLine(56,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(101,  192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(56,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(56,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(57,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(100,  193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(56,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(56,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(60, 206);
           tft.print("T,C");
           
             // давление
             // отрисовываем контур
             
           tft.drawFastVLine(102,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(147,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(102,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(102,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(103,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(146,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(103,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(103,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(115, 206);
           tft.print("P");
           
             // влажность
             // отрисовываем контур
             
           tft.drawFastVLine(148,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(192,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(148,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(148,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(149,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(191,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(149,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(149,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(165, 206);
           tft.print("D");
           
             // CO2
               // выделенная кнопка
             // очищаем линию для вкладки
           tft.drawFastHLine(193,   190,  45,        COLOR_LCD);
           tft.drawFastHLine(193,   191,  45,        COLOR_LCD);
           tft.drawFastHLine(10,    190,  182,        COLOR_LINE_FRAME);
           tft.drawFastHLine(10,    191,  182,        COLOR_LINE_FRAME);
           tft.drawFastHLine(236,  190,  311-236,        COLOR_LINE_FRAME);
           tft.drawFastHLine(236,  191,  311-236,        COLOR_LINE_FRAME);
             // отрисовываем контур
             
           tft.drawFastVLine(193,   190,   39,         COLOR_LINE_FRAME);
           tft.drawFastVLine(237,   190,   39,         COLOR_LINE_FRAME);
           tft.drawFastHLine(200,   236,   31,         COLOR_LINE_FRAME);
           tft.drawLine     (193,   229,   200,  236,  COLOR_LINE_FRAME);
           tft.drawLine     (231,   236,   238,  229,  COLOR_LINE_FRAME);
                      
           tft.drawFastVLine(194,   190,   39,         COLOR_LINE_FRAME);
           tft.drawFastVLine(236,   190,   39,         COLOR_LINE_FRAME);
           tft.drawFastHLine(200,   235,   31,         COLOR_LINE_FRAME);
           tft.drawLine     (194,   229,   201,  236,  COLOR_LINE_FRAME);
           tft.drawLine     (230,   236,   237,  229,  COLOR_LINE_FRAME);
                      
            // текст кнопки
           tft.setCursor(200, 206);
           tft.print("CO2");
           
             // меню настроек 
             // отрисовываем контур
             
           tft.drawFastVLine(238,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(282,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(238,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(238,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(239,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(281,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(239,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(239,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
            tft.drawFastHLine(246,   201,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   202,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   203,   28,        COLOR_BUTTON);
           
            tft.drawFastHLine(246,   209,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   210,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   211,   28,        COLOR_BUTTON);
             
            tft.drawFastHLine(246,   217,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   218,   28,        COLOR_BUTTON);
            tft.drawFastHLine(246,   219,   28,        COLOR_BUTTON);
           
           break;
         }
         case 6:
         { 
             // часы
             // отрисовываем контур
           tft.drawFastVLine(10,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(55,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(10,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(10,   229,   45,        COLOR_BUTTON);
           
           
           tft.drawFastVLine(11,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(54,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(10,   193,   45,        COLOR_BUTTON);
           tft.drawFastHLine(10,   228,   45,        COLOR_BUTTON);
           
             // текст кнопки
           tft.drawCircle   (33,   210,   15,        COLOR_BUTTON),
           tft.drawFastVLine(33,   195,   13,        COLOR_BUTTON);
           tft.drawLine     (33,   210,   40,   205, COLOR_BUTTON);
           // остальные кнопки
             
             // температура
             // отрисовываем контур
             
           tft.drawFastVLine(56,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(101,  192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(56,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(56,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(57,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(100,  193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(56,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(56,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(60, 206);
           tft.print("T,C");
           
             // давление
             // отрисовываем контур
             
           tft.drawFastVLine(102,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(147,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(102,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(102,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(103,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(146,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(103,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(103,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(115, 206);
           tft.print("P");
           
             // влажность
             // отрисовываем контур
             
           tft.drawFastVLine(148,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(192,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(148,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(148,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(149,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(191,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(149,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(149,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(165, 206);
           tft.print("D");
           
             // CO2
             // отрисовываем контур
             
           tft.drawFastVLine(193,   192,   36,        COLOR_BUTTON);
           tft.drawFastVLine(237,   192,   36,        COLOR_BUTTON);
           tft.drawFastHLine(193,   192,   45,        COLOR_BUTTON);
           tft.drawFastHLine(193,   229,   45,        COLOR_BUTTON);
                      
           tft.drawFastVLine(194,   193,   36,        COLOR_BUTTON);
           tft.drawFastVLine(236,   193,   36,        COLOR_BUTTON);
           tft.drawFastHLine(194,   193,   44,        COLOR_BUTTON);
           tft.drawFastHLine(194,   228,   44,        COLOR_BUTTON);
           
            // текст кнопки
           tft.setCursor(200, 206);
           tft.print("CO2");
           
             // меню настроек 
                // выделенная кнопка
             // очищаем линию для вкладки
           tft.drawFastHLine(238,   190,  45,        COLOR_LCD);
           tft.drawFastHLine(238,   191,  45,        COLOR_LCD);
           tft.drawFastHLine(10,    190,  227,        COLOR_LINE_FRAME);
           tft.drawFastHLine(10,    191,  227,        COLOR_LINE_FRAME);
           tft.drawFastHLine(282,  190,  311-282,        COLOR_LINE_FRAME);
           tft.drawFastHLine(282,  191,  311-282,        COLOR_LINE_FRAME);
             // отрисовываем контур
             
           tft.drawFastVLine(238,   190,   39,         COLOR_LINE_FRAME);
           tft.drawFastVLine(282,   190,   39,         COLOR_LINE_FRAME);
           tft.drawFastHLine(245,   236,   31,         COLOR_LINE_FRAME);
           tft.drawLine     (238,   229,   245,  236,  COLOR_LINE_FRAME);
           tft.drawLine     (275,   236,   282,  229,  COLOR_LINE_FRAME);
                      
           tft.drawFastVLine(239,   190,   39,         COLOR_LINE_FRAME);
           tft.drawFastVLine(281,   190,   39,         COLOR_LINE_FRAME);
           tft.drawFastHLine(245,   235,   31,         COLOR_LINE_FRAME);
           tft.drawLine     (239,   229,   246,  236,  COLOR_LINE_FRAME);
           tft.drawLine     (275,   235,   281,  229,  COLOR_LINE_FRAME);
           
                            
                      
            // текст кнопки
            tft.drawFastHLine(246,   201,   28,        COLOR_LINE_FRAME);
            tft.drawFastHLine(246,   202,   28,        COLOR_LINE_FRAME);
            tft.drawFastHLine(246,   203,   28,        COLOR_LINE_FRAME);
           
            tft.drawFastHLine(246,   209,   28,        COLOR_LINE_FRAME);
            tft.drawFastHLine(246,   210,   28,        COLOR_LINE_FRAME);
            tft.drawFastHLine(246,   211,   28,        COLOR_LINE_FRAME);
             
            tft.drawFastHLine(246,   217,   28,        COLOR_LINE_FRAME);
            tft.drawFastHLine(246,   218,   28,        COLOR_LINE_FRAME);
            tft.drawFastHLine(246,   219,   28,        COLOR_LINE_FRAME);
           
           break;
         }
         default: break;
  }
}
//-----------------_ очистка регистров после касания _------------
void clean_touch(void)
{
pinMode(XM, OUTPUT);
digitalWrite(XM, LOW);
pinMode(YP, OUTPUT);
digitalWrite(YP, HIGH);
pinMode(YM, OUTPUT);
digitalWrite(YM, LOW);
pinMode(XP, OUTPUT);
digitalWrite(XP, HIGH);
}

//------------------------------------------------------------------
void turnHeaterHigh(){
  // 5v phase
  digitalWrite(VOLTAGE_REGULATOR_DIGITAL_OUT_PIN, LOW);
  heaterInHighPhase = true;
  switchTimeMillis = millis() + MQ7_HEATER_5_V_TIME_MILLIS;
}

void turnHeaterLow(){
  // 1.4v phase
  digitalWrite(VOLTAGE_REGULATOR_DIGITAL_OUT_PIN, HIGH);
  heaterInHighPhase = false;
  switchTimeMillis = millis() + MQ7_HEATER_1_4_V_TIME_MILLIS;
}

