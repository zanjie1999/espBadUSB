#include <Keyboard.h>
#include <Mouse.h>
#include <FingerprintUSBHost.h>

#define BAUD_RATE 57200
#define FLASH_PIN A0

#define ExternSerial Serial1

boolean isFlash = false;
String os = "";
String bufferStr = "";
String last = "";
int defaultDelay = 0;

boolean dontwrite = false;
boolean textmode = false;

void Line(String _line) {
  int firstSpace = _line.indexOf(" ");

  if (firstSpace == -1) {
    if (_line.equals("TEXT")) {
      textmode = true;
    } else if (_line.equals("OSEND")) {
      dontwrite = false;
    }
  } else if (_line.substring(0, firstSpace) == "OS") {
    dontwrite = true;
    if (_line.substring(firstSpace + 1) == os) dontwrite = false;
  }

  if (!dontwrite) {
    if (firstSpace == -1)  Press(_line);
    else if (_line.substring(0, firstSpace) == "STRING") {
      for (int i = firstSpace + 1; i < _line.length(); i++) Keyboard.write(_line[i]);
    } else if (_line.substring(0, firstSpace) == "ASCII") {
      Keyboard.write(_line.substring(firstSpace + 1).toInt());
    } else if (_line.substring(0, firstSpace) == "SCROLL") {
      Mouse.move(0, 0, _line.substring(firstSpace + 1).toInt());
    } else if (_line.substring(0, firstSpace) == "MOUSEX") {
      Mouse.move(_line.substring(firstSpace + 1).toInt(), 0);
    } else if (_line.substring(0, firstSpace) == "MOUSEY") {
      Mouse.move(0, _line.substring(firstSpace + 1).toInt());
    } else if (_line.substring(0, firstSpace) == "MOUSE") {
      int index = _line.indexOf(" ", firstSpace + 1);
      Mouse.move(_line.substring(firstSpace + 1).toInt(), _line.substring(index + 1).toInt());
    } else if (_line.substring(0, firstSpace) == "DELAY") {
      int delaytime = _line.substring(firstSpace + 1).toInt();
      delay(delaytime);
    }
    else if (_line.substring(0, firstSpace) == "DEFAULTDELAY") defaultDelay = _line.substring(firstSpace + 1).toInt();
    else if (_line.substring(0, firstSpace) == "REM") {} //nothing :/
    else if (_line.substring(0, firstSpace) == "REPLAY") {
      int replaynum = _line.substring(firstSpace + 1).toInt();
      while (replaynum)
      {
        Line(last);
        --replaynum;
      }
    } else {
      String remain = _line;

      while (remain.length() > 0) {
        int latest_space = remain.indexOf(" ");
        if (latest_space == -1) {
          Press(remain);
          remain = "";
        }
        else {
          Press(remain.substring(0, latest_space));
          remain = remain.substring(latest_space + 1);
        }
        delay(5);
      }
    }

    Keyboard.releaseAll();
    delay(defaultDelay);
  }

}


