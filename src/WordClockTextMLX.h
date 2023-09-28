/*
 Die Texte werden buschtabenweise angezeigt. Nach jedem Wort (Leerzeichen) kommt eine Pause.
 Dabei werden die passenden Buchstaben von links oben nach rechts unten gesucht.
 Wenn ein Buchstabe mehrfach in einem Wort vorkommt, aber nicht so oft vorhanden ist, dann wird der
 entsprechende Buchstabe in einer anderen Farbe dargestellt.

 Wenn mehrere Worte ohne Pause angezeigt werden sollen, dann müssen diese mit einem Unterstrich anstelle eines
 Leerzeichens getrennt werden.

 Wenn ein Buchstabe überspringen werden soll, dann wird ihm ein '.' vorangestellt. Dadurch kann z.B.
 erreicht werden dass das zusammenhängende "LED" bei MOBA_...LED_LIB verwendet wird und nicht die davor
 vorhandenen L's. Zwischen "MOBA" und "LED" sind drei "L". Darum werden drei Punkte vorangestellt.

 Achtung:
 Die CopyLED() Funktion darf in der Konfiguration nur dann verwendet werden wenn sie während der Anzeige
 der Texte abgeschaktet ist. Dazu mus in der Spalte "Adresse oder Name" eine entsprechende Steuervariable
 stehen.


 Revision History:
 ~~~~~~~~~~~~~~~~~
 23.05.23:  - Started


 ToDo:
 ~~~~~
 - Sonderzeichen mit dem man die Buchstaben eines Wortes sofort anzeigen kann.
*/

#ifndef __WORD_CLOCK_TEXT_EXTENTION__
#define __WORD_CLOCK_TEXT_EXTENTION__
#if !defined(AVR) && !defined(ESP32)
  #error Platform is not supported
#endif


#include <MLLExtension.h>

// The following defines could be changed in the excel table
#ifndef WCT_LETTERS
#define WCT_LETTERS "  ESMOBAHISTAJ  "    \
                    "  ZEHNXZWANZIG  "    \
                    "  \304DREIVIERTEL  " \
                    "  F\334NFAVORNACH  " \
                    "  HALBDELF\334NFN  " \
                    "  EINSLEDZWEIY  "    \
                    "  DREIVIERPLIB  "    \
                    "  RGBSECHSACHT  "    \
                    "  STUMMIKZW\326LF  " \
                    "  ZEHNEUNFORUM  "    \
                    "  SIEBENQUHRNM  "    \
                    "  MISAFRDISODO  "
// German letters AE = \304, OE = \326, UE = \334
#endif

#ifndef WCT_CHARACTER_PERIOD
#define WCT_CHARACTER_PERIOD 600
#endif

#ifndef WCT_WORD_END_PERIOD
#define WCT_WORD_END_PERIOD  4000
#endif

#ifndef WCT_END_PERIOD
#define WCT_END_PERIOD       3000
#endif


#ifndef WCT_FIRST_LETTER_NR
#define WCT_FIRST_LETTER_NR 33
#endif

#ifndef WCT_S_ORDER
#define WCT_S_ORDER  1     // 0: The LEDs are arranged in "Z" form => 012
#endif                     //                                         345
                           //                                         678
                           // 1: The LEDs are arranged in "S" form => 210
                           //                                         345
                           //                                         876

#ifndef WCT_LINE_LEN
#define WCT_LINE_LEN 16    // Length of the LED rows. Used to reverse every second line if WCT_S_ORDER is achtive
#endif

#ifndef WCT_REVERSE_SECOND_LINE
#define WCT_REVERSE_SECOND_LINE  0
#endif



char Letters[] = WCT_LETTERS;
bool initialized = false;


