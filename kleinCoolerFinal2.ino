/* Klein Cooler
 * 
 * A little Fan who run if User is in Front and stop if he left his place.
 * I add some little extras like Clock, alarm and timer functions to make it more usefull.
 * 
 * Version: 1.0
 * @Author Linuxfabi
 * @Email fabienne@cherrymedia.ch
 * 
 * Special Thank to Sandra Weidmann for the melodys
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include "RTClib.h"
#include "pitches.h"

/**************************
 * Define needed Variables and Objects
 */
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// RealTimeClock Module
RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};

// init DHT Sensor
DHT dht(10, DHT11);

// init buttonstates
unsigned int buttonUp    = 0;
unsigned int buttonRight = 0;
unsigned int buttonDown  = 0;
unsigned int buttonLeft  = 0;
unsigned int buttonOk    = 0;
unsigned int buttonBack  = 0;

// fancontrol
boolean      motion     = false;
boolean      fanState   = false;
unsigned int fanLevel   = 0;
unsigned int fanTimeOut = 0;
unsigned int fanMode    = 1; // 1: buttons, 2: temperature

// Page system
unsigned int page    = 0; // 0: default clock
                          // 1: menu        2: alarm
                          // 3: timer       4: counter
                          // 5: settings    6: fanmode
                          // 7: noisesignal 8: Temp mode
                          // 9: set time
unsigned int cursorP = 0; // 1: line1 left, 2: line2 left, 3:line1 right, 4: line2 right
unsigned int blinkP  = 0; // secound position info
unsigned int prePage = 0; // for page with different originpages

// Counter
int count = 0;

// DHT
unsigned int tempMode = 1; // 1: Celsius, 2: Farenheit
int humidity          = 0;
int temperature       = 0;

// clock
boolean      blinking       = false;
unsigned int setClockHour   = 0;
unsigned int setClockMinute = 0;
unsigned int setClockSecond = 0;
unsigned int setClockYear   = 2020;
unsigned int setClockMonth  = 1;
unsigned int setClockDay    = 1;
unsigned int hour           = 0;
unsigned int minute         = 0;
unsigned int second         = 0;
unsigned int year           = 0;
unsigned int month          = 0;
unsigned int day            = 0;
String       weekday        = "";

// message
boolean      noise     = false;
unsigned int noiseMode = 2;
String       message   = "";

// alarm
boolean      alarm          = false;
boolean      setAlarm       = false;
unsigned int setAlarmHour   = 0;
unsigned int setAlarmMinute = 0;
unsigned int alarmHour      = 0;
unsigned int alarmMinute    = 0;

// timer
boolean      timer          = false;
boolean      setTimer       = false;
unsigned int setTimerHour   = 0;
unsigned int setTimerMinute = 0;
unsigned int timerHour      = 0;
unsigned int timerMinute    = 0;


/***************************************
 ****************  SETUP  **************
 ***************************************/
void setup() {
  // initialize display
  lcd.init();
  lcd.backlight();

  // init dht
  dht.begin();

  // init RTC Module 
  rtc.begin();
  // set date and time from from compiletime
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); 
  
  // Buttons depends on used buttonboard
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);
  
  pinMode(9, INPUT);        // PIR
  pinMode(10, INPUT);       // DHT Sensor
  pinMode(11, OUTPUT);      // Piezo
  pinMode(12, OUTPUT);      // Fan
  
  // A4/A5 I2C RTC Module
  // A4/A5 I2C Crystall display

  // Welcome message (deactivated in development)
  showWelcome();
}



/***************************************
 *************  MAIN LOOP  *************
 ***************************************/
