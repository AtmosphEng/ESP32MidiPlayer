// Basic MIDI playback example. 
// To upload midi to ESP32's storage
// you can use my ESP32PartitionTool to upload LittleFS binary (chose LittleFS as SPIFFS type before upload)
// ESP32PartitionTool > https://github.com/serifpersia/esp32partitiontool

// Serial commands: play, pause, resume, stop

// DEFINE ****************************************************************************************

#define BITS_PER_BYTE 8

#define MIDI_MSG_BYTE_NOTE_OFF            0x80
#define MIDI_MSG_BYTE_NOTE_ON             0x90
#define MIDI_MSG_STATUS_CONTROL_CHANGE    0xb0
#define MIDI_MSG_STATUS_PROGRAM_CHANGE    0xc0
#define MIDI_MSG_STATUS_PITCH_BEND        0xe0


//#define TARGET_ESP32_KS0413
#define TARGET_ESP32_CYD
//#define TARGET_ESP32_T7_S3
//#define TARGET_XIAO_SAMD21 // NOTE - NOT AN ESP32 - 2026-01-03 NOK - THIS FAILS AT LOADING A MIDI FILE. WHY?

//#define FS_LITTLEFS 
#define FS_SD 

#define DEBUG_SERIAL_OUT
#define SERIAL_MIDI_STREAM_OUT
#define MIDI_CHANNEL_SPECIFIC // allow channel (0-15) specification in the msg, otherwise its all notes on channel 0.

// Choose serial port for midi stream to seeed midi synth.
#ifdef TARGET_XIAO_SAMD21
#define SERIAL_MIDI Serial1 //  for xiao samd21
#elif defined(TARGET_ESP32_CYD)
#define SERIAL_MIDI Serial // for TCMENU to allow use of CYD with rotary encoder. Free-up Serial2 hw pins for encoder.
//#define SERIAL_MIDI Serial2
#else
#define SERIAL_MIDI Serial2
#endif

#define MUTED_CHANNEL_A 9 // equivalent as '10' for 1-16. Drum channel.

// INCLUDE ****************************************************************************************

#if 0 // if 1 then PRESERVE serial dubug prints to a chosen port, else serial debug prints are REMOVED.
#define MYDEBUGON
#define MYDEBUGSERIALCHAN Serial
#endif
#include "D:\SBOX\WORK\aaaPROJ\libraries\MyShared\src\myDebugPrint.h"

#ifdef FS_LITTLEFS 

#include <LittleFS.h>
ESP32MidiPlayer midiPlayer(LittleFS); // Use LittleFS

#elif defined(FS_SD)

#if defined(TARGET_XIAO_SAMD21) 
#include <SPI.h> //  for Seeed Xiao samd21
// #include <SD.h> 
#include <Seeed_Arduino_FS.h> 
#define SD_CS 2  // xiao samd21 expansion board

#elif defined(TARGET_ESP32_CYD)
#include <SD.h> 
#include <FS.h> 
#define SD_CS 5  // cyd sd_cs

// #include "ESP32MidiPlayer.h"
#include "src/ESP32MidiPlayer.h" //  mod

ESP32MidiPlayer midiPlayer(SD); // Use SD and then FS

#else // default
#include <SD.h> 
#include <FS.h> 
#define SD_CS 5 

#endif // if defined(TARGET_XIAO_SAMD21) 

#endif // FS_LITTLEFS


// const char* MIDI_FILE = "/small.mid";
const char* MIDI_FILE = "/file001.mid"; //  copy of Pachelbel.mid
// const char* MIDI_FILE = "/test.mid";
// const char* MIDI_FILE = "/song.mid";


// FUNCTIONS **************************************************************************************

void handleLog(MidiLogLevel level, const char* message) {
  const char* levelStr = "";
  switch (level) {
    case MidiLogLevel::ERROR:
      levelStr = "[ERR] "; // Errors that might halt playback or indicate corruption
      break;
    case MidiLogLevel::WARN:
      levelStr = "[WRN] "; // Warnings about unexpected data or potential issues
      break;
    case MidiLogLevel::INFO:
      levelStr = "[INF] "; // General information (playback start/stop, file loaded)
      break;
    case MidiLogLevel::DEBUG:
      levelStr = "[DBG] "; // Detailed debugging steps (event parsing, byte reads)
      break;
    case MidiLogLevel::VERBOSE:
      levelStr = "[VER] "; // Extremely detailed info (often too noisy)
      break;
    // case MidiLogLevel::NONE: // No need to handle NONE, the library checks this
    default:
      levelStr = "[???] "; // Unknown level? Should not happen.
      break;
  }
  // Print the prefix and the message, followed by a newline
  MYDEBUGPRINT_F_3("%s%s\n", levelStr, message);
}


void handleNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {

  if(channel != MUTED_CHANNEL_A){ // specify channel to be muted.

#ifdef DEBUG_SERIAL_OUT
    MYDEBUGPRINT_F_5("[EVT] Note On:  Ch=%u Note=%u Vel=%u (Tick: %lu)\n",
    channel + 1, note, velocity, midiPlayer.getCurrentTick()); // Display channel 1-16
#endif // DEBUG_SERIAL_OUT

#ifdef SERIAL_MIDI_STREAM_OUT

#ifdef MIDI_CHANNEL_SPECIFIC
    SERIAL_MIDI.write((MIDI_MSG_BYTE_NOTE_ON & 0xf0) | ((channel + 1) & 0x0f)); // note on, merge channel number in to message.
#else
    SERIAL_MIDI.write(MIDI_MSG_BYTE_NOTE_ON); // note on, channel "0" = channel 1  = all channels
#endif // MIDI_CHANNEL_SPECIFIC

    SERIAL_MIDI.write(note); // note value
    SERIAL_MIDI.write(velocity); // velocity value

#endif // SERIAL_MIDI_STREAM_OUT 
  } // if channel ...
} // handleNoteOn

void handleNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {

  if(channel != MUTED_CHANNEL_A){ // specify channel to be muted

#ifdef DEBUG_SERIAL_OUT
    MYDEBUGPRINT_F_5("[EVT] Note Off: Ch=%u Note=%u Vel=%u (Tick: %lu)\n",
    channel + 1, note, velocity, midiPlayer.getCurrentTick()); // Display channel 1-16
#endif // DEBUG_SERIAL_OUT

#ifdef SERIAL_MIDI_STREAM_OUT

    #ifdef MIDI_CHANNEL_SPECIFIC
    SERIAL_MIDI.write((MIDI_MSG_BYTE_NOTE_OFF & 0xf0) | ((channel + 1) & 0x0f)); // note on, merge channel number in to message.
#else
    SERIAL_MIDI.write(MIDI_MSG_BYTE_NOTE_OFF); // note off, channel "0" = channel 1 = all channels
    #endif // MIDI_CHANNEL_SPECIFIC

    SERIAL_MIDI.write(note); // note value
    SERIAL_MIDI.write(velocity); // velocity value

#endif // SERIAL_MIDI_STREAM_OUT
  } // if channel ...

} // handleNoteOff


void handleControlChange(uint8_t channel, uint8_t controller, uint8_t value) {

#ifdef DEBUG_SERIAL_OUT
MYDEBUGPRINT_F_5("[EVT] Ctrl Chg: Ch=%u CC=%u Val=%u (Tick: %lu)\n",
channel + 1, controller, value, midiPlayer.getCurrentTick()); // Display channel 1-16
#endif // DEBUG_SERIAL_OUT

#ifdef SERIAL_MIDI_STREAM_OUT

#ifdef MIDI_CHANNEL_SPECIFIC
SERIAL_MIDI.write((MIDI_MSG_STATUS_CONTROL_CHANGE & 0xf0) | ((channel + 1) & 0x0f)); // note on, merge channel number in to message.
#else
SERIAL_MIDI.write(MIDI_MSG_STATUS_CONTROL_CHANGE);
#endif // MIDI_CHANNEL_SPECIFIC

SERIAL_MIDI.write(controller); // data byte 1
SERIAL_MIDI.write(value); // data byte 2

#endif // SERIAL_MIDI_STREAM_OUT

} // handleControlChange

void handleProgramChange(uint8_t channel, uint8_t program) {

#ifdef DEBUG_SERIAL_OUT
MYDEBUGPRINT_F_4("[EVT] Prog Chg: Ch=%u Prog=%u (Tick: %lu)\n",
channel + 1, program, midiPlayer.getCurrentTick()); // Display channel 1-16
#endif // DEBUG_SERIAL_OUT

#ifdef SERIAL_MIDI_STREAM_OUT

#ifdef MIDI_CHANNEL_SPECIFIC
SERIAL_MIDI.write((MIDI_MSG_STATUS_PROGRAM_CHANGE & 0xf0) | ((channel + 1) & 0x0f)); // note on, merge channel number in to message.
#else
SERIAL_MIDI.write(MIDI_MSG_STATUS_PROGRAM_CHANGE);
#endif // MIDI_CHANNEL_SPECIFIC

SERIAL_MIDI.write(program); // data byte 1

#endif // SERIAL_MIDI_STREAM_OUT

} // handleProgramChange