void Press(String b) {
  if (b.length() == 1) Keyboard.press(char(b[0]));
  else if (b.equals("ENTER")) Keyboard.press(KEY_RETURN);
  else if (b.equals("CTRL")) Keyboard.press(KEY_LEFT_CTRL);
  else if (b.equals("SHIFT")) Keyboard.press(KEY_LEFT_SHIFT);
  else if (b.equals("ALT")) Keyboard.press(KEY_LEFT_ALT);
  else if (b.equals("GUI")) Keyboard.press(KEY_LEFT_GUI);
  else if (b.equals("UP") || b.equals("UPARROW")) Keyboard.press(KEY_UP_ARROW);
  else if (b.equals("DOWN") || b.equals("DOWNARROW")) Keyboard.press(KEY_DOWN_ARROW);
  else if (b.equals("LEFT") || b.equals("LEFTARROW")) Keyboard.press(KEY_LEFT_ARROW);
  else if (b.equals("RIGHT") || b.equals("RIGHTARROW")) Keyboard.press(KEY_RIGHT_ARROW);
  else if (b.equals("DELETE")) Keyboard.press(KEY_DELETE);
  else if (b.equals("PAGEUP")) Keyboard.press(KEY_PAGE_UP);
  else if (b.equals("PAGEDOWN")) Keyboard.press(KEY_PAGE_DOWN);
  else if (b.equals("HOME")) Keyboard.press(KEY_HOME);
  else if (b.equals("ESC")) Keyboard.press(KEY_ESC);
  else if (b.equals("BACKSPACE")) Keyboard.press(KEY_BACKSPACE);
  else if (b.equals("INSERT")) Keyboard.press(KEY_INSERT);
  else if (b.equals("TAB")) Keyboard.press(KEY_TAB);
  else if (b.equals("END")) Keyboard.press(KEY_END);
  else if (b.equals("CAPSLOCK")) Keyboard.press(KEY_CAPS_LOCK);
  else if (b.equals("F1")) Keyboard.press(KEY_F1);
  else if (b.equals("F2")) Keyboard.press(KEY_F2);
  else if (b.equals("F3")) Keyboard.press(KEY_F3);
  else if (b.equals("F4")) Keyboard.press(KEY_F4);
  else if (b.equals("F5")) Keyboard.press(KEY_F5);
  else if (b.equals("F6")) Keyboard.press(KEY_F6);
  else if (b.equals("F7")) Keyboard.press(KEY_F7);
  else if (b.equals("F8")) Keyboard.press(KEY_F8);
  else if (b.equals("F9")) Keyboard.press(KEY_F9);
  else if (b.equals("F10")) Keyboard.press(KEY_F10);
  else if (b.equals("F11")) Keyboard.press(KEY_F11);
  else if (b.equals("F12")) Keyboard.press(KEY_F12);
  else if (b.equals("NUM_0")) Keyboard.press(234);
  else if (b.equals("NUM_1")) Keyboard.press(225);
  else if (b.equals("NUM_2")) Keyboard.press(226);
  else if (b.equals("NUM_3")) Keyboard.press(227);
  else if (b.equals("NUM_4")) Keyboard.press(228);
  else if (b.equals("NUM_5")) Keyboard.press(229);
  else if (b.equals("NUM_6")) Keyboard.press(230);
  else if (b.equals("NUM_7")) Keyboard.press(231);
  else if (b.equals("NUM_8")) Keyboard.press(232);
  else if (b.equals("NUM_9")) Keyboard.press(233);
  else if (b.equals("CZ_0")) Keyboard.press(41);//////// Czech layout is different
  else if (b.equals("CZ_1")) Keyboard.press(33);
  else if (b.equals("CZ_2")) Keyboard.press(64);
  else if (b.equals("CZ_3")) Keyboard.press(35);
  else if (b.equals("CZ_4")) Keyboard.press(36);
  else if (b.equals("CZ_5")) Keyboard.press(37);
  else if (b.equals("CZ_6")) Keyboard.press(94);
  else if (b.equals("CZ_7")) Keyboard.press(38);
  else if (b.equals("CZ_8")) Keyboard.press(42);
  else if (b.equals("CZ_9")) Keyboard.press(40);
  else if (b.equals("ASTERIX") || b.equals("HVEZDICKA")) Keyboard.press(221);
  else if (b.equals("MINUS")) Keyboard.press(222);
  else if (b.equals("PLUS")) Keyboard.press(223);
  else if (b.equals("SLASH")) Keyboard.press(220);
  else if (b.equals("SPACE")) Keyboard.press(' ');
  else if (b.equals("PRINTSCREEN")) Keyboard.press(206);

  // Mouse
  else if (b.equals("CLICK") || b.equals("CLICK_LEFT") || b.equals("MOUSE_CLICK_LEFT") || b.equals("MOUSE_CLICK")) Mouse.click();
  else if (b.equals("CLICK_RIGHT") || b.equals("MOUSE_CLICK_RIGHT")) Mouse.click(MOUSE_RIGHT);
  else if (b.equals("CLICK_MIDDLE") || b.equals("MOUSE_CLICK_MIDDLE")) Mouse.click(MOUSE_MIDDLE);

  else if (b.equals("PRESS") || b.equals("PRESS_LEFT") || b.equals("MOUSE_PRESS_LEFT")) Mouse.press();
  else if (b.equals("PRESS_RIGHT") || b.equals("MOUSE_PRESS_RIGHT")) Mouse.press(MOUSE_RIGHT);
  else if (b.equals("PRESS_MIDDLE") || b.equals("MOUSE_PRESS_MIDDLE")) Mouse.press(MOUSE_MIDDLE);

  else if (b.equals("RELEASE") || b.equals("RELEASE_LEFT") || b.equals("MOUSE_RELEASE_LEFT") || b.equals("MOUSE_RELEASE")) Mouse.release();
  else if (b.equals("RELEASE_RIGHT") || b.equals("MOUSE_RELEASE_RIGHT")) Mouse.release(MOUSE_RIGHT);
  else if (b.equals("RELEASE_MIDDLE") || b.equals("MOUSE_RELEASE_MIDDLE")) Mouse.release(MOUSE_MIDDLE);
  //else Serial.println("not found :'"+b+"'("+String(b.length())+")");
}