void loop() {

  /*
  // stickboard
  buttonUp    = digitalRead(2);
  buttonRight = digitalRead(3);
  buttonDown  = digitalRead(4);
  buttonLeft  = digitalRead(5);
  buttonOk    = digitalRead(6);
  buttonBack  = digitalRead(7);
  */
  /*
  // Cableboard
  buttonBack  = digitalRead(2);
  buttonRight = digitalRead(3);
  buttonUp    = digitalRead(4);
  buttonOk    = digitalRead(5);
  buttonDown  = digitalRead(6);
  buttonLeft  = digitalRead(7);
  */
  // Caseboard
  buttonLeft  = digitalRead(3);
  buttonDown  = digitalRead(4);
  buttonOk    = digitalRead(5);
  buttonUp    = digitalRead(6);
  buttonRight = digitalRead(7);
  buttonBack  = digitalRead(8);
  
  // Use this to check new cabled boards if every button right linked
  //buttonTest(buttonUp, buttonRight, buttonDown, buttonLeft, buttonOk, buttonBack);
  
  DateTime now = rtc.now();
  //if (now.hour() > 0 && now.minute() > 0 && page != 9) {
  if (now.year() > 2020 && page != 9) {
    hour    = now.hour();
    minute  = now.minute();
    second  = now.second();
    year    = now.year();
    month   = now.month();
    day     = now.day();
    weekday = daysOfTheWeek[now.dayOfTheWeek()];
  }
  
  // Alarm output
  if (noise) {
    show("", message);
    if (!buttonOk || !buttonBack) {
      noise = false; // stop alarm
      buttonOk   = 0;
      buttonBack = 0;
    } else {
      switch(noiseMode) {
        case 1:  melody1();  break;
        case 2:  melody2();  break;
        case 3:  melody3();  break;
        default: makeNoise();
      }
    }
    delay(500);
  }
  if (alarm) {
    if (alarmHour == hour && alarmMinute == minute) {
      noise   = true;
      alarm   = false;
      page    = 0;
      message = "Weckzeit erreicht";
    }
  }
  if (timer) {
    if (timerHour == hour && timerMinute == minute) {
      noise   = true;
      timer   = false;
      page    = 0;
      message = "Timer Abgelaufen";
    }
  }
  
/**
 * Default mode
 * Page0 show clock, date and temperature
 * 
 */
  if (page == 0) {
    humidity    = (int)dht.readHumidity();
    temperature = (int)dht.readTemperature();
    
    showDefaultDisplay(hour, minute, second, year, month, day, temperature, tempMode, humidity, weekday, fanLevel, alarm, timer);
    if (!buttonOk) page = 1;
    
/**
* Menu
* Page 1 give access to alarm, timer, counter and settings
*/
  } else if (page == 1) {
    if (cursorP == 0) cursorP = 1;

    // select marked position
    if (!buttonOk) {
      page    = cursorP+1;
      cursorP = 1;
      delay(300);
    } else {
      cursorP = moveCursor(cursorP, buttonRight, buttonLeft, buttonUp, buttonDown);
      showMenu(cursorP);
    }
    if (!buttonBack) page = 0;


/**
 * Alarm (set alarmtime)
 */
  } else if (page == 2) {
    if (!buttonOk) {
      if (cursorP == 2 && !setAlarm) {
        setAlarm = true; // open timepicker
      } else if (cursorP == 1 || (cursorP == 2 && setAlarm)) { // ON or blinking time save alarmtime and activate alarm
        alarm       = true;
        alarmHour   = setAlarmHour;
        alarmMinute = setAlarmMinute;
        page        = 0;
        cursorP     = 1;
        showOk();
      } else if (cursorP == 2 && !setAlarm) {
        setAlarm = true;
      } else if (cursorP == 3) {
        alarm = false;
        page = 1;
        showOk();
      } else if (cursorP == 4) { // send user to noisesignal (not saved alarm time remains in memory)
        prePage = 2;
        page    = 7;
      }
    }
    
    // timepicker
    if (setAlarm) {
      if (blinkP == 1) {
        if (!buttonUp)   setAlarmHour++;
        if (!buttonDown) setAlarmHour--;
        setAlarmHour = checkHour(setAlarmHour);
        if (!buttonRight) blinkP = 2;
      } else if (blinkP == 2) {
        if (!buttonUp)   setAlarmMinute++;
        if (!buttonDown) setAlarmMinute--;
        setAlarmMinute = checkMinSec(setAlarmMinute);
        if (!buttonLeft) blinkP = 1;
      }
      if (blinkP == 0) blinkP = 1;
      if (blinking)    blinking = false;
      else             blinking = true;
      
      showSetAlarmMenu(blinkP, blinking, setAlarmHour, setAlarmMinute);
      
    } else {

      // alarm menu
      cursorP = moveCursor(cursorP, buttonRight, buttonLeft, buttonUp, buttonDown);
      showAlarmMenu(cursorP, setAlarmHour, setAlarmMinute);
    }
    
    if (!buttonBack) {
      if (setAlarm) setAlarm = false; // leave timepicker
      else          page     = 1;     // back to menu
    }


/**
 * Timer
 */
  } else if (page == 3) {
    if (!buttonOk) {
      if (cursorP == 2 && !setTimer) {
        setTimer = true; // open timepicker
      } else if (cursorP == 1 || (cursorP == 2 && setTimer)) { // ON or on blinking timepicker => calculate timerposition and start timer
        timer       = true;
        timerHour   = (hour+setTimerHour);
        timerMinute = (minute+setTimerMinute);
        page    = 0;
        cursorP = 1;
        showOk();
      } else if (cursorP == 3) { // OFF
        timer = false;
        page = 1;
        showOk();
      } else if (cursorP == 4) { // send user to noisesignal (not saved alarm time remains)
        prePage = 3;
        page    = 7;
      }
    }

    // timepicker
    if (setTimer) {
      if (blinkP == 1) {
        if (!buttonUp)   setTimerHour++;
        if (!buttonDown) setTimerHour--;
        setTimerHour = checkHour(setTimerHour);
        if (!buttonRight) blinkP = 2;
      } else if (blinkP == 2) {
        if (!buttonUp)   setTimerMinute++;
        if (!buttonDown) setTimerMinute--;
        setTimerMinute = checkMinSec(setTimerMinute);
        if (!buttonLeft) blinkP = 1;
      }
      if (blinkP == 0) blinkP = 1;
      if (blinking)    blinking = false;
      else             blinking = true;
      // use the same display as alarm
      showSetAlarmMenu(blinkP, blinking, setTimerHour, setTimerMinute);
      
    } else {
      cursorP = moveCursor(cursorP, buttonRight, buttonLeft, buttonUp, buttonDown);
      showAlarmMenu(cursorP, setTimerHour, setTimerMinute);
    }
    
    if (!buttonBack) {
      if (setTimer) setTimer = false; // leave timepicker
      else          page     = 1;     // back to settingpage
    }


/**
 * Counter
 * TODO: Maybe need a little makeover
 */
  } else if (page == 4) {
    if (!buttonUp)    count++;
    if (!buttonDown)  count--;
    if (!buttonRight) count = count+10;
    if (!buttonLeft)  count = count-10;
    if (!buttonOk)    count = 0;
    showCounter(count);
    if (!buttonBack) page = 1;
    

/**
 * Settings
 * Here are all the options
 */
  } else if (page == 5) {
    if (cursorP == 0) cursorP = 1;
    if (!buttonOk) {
      page = cursorP+5;
      cursorP = 1;
    } else {
      cursorP = moveCursor(cursorP, buttonRight, buttonLeft, buttonUp, buttonDown);
      showSettings(cursorP);
      if (!buttonBack)   page = 1;
    }
    
/**
 * Fanmode
 * to configure how the ventilator operate
 */
  } else if (page == 6) {
    if (!buttonOk) {
      fanMode = cursorP;
      page    = 0;
      cursorP = 1;
      showOk();
    } else {
      if      (!buttonUp)   cursorP = 1;
      else if (!buttonDown) cursorP = 2;
      showFanMenu(cursorP);
      if (!buttonBack)   page = 5;
    }

/**
 * Noise signal
 * how the alarm and timer beep
 */
  } else if (page == 7) {
    if (!buttonOk) {
      noiseMode = cursorP;
      page      = prePage;
      cursorP   = 1;
      showOk();
    } else {
      cursorP = moveCursor(cursorP, buttonRight, buttonLeft, buttonUp, buttonDown);
      showNoiseSignalMenu(cursorP);
      if (!buttonBack) page = prePage;
    }

/**
 * Tempmode
 * Celsius or Fahrenheit
 */
  } else if (page == 8) {
    if (!buttonOk) {
      tempMode = cursorP;
      page     = 0;
      cursorP  = 1;
      showOk();
    } else {
      if      (!buttonUp)   cursorP = 1;
      else if (!buttonDown) cursorP = 2;
      showTempMenu(cursorP);
      if (!buttonBack) page = 5;
    }


/**
 * Set Time (send back to settingmode)
 * TODO: deprecated after new timesetsystem is complet
 * Completed ^,^
 * I tear out all the old stuff, settingmode, old buttonhandler, 
 * all the redundant and prone to error digitStringcode and integrate a new system in the pagesystem
 * more and more i get fit again XD
 */
  } else if (page == 9) {
    
    if (setClockYear == 0) {
      setClockHour   = hour;
      setClockMinute = minute;
      setClockSecond = second;
      setClockDay    = day;
      setClockMonth  = month;
      setClockYear   = year;
    }
    
    if (!buttonOk) {
      rtc.adjust(DateTime(setClockYear, setClockMonth, setClockDay, setClockHour , setClockMinute, setClockSecond));
      cursorP   = 1;
      page      = 0;
      showOk();
    } else {
       if (blinkP == 0) blinkP = 1;
      if (blinking)    blinking = false;
      else             blinking = true;
      
      if (blinkP == 1) {
        if (!buttonUp)    setClockHour++;
        if (!buttonDown)  setClockHour--;
        if (!buttonRight) blinkP = 2;
      } else if (blinkP == 2) {
        if (!buttonUp)   setClockMinute++;
        if (!buttonDown) setClockMinute--;
        if (!buttonLeft)  blinkP = 1;
        if (!buttonRight) blinkP = 3;
      } else if (blinkP == 3) {
        if (!buttonUp)   setClockSecond++;
        if (!buttonDown) setClockSecond--;
        if (!buttonLeft)  blinkP = 2;
        if (!buttonRight) blinkP = 4;
      } else if (blinkP == 4) {
        if (!buttonUp)   setClockYear++;
        if (!buttonDown) setClockYear--;
        if (!buttonLeft)  blinkP = 3;
        if (!buttonRight) blinkP = 5;
      } else if (blinkP == 5) {
        if (!buttonUp)   setClockMonth++;
        if (!buttonDown) setClockMonth--;
        if (!buttonLeft)  blinkP = 4;
        if (!buttonRight) blinkP = 6;
      } else if (blinkP == 6) {
        if (!buttonUp)   setClockDay++;
        if (!buttonDown) setClockDay--;
        if (!buttonLeft) blinkP  = 5;
      }
      // prevent over or undervalues
      setClockHour   = checkHour(setClockHour);
      setClockMinute = checkMinSec(setClockMinute);
      setClockSecond = checkMinSec(setClockSecond);
      setClockMonth  = checkMonth(setClockMonth);
      setClockDay    = checkDay(setClockDay);
      
      showSetClock(blinkP, blinking, setClockHour, setClockMinute, setClockSecond, setClockYear, setClockMonth, setClockDay);
      delay(100);
    }
    
    if (!buttonBack) page = 5;
    delay(100);
  }


/**
 * FAN CONTROL
 */
  motion = digitalRead(9);
  if (motion) {
    fanState = true;
    fanTimeOut = 300;
  } else {
    fanTimeOut--;
    if (fanTimeOut <= 0) {
      fanState = false;
    }
  }
  if (page == 0) {
    if (fanMode == 1) {
      if (!buttonUp)     fanLevel = fanLevel+20;
      if (!buttonDown)   fanLevel = fanLevel-20;
      if (!buttonBack)   fanState = false;
    } else {
      if      (temperature <= 20) fanLevel = 40;
      else if (temperature <= 22) fanLevel = 50;
      else if (temperature <= 23) fanLevel = 55;
      else if (temperature <= 24) fanLevel = 60;
      else if (temperature <= 25) fanLevel = 70;
      else if (temperature <= 26) fanLevel = 80;
      else if (temperature >  26) fanLevel = 100;
    }
    if (fanLevel>=100) fanLevel = 100;
    if (fanLevel<=40)  fanLevel = 40;
  }
  
  if (fanState) {
    digitalWrite(12, HIGH);
    if (fanLevel < 100) {
      delay(fanLevel);
      digitalWrite(12, LOW);
    }
  }

  // hold back after a button is pressed to prevent repeating same action
  if (!buttonUp || !buttonRight || !buttonDown || !buttonLeft || !buttonOk || !buttonBack) delay(300);
  
} // End main loop