//***************************************
class WordClockText : public MLLExtension
//***************************************
{
  private:
    uint8_t     InCh;
    bool        OldInp;
    const char *Text;
    const char *p;
    const char *lp;
    uint32_t    LastChar_Time;
    uint16_t    Period;
    uint8_t     SkipCnt;


    //--------------------------------------------------
    public:WordClockText(uint8_t InCh, const char *Text) // Constructor
    //--------------------------------------------------
    {
      this->InCh = InCh;
      this->Text = Text;
      p = Text;
      LastChar_Time = 0;
      OldInp = 0;
      lp = Letters;
      SkipCnt = 0;
	}

    //-----------------------------------------
    public:void setup(MobaLedLib_C& mobaLedLib)
    //-----------------------------------------
    {
//      Serial << F("\nWordClockText setup\n"); // Debug
//      Serial << Text << "\n";                 // Debug
      #if WCT_S_ORDER == 1
        if (initialized == false)
           {
           initialized = true;
           Period      = 0;
           //Rearange_Letter();
           }
      #endif
	}

    //----------------------------------------------
    private:int16_t Get_LED_Nr_Matching_Char(char c)
    //----------------------------------------------
    {
       if (c == ' ') return -1;
       if (c == '.') { SkipCnt++; return -1; }
       c = toupper(c);
       //if (SkipCnt) Serial << "SkipCnt:" << SkipCnt << endl; // Debug
       switch (c)
          { // Convert lower caser German umlauts to upper case
          case '\344': c = '\304'; break; // ae
          case '\366': c = '\326'; break; // oe
          case '\374': c = '\334'; break; // ue
          }
       int16_t Nr;
       bool Skip;
       do {
          Nr = -1;
          for (uint16_t i = 0; i < sizeof(Letters) && Nr == -1; i++)
             {
             if (c == *lp) Nr = lp - Letters;
             lp++;
             if (*lp == '\0') lp = Letters; // Start from the beginning
             }
          if (Nr != -1)
             {
             #if WCT_S_ORDER
                uint16_t LineNr = Nr / WCT_LINE_LEN;
                   if ((LineNr % 2) == WCT_REVERSE_SECOND_LINE)
                      { // reverse the line index if S-form is used
                      uint8_t LineIx = Nr % WCT_LINE_LEN;
                      // Ix    New    Delta    Calculation
                      // 0  -> 15     + 15
                      // 1  -> 14     + 13
                      // 2  -> 13     + 11
                      // 3  -> 12     +  9
                      // 4  -> 11     +  7
                      // 5  -> 10     +  5
                      // 6  ->  9     +  3
                      // 7  ->  8     +  1     15 - 7*2 =  1
                      // 8  ->  7     -  1     15 - 8*2 = -1
                      // 9  ->  6     -  3     15 - 9*2 = -1
                      // 10 ->  5     -  5
                      // 11 ->  4     -  7
                      // 12 ->  3     -  9
                      // 13 ->  2     - 11
                      // 14 ->  1     - 13
                      int16_t Delta = WCT_LINE_LEN - 1 - LineIx*2;
                      //if (c == 'H')  /* Debug */ { Serial << "LineNr:" << LineNr << " Nr:" << Nr << " Ix: " << LineIx << " Delta: " << Delta << "\n"; }
                      Nr += Delta;
                      }
             #endif
             Nr += WCT_FIRST_LETTER_NR;
             }
            if (SkipCnt > 0)
                { Skip = true; SkipCnt--;}
            else  Skip = false;
          } while (Skip);

       return Nr;
    };

    //------------------------------------------------
    private:void New_Display(MobaLedLib_C& mobaLedLib)
    //------------------------------------------------
    {
      lp = Letters;
      fill_solid(&mobaLedLib.leds[WCT_FIRST_LETTER_NR], sizeof(Letters)-1, CRGB::Black);
    }

    //----------------------------------------
    public:void loop2(MobaLedLib_C& mobaLedLib)
    //----------------------------------------
    {
       uint8_t Inp = mobaLedLib.Get_Input(InCh);
       bool Inp_On = Inp_Is_On(Inp);

       if (!OldInp && Inp_On)  // INP_TURNED_ON / INP_TURNED_OFF funktioniert nicht weil loop2() der extention nach dem aufruf von Inp_Processed() aufgerufen wird ;-(
          {
          //LastChar_Time = 0;
          Period = 0;
          p = Text;
          //Serial << F("<ON>\n");   // Debug
          lp = Letters; // Start from the beginning
          }
       if (OldInp && !Inp_On)
          {
          New_Display(mobaLedLib);
          Serial << F(" <OFF>\n"); // Debug
          p = Text;
          }
       if (Inp_On)
          {
          if (millis() - LastChar_Time > Period)  // Overflow save calculation
             {
             LastChar_Time = millis();
             switch (*p)
                {
                case '\0': p = Text; // no break;
                case ' ' : New_Display(mobaLedLib); // No break;
                }

             int16_t LEDNr = Get_LED_Nr_Matching_Char(*p);
             if (LEDNr >= 0)
                {
                uint8_t r = mobaLedLib.leds[LEDNr].r; // ToDo: Was ist bei mehreren LED Bussen?
                uint8_t g = mobaLedLib.leds[LEDNr].g;
                uint8_t b = mobaLedLib.leds[LEDNr].b;
                if      (r == 0 && g == 0 && b == 0) mobaLedLib.leds[LEDNr] = CRGB::Red;
                else if (r == 255)                   mobaLedLib.leds[LEDNr] = CRGB::Green;
                else                                 mobaLedLib.leds[LEDNr] = CRGB::Blue;
                }

             Serial << *p ;    // Debug
             p++;
             if (*p == '\0') Serial << "\n"; // Debug

             switch (*p)
                {
                case '\0': Period = WCT_END_PERIOD;       break;
                case ' ' : Period = WCT_WORD_END_PERIOD;  break;
                case '.' : Period = 0;                    break;
                default:   Period = WCT_CHARACTER_PERIOD;
                }
             }
          }
       OldInp = Inp_On;
    }
};


#endif // __WORD_CLOCK_TEXT_EXTENTION__
/*

*/