void setup() {
  Keyboard.begin();
  Mouse.begin();
  Serial.begin(BAUD_RATE);
  ExternSerial.begin(BAUD_RATE);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(FLASH_PIN, INPUT_PULLUP);
  isFlash = analogRead(FLASH_PIN) < 100;
  pinMode(FLASH_PIN, INPUT);
  if (isFlash) {
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void loop() {
  if (isFlash) {
    while (ExternSerial.available()) {
      digitalWrite(LED_BUILTIN, LOW);
      Serial.write((uint8_t)ExternSerial.read());
      digitalWrite(LED_BUILTIN, HIGH);
    }

    if (Serial.available()) {
      while (Serial.available()) {
        digitalWrite(LED_BUILTIN, LOW);
        ExternSerial.write((uint8_t)Serial.read());
        digitalWrite(LED_BUILTIN, HIGH);
      }
    }
  } else {
    if (ExternSerial.available()) {
      digitalWrite(LED_BUILTIN, HIGH);
      bufferStr = ExternSerial.readStringUntil("END");
      Serial.println(bufferStr);
    }

    if (bufferStr.length() > 0) {
      if (os == "") {
        FingerprintUSBHost.guessHostOS(os);
        os.toUpperCase();
      }

      bufferStr.replace("\r\n", "\n");
      bufferStr.replace("\r", "\n");

      while (bufferStr.length() > 0) {
        if (textmode) {
          int textend = bufferStr.indexOf("\nTEXTEND");
          if (textend == -1) {
            for (int i = 0; i < bufferStr.length() - 1; i++) Keyboard.write(bufferStr[i]);
            bufferStr = "";
          } else {
            for (int i = 0; i < textend; i++) Keyboard.write(bufferStr[i]);
            bufferStr = bufferStr.substring(textend + 8);
            textmode = false;
          }
        } else {
          int latest_return = bufferStr.indexOf("\n");
          if (latest_return == -1) {
            Serial.println("run: " + bufferStr);
            Line(bufferStr);
            bufferStr = "";
          } else {
            Serial.println("run: '" + bufferStr.substring(0, latest_return) + "'");
            Line(bufferStr.substring(0, latest_return));
            last = bufferStr.substring(0, latest_return);
            bufferStr = bufferStr.substring(latest_return + 1);
          }
        }
      }

      bufferStr = "";
      dontwrite = false;
      textmode = false;
      ExternSerial.write(0x99);
      Serial.println("done");
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}