/**
 * ==================================================================================================
 * ==================================================================================================
 * =================================== Basic Functions ==============================================
 * ==================================================================================================
 * ==================================================================================================
 */
void show(String line1, String line2) {
  if (line1.length() > 0) {
    lcd.setCursor(0,0);
    lcd.print(line1);
  }
  if (line2.length() > 0) {
    lcd.setCursor(0,1);
    lcd.print(line2);
  }
}


String makeTwo(int digit) {
  String digitString = "";
  digitString = String(digit);
  if (digitString.length() == 1) return "0"+digitString;
  else return digitString;
}


void debug(String debugStr) {
  lcd.clear();
  show("      Debug     ", debugStr);
  delay(2000);
}


void showWelcome() {
  show("                ", "                "); delay(200);
  show("k               ", "|               "); delay(150);
  show("kl              ", "||              "); delay(150);
  show("kle             ", "|||             "); delay(200);
  show("klei            ", "||||            "); delay(200);
  show("klein           ", "|||||           "); delay(100);
  show("kleinC          ", "||||||          "); delay(600);
  show("kleinCo         ", "|||||||         "); delay(100);
  show("kleinCoo        ", "||||||||        "); delay(150);
  show("kleinCool       ", "|||||||||       "); delay(250);
  show("kleinCoole      ", "||||||||||      "); delay(100);
  show("kleinCooler     ", "|||||||||||     "); delay(100);
  show("kleinCooler     ", "||||||||||||    "); delay(400);
  show("kleinCooler 1   ", "|||||||||||||   "); delay(200);
  show("kleinCooler 1.  ", "||||||||||||||  "); delay(200);
  show("kleinCooler 1.1 ", "||||||||||||||| "); delay(200);
  show("kleinCooler 1.1 ", "||||||||||||||||"); delay(200);
  show("                ", "                "); delay(200);
  show("                ", "||||||||||||||||"); delay(200);
  show("                ", "                "); delay(200);
  show("                ", "||||||||||||||||"); delay(200);
  show("                ", "                "); delay(200);
  show("                ", "||||||||||||||||"); delay(200);
  show("                ", "                "); delay(200);
  show("                ", "By Fabienne     "); delay(300);
}