void handlePitchBend(uint8_t channel, int16_t value) {

#ifdef DEBUG_SERIAL_OUT
MYDEBUGPRINT_F_4("[EVT] Pitch Bnd: Ch=%u Val=%d (Tick: %lu)\n",
channel + 1, value, midiPlayer.getCurrentTick()); // Display channel 1-16
#endif // DEBUG_SERIAL_OUT

#ifdef SERIAL_MIDI_STREAM_OUT

#ifdef MIDI_CHANNEL_SPECIFIC
SERIAL_MIDI.write((MIDI_MSG_STATUS_PITCH_BEND & 0xf0) | ((channel + 1) & 0x0f)); // note on, merge channel number in to message.
#else
SERIAL_MIDI.write(MIDI_MSG_STATUS_PITCH_BEND);
#endif // MIDI_CHANNEL_SPECIFIC

SERIAL_MIDI.write(value & 0x00ff); // data byte 1 - ls byte
SERIAL_MIDI.write((value & 0xff00 >> BITS_PER_BYTE)); // data byte 1 - ms byte

#endif // SERIAL_MIDI_STREAM_OUT

} // handlePitchBend

void handleTempoChange(uint32_t microsecondsPerQuarterNote) {

#ifdef DEBUG_SERIAL_OUT
float bpm = 60000000.0f / microsecondsPerQuarterNote;
MYDEBUGPRINT_F_4("[EVT] Tempo Chg: %lu us/qn (%.2f BPM) (Tick: %lu)\n",
microsecondsPerQuarterNote, bpm, midiPlayer.getCurrentTick());
#endif // DEBUG_SERIAL_OUT

} // handleTempoChange

void handleTimeSignature(uint8_t num, uint8_t den_pow2, uint8_t clocks, uint8_t b) {

#ifdef DEBUG_SERIAL_OUT
MYDEBUGPRINT_F_6("[EVT] Time Sig: %u/%u Clocks=%u 32nds/QN=%u (Tick: %lu)\n",
num, (1 << den_pow2), clocks, b, midiPlayer.getCurrentTick());
#endif // DEBUG_SERIAL_OUT

} // handleTimeSignature

void handleEndOfTrack(uint8_t trackIndex) {

#ifdef DEBUG_SERIAL_OUT
MYDEBUGPRINT_F_3("[INF] EndOfTrack reached for track %u (Tick: %lu)\n",
trackIndex, midiPlayer.getCurrentTick());
#endif // DEBUG_SERIAL_OUT

} // handleEndOfTrack

void handlePlaybackComplete() {

#ifdef DEBUG_SERIAL_OUT
MYDEBUGPRINTLN("\n[INF] === Playback Finished ===\n");
#endif // DEBUG_SERIAL_OUT

} // handlePlaybackComplete


void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  MYDEBUGPRINT_F_2("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    MYDEBUGPRINTLN("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    MYDEBUGPRINTLN("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      MYDEBUGPRINT("  DIR : ");
      MYDEBUGPRINTLN(file.name());
      if (levels) { // Recursively list subdirectories if levels > 0
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      MYDEBUGPRINT("  FILE: ");
      MYDEBUGPRINT(file.name());
      MYDEBUGPRINT("  SIZE: ");
      MYDEBUGPRINTLN(file.size());
    }
    file = root.openNextFile();
  }
} // listDir  added


