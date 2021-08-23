#include <Arduino.h>
#include <EEPROM.h>
#include <Timer.h>
#include <SPI.h>
#include <dogm_7036.h>
#include <ClickEncoder.h>
#include "VS1053.h"

#include "Trellis.h"

//pattern for own defined character
const byte val1  [] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F};
const byte val2  [] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F};
const byte val3  [] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F};
const byte val4  [] = {0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F};
const byte val5  [] = {0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
const byte val6  [] = {0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
const byte val7  [] = {0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
const byte val8  [] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};

//the following port definitions are used by our demo board "EA PCBARDDOG7036"
int _pin_backlight = 13; //PC7 / OC4A // influences INP !!! ???
int _pin_note = 11;
int _pin_gate = 12;

Timer  _timer;
ClickEncoder _encoder(A1, A0, A2, 4);  // A B Button, ticksPerStep
dogm_7036 DOG;

enum
{
    menuItemBPM       = 0,
    menuItemBacklight,
    menuItemContrast,
    menuItemVS1053_instr,
    menuItemVS1053_vol,
    menuItemVS1053_oct
};
const char*   _menuTitle[] = {"BPM", "backlight", "contrast", "vs1053 instr", "vs1053 vol", "vs1053_oct"};
const int16_t _menuMinim[] = { 30,   0,   0,   0,   0,   0};  //min values
const int16_t _menuMaxim[] = {256, 256,  64, 128, 128,  11};  //max values
int16_t       _menuValue[] = {120, 100,  32,   0, 127,   5};  //init values
bool          _menuOn = false;
int16_t       _iMenuItems = 6;
int16_t       _iMenuSelect = 0;

byte     _seqScale[] = {0,0,2,4,5,7,9,11,12};
byte     _seqNote[] = {0,1,2,3, 0,2,4,6, 8,4,6,0, 8,4,6,6};
byte     _seqGate[] = {8,4,6,0, 8,4,6,6, 0,1,2,3, 0,2,4,6};
byte     _seqStep = 0;
byte     _seqBPM  = 120;
uint16_t _seqNoteScale = 255/12/5;

int _eventIdSeq = 0;

byte _vs1053_oldnote = 0;

void eventSequenceAdvanceCB()
{
    byte bnote = _seqNote[_seqStep];
    byte snote = _seqScale[bnote];
    uint16_t note1 = _seqNoteScale*(snote);

    byte gate  = _seqGate[_seqStep];
    digitalWrite(_pin_gate, (bnote != 0));
    analogWrite(_pin_note, note1);
    
    // get BPM from menu
    _timer.period(_eventIdSeq, 60000/(_menuValue[menuItemBPM]*4));

    //  for (uint8_t i=60; i<69; i++) {
    const byte kick = 64; //41; //36; 64
    const byte snare = 38;
    const byte hihat = 42;

    if(bnote != 0 && _menuValue[menuItemVS1053_instr] != 0)
    {
        byte theNote = 12*_menuValue[menuItemVS1053_oct]+snote;
        VS1053::midiSetInstrument (0, _menuValue[menuItemVS1053_instr]);
        VS1053::midiNoteOff(0, _vs1053_oldnote, 127);
        VS1053::midiNoteOn (0, theNote, _menuValue[menuItemVS1053_vol]);
        _vs1053_oldnote = theNote;
    }
    #if 0
    else
    {
        VS1053::midiNoteOff(0, _vs1053_oldnote, 127);
        _vs1053_oldnote = 0;
    }
    #endif
    
#define _play_se_drums__
#ifdef _all_vs_drum_sounds__
    Serial.print("snd:");
    Serial.println(vs1053);
    VS1053::midiNoteOn (1, vs1053, 127);
#endif
#ifdef _play_se_drums__
    if(_seqStep%4 == 0)
    {
        VS1053::midiNoteOff(1, kick, 127);
        VS1053::midiNoteOn (1, kick, 127);
    }
    if(_seqStep%4 == 2)
    {
        VS1053::midiNoteOff(1, snare, 64);
        VS1053::midiNoteOn (1, snare, 64);
    }
    if(_seqStep%2 == 0)
    {
        if(_seqStep/4 == 3)
        {
        VS1053::midiNoteOff(2, hihat+4, 127);
        VS1053::midiNoteOn (2, hihat+4, 127);
        }
        else
        {
        VS1053::midiNoteOff(2, hihat, 127);
        VS1053::midiNoteOn (2, hihat, 127);
        }
    }
#endif

    _seqStep = (_seqStep + 1) % 16;
}

void updateEncoderCB()
{
    _encoder.service();

    auto button = _encoder.getButton();
    if(button == ClickEncoder::DoubleClicked)
    {
        _menuOn = !_menuOn;
    }
    if(_menuOn)
    {
        if(button == ClickEncoder::Clicked)
        {
            _iMenuSelect = (_iMenuSelect += 1) % _iMenuItems;
        }
        _menuValue[_iMenuSelect] = (_menuValue[_iMenuSelect] + _encoder.getValue()) % _menuMaxim[_iMenuSelect];
        if(_menuValue[_iMenuSelect] < _menuMinim[_iMenuSelect])
            _menuValue[_iMenuSelect] = _menuMinim[_iMenuSelect];
    }
    else
    {
        byte n = _seqNote[Trellis::_lastPressed];
        auto v = _encoder.getValue();
        if(n == 0 && v < 0)
        {
        }
        else
        {
            n = (n + v) % 9;
        }
        _seqNote[Trellis::_lastPressed] = n;
    }
}

void updateDisplay()
{
    DOG.clear_display();
    DOG.position(1,1);
    DOG.string("step:");
    DOG.position(6,1);
    String sval1(Trellis::_lastPressed+1);
    DOG.string(sval1.c_str());

    DOG.position(11,1);
    String sval2(_seqStep/4+1);
    DOG.string(sval2.c_str());
    DOG.string(".");
    String sval3(_seqStep%4+1);
    DOG.string(sval3.c_str());
    DOG.position(15,1);
    DOG.ascii(_seqNote[_seqStep]-1);
    DOG.ascii(_seqGate[_seqStep]-1);

    /*
    DOG.position(10,1);
    DOG.string("gate:");
    String sval3(_seqLength[Trellis::_lastPressed]);
    DOG.position(16-sval3.length(),1);
    DOG.string(sval3.c_str());
    DOG.position(16,1);
    DOG.ascii(_seqLength[Trellis::_lastPressed]);
    */
    // menu
    if(_menuOn)
    {
        DOG.position(1,2);
        DOG.string(_menuTitle[_iMenuSelect]);
    
        String sval4(_menuValue[_iMenuSelect]);
        DOG.position(17-sval4.length(),2);
        DOG.string(sval4.c_str());
    
        DOG.position(16,2);
        
        DOG.contrast(_menuValue[menuItemContrast]);
        analogWrite(_pin_backlight, _menuValue[menuItemBacklight]);
    }
    else
    {
        DOG.position(1,2);
        for(byte i = 0; i < 16; i++)
        {
            byte val = _seqNote[i];
            if(val)
                DOG.ascii(val-1);
            else
                DOG.string(" ");
        }
    }
}

void updateSerial()
{
//    Serial.print("btn:");
//    Serial.println(_encoder.getButton());
    {
        //Serial.print("This is Arpus");
        //Serial.println();
    }
}

void loadMenu()
{
    const int oldOff = 0;
    int addr = 0;
    for(int i = 0; i < _iMenuItems - oldOff; i++)
    {
        byte lo, hi;
        EEPROM.get(addr+0, lo);
        EEPROM.get(addr+1, hi);
        int16_t val = hi<<8 | lo;
        _menuValue[i] = val;
        addr += sizeof(int16_t);
    }
    
    for(int i = 0; i < 16; i++)
    {
        byte val;
        EEPROM.get(addr, val);
        _seqNote[i] = val;
        addr += sizeof(byte);
    }
}

void saveMenu()
{
    int addr = 0;
    for(int i = 0; i < _iMenuItems; i++)
    {
        int16_t val = _menuValue[i];
        byte lo = val, hi = val>>8;
        EEPROM.update(addr+0, lo);
        EEPROM.update(addr+1, hi);
        addr += sizeof(int16_t);
    }
    
    for(int i = 0; i < 16; i++)
    {
        byte val = _seqNote[i];
        EEPROM.update(addr, val);
        addr += sizeof(byte);
    }
}

void setup()
{
    Serial.begin(115200);
    
    _encoder.setAccelerationEnabled(true);
    loadMenu();
    
    _timer.every(  1, updateEncoderCB);       // 1ms rotary encoder updates
    _timer.every(100, updateDisplay);  // 10fps display updates
    _timer.every(100, updateSerial);  // 10fps display updates
    _timer.every(3000, saveMenu);      // save values every 3 sec
    _timer.every(30, Trellis::loop);      // save values every 3 sec
    _eventIdSeq = _timer.every(60000/(_menuValue[menuItemBPM]*4), eventSequenceAdvanceCB);

    DOG.initialize(10,0,0,9,8,1,DOGM162);   //SS = 10, 0,0= use Hardware SPI, 9 = RS, 4= RESET, 1 = 5V, EA DOGM081-A (=1 line)
    DOG.displ_onoff(true);          //turn Display on
    DOG.cursor_onoff(false);         //turn Curosor blinking on
    DOG.define_char(0, val1); //define own char on memory adress 0
    DOG.define_char(1, val2);
    DOG.define_char(2, val3);
    DOG.define_char(3, val4);
    DOG.define_char(4, val5);
    DOG.define_char(5, val6);
    DOG.define_char(6, val7);
    DOG.define_char(7, val8);

    pinMode(_pin_backlight, OUTPUT);
    digitalWrite(_pin_backlight, 1);
    //pinMode(_pin_note, OUTPUT);
    //digitalWrite(_pin_note, 0);
    pinMode(_pin_gate, OUTPUT);
    digitalWrite(_pin_gate, 0);

    Trellis::setup();

    VS1053::setup();
}

void loop()
{
    _timer.update();
}