void showOk() {
  show("  Gespeichert   ", "                ");
  delay(500);
}


int checkHour(int cHour) {
  cHour = (cHour >= 24) ?  0 : cHour;
  return  (cHour < 0)   ? 23 : cHour;
}
int checkMinSec(int cSM) {
  cSM  = (cSM >= 60) ?  0 : cSM;
  return (cSM < 0)   ? 59 : cSM;
}
int checkMonth(int cMonth) {
  cMonth = (cMonth > 12) ?  1 : cMonth;
  return   (cMonth < 1)  ? 12 : cMonth;
}
int checkDay(int cDay) {
  cDay = (cDay > 31) ?  1 : cDay;
  return (cDay < 1)  ? 31 : cDay;
}


void buttonTest(int buttonUp, int buttonRight, int buttonDown, int buttonLeft, int buttonOk, int buttonBack) {
  if (!buttonUp)    lcd.setCursor(0,1); lcd.print("buttonUp");
  if (!buttonRight) lcd.setCursor(0,1); lcd.print("buttonRight");
  if (!buttonDown)  lcd.setCursor(0,1); lcd.print("buttonDown");
  if (!buttonLeft)  lcd.setCursor(0,1); lcd.print("buttonLeft");
  if (!buttonOk)    lcd.setCursor(0,0); lcd.print("buttonOk");
  if (!buttonBack)  lcd.setCursor(0,0); lcd.print("buttonBack");
  if (!(buttonUp && buttonRight && buttonDown && buttonLeft && buttonOk && buttonBack)) delay(300);
}