void setup() {
  //Serial.begin(115200);
  Serial.begin(31250);

  //  while (!Serial) delay(10);

//  Serial2.begin(115200, SERIAL_8N1, RX, TX);
#ifdef TARGET_ESP32_KS0413 
  Serial2.begin(115200, SERIAL_8N1, 16, 17); // (BAUD) (BITS) (RXD2 - GPIO-16) (TXD2 - GPIO-17)

#elif defined (TARGET_ESP32_CYD) 
  // used by USB COM Serial2.begin(115200, SERIAL_8N1, 3, 1); // (BAUD) (BITS) (RXD2 - U0RXD - GPIO-3) (TXD2 - U0TXD - GPIO-1)
  // Serial2.begin(115200, SERIAL_8N1, 35, 22); //  CYD connector P3 (BAUD) (BITS) (RXD - GPIO-35) (TXD - GPIO-22)
  Serial2.begin(31250, SERIAL_8N1, 35, 22); //  CYD connector P3 (BAUD) (BITS) (RXD - GPIO-35) (TXD - GPIO-22)

#elif defined (TARGET_T7_S3)
  Serial2.begin(115200, SERIAL_8N1, 47, 48); //  T7_S3 connector D (BAUD) (BITS) (RXD - GPIO-47) (TXD - GPIO-48)

#elif defined (TARGET_XIAO_SAMD21)
  SERIAL_MIDI.begin(31250);
//  SERIAL_MIDI.begin(115200);
//  SERIAL_MIDI.begin(115200, SERIAL_8N1, 7, 6); //  (BAUD) (BITS) (RXD - GPIO-7) (TXD - GPIO-6)
#endif


//  delay(1000);
  delay(2000); //  allow extra time for Windows to register the COM port

  MYDEBUGPRINTLN("Initializing MIDI Player...");

  Serial.flush(); 
  delay(1000); 

  pinMode(SD_CS, OUTPUT); 

  // Enable logging callback (set to info by default)
  // midiPlayer.setLogCallback(handleLog); //  comment added
  // midiPlayer.setLogLevel(MidiLogLevel::INFO); //  comment added

  midiPlayer.setNoteOnCallback(handleNoteOn);
  midiPlayer.setNoteOffCallback(handleNoteOff);
  midiPlayer.setControlChangeCallback(handleControlChange);
  midiPlayer.setProgramChangeCallback(handleProgramChange);
  midiPlayer.setPitchBendCallback(handlePitchBend);
  midiPlayer.setTempoChangeCallback(handleTempoChange);
  midiPlayer.setTimeSignatureCallback(handleTimeSignature);
  midiPlayer.setEndOfTrackCallback(handleEndOfTrack);
  midiPlayer.setPlaybackCompleteCallback(handlePlaybackComplete);

#ifdef FS_LITTLEFS // 
  if (!LittleFS.exists(MIDI_FILE)) {
    MYDEBUGPRINT_F_2("MIDI file %s not found in LittleFS!\n", MIDI_FILE);
    //  while (true) delay(1000);
  }

#elif defined(FS_SD) 
  // Initialize SD card

  if (!SD.begin(SD_CS)) { // SD_CS is the Chip Select pin for your SD card module
    MYDEBUGPRINTLN("SD Card Mount Failed");
    return;
  }
  else{
   // Call the function to list the root directory with a certain level of recursion
  listDir(SD, "/", 0); // Lists root directory, 0 for no recursion into subdirectories. Lists to Serial. 
  }

#endif //  FS

  if (!midiPlayer.load(MIDI_FILE)) {
    MYDEBUGPRINT_F_2("Failed to load MIDI file %s\n", MIDI_FILE);
    while (true) delay(1000);
  }
  else{
      MYDEBUGPRINT_F_2("Successful load MIDI file %s\n", MIDI_FILE);
  }

  MYDEBUGPRINTLN("MIDI Player ready. Commands: play, pause, resume, stop");

  // Serial2.println("play"); //  automatically play the current file ?
  midiPlayer.play();
  MYDEBUGPRINTLN("Playback started");

} // setup  comment added

void handleSerialCommands() {
#if 0 // aaaFIXME
  if (Serial.available() > 0) {
    // aaaFIXME String command = Serial.readStringUntil('\n');
    command.trim();
    
    PlaybackState state = midiPlayer.getState();
    
    if (command.equalsIgnoreCase("play")) {
      if (state != PlaybackState::PLAYING) {
        midiPlayer.play();
        MYDEBUGPRINTLN("Playback started");
      } else {
        MYDEBUGPRINTLN("Already playing");
      }
    }
    else if (command.equalsIgnoreCase("pause")) {
      if (state == PlaybackState::PLAYING) {
        midiPlayer.pause();
        MYDEBUGPRINTLN("Playback paused");
      } else {
        MYDEBUGPRINTLN("Not playing - cannot pause");
      }
    }
    else if (command.equalsIgnoreCase("stop")) {
      if (state == PlaybackState::PLAYING || state == PlaybackState::PAUSED) {
        midiPlayer.stop();
        MYDEBUGPRINTLN("Playback stopped");
      } else {
        MYDEBUGPRINTLN("Already stopped");
      }
    }
    else {
      MYDEBUGPRINTLN("Unknown command. Use: play, pause, resume, stop");
    }
  }
#endif 
} // MYDEBUGPRINT_F_5

void loop() {
  midiPlayer.tick(); // Process MIDI events
  // aaaFIME handleSerialCommands(); // Handle serial input
} // handleSerialCommands

// END_OF_FILE