int moveCursor(int cursorP, int buttonRight, int buttonLeft, int buttonUp, int buttonDown) {
  if (!buttonRight) {
    if (cursorP == 1) return 2;
    if (cursorP == 3) return 4;
  } else if (!buttonLeft) {
    if (cursorP == 2) return 1;
    if (cursorP == 4) return 3;
  } else if (!buttonUp) {
    if (cursorP == 3) return 1;
    if (cursorP == 4) return 2;
  } else if (!buttonDown) {
    if (cursorP == 1) return 3;
    if (cursorP == 2) return 4;
  } 
  return cursorP; // in case of move to wall return same position
}


void showDefaultDisplay(int hour, int minute, int second, int year, int month, 
                        int day, double temperature, int tempMode, double humidity, 
                        String weekday, int fanLevel, boolean alarm, boolean timer) {
  
  lcd.setCursor(0,0);
  lcd.print(makeTwo(hour));
  lcd.print(":");
  lcd.print(makeTwo(minute));
  lcd.print(":");
  lcd.print(makeTwo(second));
  lcd.print(" ");
  if (tempMode == 2) {
    lcd.print(int((temperature*9/5)+32));
    lcd.print("F");
  } else {
    lcd.print(int(temperature));
    lcd.print("C");
  }
  lcd.print(" ");
  lcd.print(fanLevel);
  lcd.print("%");
  
  lcd.setCursor(0,1);
  lcd.print(weekday);
  lcd.print(" ");
  lcd.print(makeTwo(day));
  lcd.print(".");
  lcd.print(makeTwo(month));
  lcd.print(".");
  lcd.print(year);
  lcd.print(" ");
  if (alarm) lcd.print("A");
  else       lcd.print("-");
  if (timer) lcd.print("T");
  else       lcd.print("-");
}


void showMenu(int cursorP) {
  lcd.setCursor(0,0);
  if (cursorP == 1) lcd.print(">Wecker ");
  else              lcd.print(" Wecker ");
  lcd.setCursor(8,0);
  if (cursorP == 2) lcd.print(">Timer  ");
  else              lcd.print(" Timer  ");
  lcd.setCursor(0,1);
  if (cursorP == 3) lcd.print(">Counter");
  else              lcd.print(" Counter");
  lcd.setCursor(8,1);
  if (cursorP == 4) lcd.print(">System ");
  else              lcd.print(" System ");
}


void showSettings(int cursorP) {
  lcd.setCursor(0,0);
  if (cursorP == 1) lcd.print(">Luefter");
  else              lcd.print(" Luefter");
  lcd.setCursor(8,0);
  if (cursorP == 2) lcd.print(">Weckton");
  else              lcd.print(" Weckton");
  lcd.setCursor(0,1);
  if (cursorP == 3) lcd.print(">DHT    ");
  else              lcd.print(" DHT    ");
  lcd.setCursor(8,1);
  if (cursorP == 4) lcd.print(">Zeit   ");
  else              lcd.print(" Zeit   ");
}


void showCounter(int count) {
  lcd.setCursor(0,0);
  lcd.print("^+1     ");
  lcd.print(makeTwo(count));
  lcd.print("      ");
  lcd.setCursor(0,1);
  lcd.print("v-1 <-10    +10>");
}


void showFanMenu(int cursorP) {
  lcd.setCursor(0,0);
  if (cursorP == 1) lcd.print(">Buttons        ");
  else              lcd.print(" Buttons        ");
  lcd.setCursor(0,1);
  if (cursorP == 2) lcd.print(">Temp           ");
  else              lcd.print(" Temp           ");
}


void showNoiseSignalMenu(int cursorP) {
  lcd.setCursor(0,0);
  if (cursorP == 1) lcd.print(">bipbip ");
  else              lcd.print(" bipbip ");
  lcd.setCursor(8,0);
  if (cursorP == 2) lcd.print(">Melody1");
  else              lcd.print(" Melody1");
  lcd.setCursor(0,1);
  if (cursorP == 3) lcd.print(">Melody2");
  else              lcd.print(" Melody2");
  lcd.setCursor(8,1);
  if (cursorP == 4) lcd.print(">Melody3");
  else              lcd.print(" Melody3");
}


void showTempMenu(int cursorP) {
  lcd.setCursor(0,0);
  if (cursorP == 1) lcd.print(">Celsius        ");
  else              lcd.print(" Celsius        ");
  lcd.setCursor(0,1);
  if (cursorP == 2) lcd.print(">Farenheit      ");
  else              lcd.print(" Farenheit      ");
}


void showAlarmMenu(int cursorP, int setHour, int setMinute) {
  lcd.setCursor(0,0);
  if (cursorP == 1) lcd.print(">On     ");
  else              lcd.print(" On     ");
  if (cursorP == 2) lcd.print(">");
  else              lcd.print(" ");
  lcd.print(makeTwo(setHour));
  lcd.print(":");
  lcd.print(makeTwo(setMinute));
  lcd.print("   ");
  lcd.setCursor(0,1);
  if (cursorP == 3) lcd.print(">Off    ");
  else              lcd.print(" Off    ");
  if (cursorP == 4) lcd.print(">Signal ");
  else              lcd.print(" Signal ");
}


void showSetAlarmMenu(int blinkP, boolean blinking, int setHour, int setMinute) {
  lcd.setCursor(0,0);
  lcd.print(" On     ");
  lcd.print(">");
  if (blinkP == 1 && blinking) lcd.print("  ");
  else                         lcd.print(makeTwo(setHour));
  lcd.print(":");
  if (blinkP == 2 && blinking) lcd.print("  ");
  else                         lcd.print(makeTwo(setMinute));
  lcd.print("   ");
  lcd.setCursor(0,1);
  lcd.print(" Off    ");
  lcd.print(" Signal ");
  delay(300);
}


void showSetClock(int blinkP, boolean blinking, int sHour, int sMin, 
                  int sSec, int sYear, int sMonth, int sDay) {
  lcd.setCursor(0,0);
  if (blinkP == 1 && blinking) lcd.print("  ");
  else                         lcd.print(makeTwo(sHour));
  lcd.print(":");
  if (blinkP == 2 && blinking) lcd.print("  ");
  else                         lcd.print(makeTwo(sMin));
  lcd.print(":");
  if (blinkP == 3 && blinking) lcd.print("  ");
  else                         lcd.print(makeTwo(sSec));
  lcd.print("        ");
  lcd.setCursor(0,1);
  if (blinkP == 4 && blinking) lcd.print("    ");
  else                         lcd.print(sYear);
  lcd.print(".");
  if (blinkP == 5 && blinking) lcd.print("  ");
  else                         lcd.print(makeTwo(sMonth));
  lcd.print(".");
  if (blinkP == 6 && blinking) lcd.print("  ");
  else                         lcd.print(makeTwo(sDay));
  lcd.print("      ");
}


void makeNoise() {
  for (int i = 0; i<2; i++) {
    digitalWrite(11, HIGH);
    delay(110);
    digitalWrite(11, LOW);
    delay(80);
  }  
}


void melody1() {
  int melody[] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4};
  int noteDurations[] = {4, 8, 8, 4, 4, 4, 4, 4};
  playMelody(melody, noteDurations, 8);
}


void melody2() {
  int melody[] = {NOTE_A3, NOTE_B3, NOTE_C4, NOTE_C4, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_B3, 
                  NOTE_A3, NOTE_A3, NOTE_B3, NOTE_B3, NOTE_C4, NOTE_D4, NOTE_C4, NOTE_D4, 
                  NOTE_E4, NOTE_E4, NOTE_D4, NOTE_G3, NOTE_E4, NOTE_G4, NOTE_F4, NOTE_E4, 
                  NOTE_D4, NOTE_D4, NOTE_E4, NOTE_D4, NOTE_C4, NOTE_D4, NOTE_E4, NOTE_D4, 
                  NOTE_C4, NOTE_C4, NOTE_A3, NOTE_D4, NOTE_D4, NOTE_E4, NOTE_D4};
  int noteDurations[] = {8, 8, 2, 8, 8, 8, 8, 8, 2, 2, 8, 8, 8, 2, 8, 8, 4, 4, 8, 2, 
                         4, 4, 4, 4, 8, 8, 8, 4, 8 ,4, 4, 8, 4, 8, 4, 8, 8, 8, 4};
  playMelody(melody, noteDurations, 39);
}


void melody3() {
  int melody[] = {NOTE_E4, NOTE_E4, NOTE_G4, NOTE_A4, NOTE_E4, NOTE_E4, NOTE_G4, NOTE_A4, 
                  NOTE_E4, NOTE_E4, NOTE_G4, NOTE_A4, NOTE_E4, NOTE_E4, NOTE_G4, NOTE_A4};
  int noteDurations[] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};
  playMelody(melody, noteDurations, 16);
}


void playMelody(int melody[], int noteDurations[], int lentgh) {
  for (int i = 0; i < lentgh; i++) {
     
    int noteDuration = 1000 / noteDurations[i];
    tone(11, melody[i], noteDuration);
    
    delay(noteDuration * 1.30);
    
    noTone(11);
  }
}
