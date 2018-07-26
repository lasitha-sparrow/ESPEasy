#ifdef USES_P053
//#######################################################################################################
//#################################### Plugin 053: Plantower PMSx003 ####################################
//#######################################################################################################
//
// http://www.aqmd.gov/docs/default-source/aq-spec/resources-page/plantower-pms5003-manual_v2-3.pdf?sfvrsn=2
//
// The PMSx003 are particle sensors. Particles are measured by blowing air through the enclosure and,
// together with a laser, count the amount of particles. These sensors have an integrated microcontroller
// that counts particles and transmits measurement data over the serial connection.


#include <ESPEasySerialReader.h>

#define PLUGIN_053
#define PLUGIN_ID_053 53
#define PLUGIN_NAME_053 "Dust - PMSx003"
#define PLUGIN_VALUENAME1_053 "pm1.0"
#define PLUGIN_VALUENAME2_053 "pm2.5"
#define PLUGIN_VALUENAME3_053 "pm10"
#define PMSx003_SIG1 0X42
#define PMSx003_SIG2 0X4d
#define PMSx003_SIZE 32

boolean Plugin_053_init = false;
boolean values_received = false;

struct P053_SerialReaderStrategy : public SerialReaderStrategy {
  // Use 64 bytes as buffer.
  P053_SerialReaderStrategy() : SerialReaderStrategy(64) {
  }
  virtual void checkForValidPacket() {

  }

};
P053_SerialReaderStrategy P053_strategy;



ESPEasySerialReader P053_SerialReader(P053_strategy);

// Read 2 bytes from serial and make an uint16 of it. Additionally calculate
// checksum for PMSx003. Assumption is that there is data available, otherwise
// this function is blocking.
void SerialRead16(uint16_t* value, uint16_t* checksum)
{
  uint8_t data_high, data_low;
  P053_SerialReader.readPacketByte(data_high);
  P053_SerialReader.readPacketByte(data_low);
  *value = data_low;
  *value |= (data_high << 8);

  if (checksum != NULL)
  {
    *checksum += data_high;
    *checksum += data_low;
  }

#if 0
  // Low-level logging to see data from sensor
  String log = F("PMSx003 : byte high=0x");
  log += String(data_high,HEX);
  log += F(" byte low=0x");
  log += String(data_low,HEX);
  log += F(" result=0x");
  log += String(*value,HEX);
  addLog(LOG_LEVEL_INFO, log);
#endif
}

void SerialFlush() {
  P053_SerialReader.flush();
}

boolean PacketAvailable(void)
{
  return P053_SerialReader.packetAvailable();
}

boolean Plugin_053_process_data(struct EventStruct *event) {
  uint16_t checksum = 0, checksum2 = 0;
  uint16_t framelength = 0;
  uint16 packet_header = 0;
  SerialRead16(&packet_header, &checksum); // read PMSx003_SIG1 + PMSx003_SIG2
  if (packet_header != ((PMSx003_SIG1 << 8) | PMSx003_SIG2)) {
    // Not the start of the packet, stop reading.
    return false;
  }

  SerialRead16(&framelength, &checksum);
  if (framelength != (PMSx003_SIZE - 4))
  {
    if (loglevelActiveFor(LOG_LEVEL_ERROR)) {
      String log = F("PMSx003 : invalid framelength - ");
      log += framelength;
      addLog(LOG_LEVEL_ERROR, log);
    }
    return false;
  }

  uint16_t data[13]; // byte data_low, data_high;
  for (int i = 0; i < 13; i++)
    SerialRead16(&data[i], &checksum);

  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    String log = F("PMSx003 : pm1.0=");
    log += data[0];
    log += F(", pm2.5=");
    log += data[1];
    log += F(", pm10=");
    log += data[2];
    log += F(", pm1.0a=");
    log += data[3];
    log += F(", pm2.5a=");
    log += data[4];
    log += F(", pm10a=");
    log += data[5];
    addLog(LOG_LEVEL_DEBUG, log);
  }

  if (loglevelActiveFor(LOG_LEVEL_DEBUG_MORE)) {
    String log = F("PMSx003 : count/0.1L : 0.3um=");
    log += data[6];
    log += F(", 0.5um=");
    log += data[7];
    log += F(", 1.0um=");
    log += data[8];
    log += F(", 2.5um=");
    log += data[9];
    log += F(", 5.0um=");
    log += data[10];
    log += F(", 10um=");
    log += data[11];
    addLog(LOG_LEVEL_DEBUG_MORE, log);
  }

  // Compare checksums
  SerialRead16(&checksum2, NULL);
  SerialFlush(); // Make sure no data is lost due to full buffer.
  if (checksum == checksum2)
  {
    // Data is checked and good, fill in output
    UserVar[event->BaseVarIndex]     = data[3];
    UserVar[event->BaseVarIndex + 1] = data[4];
    UserVar[event->BaseVarIndex + 2] = data[5];
    values_received = true;
    return true;
  }
  return false;
}

boolean Plugin_053(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_053;
        Device[deviceCount].Type = DEVICE_TYPE_TRIPLE;
        Device[deviceCount].VType = SENSOR_TYPE_TRIPLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 3;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        success = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_053);
        success = true;
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_053));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_053));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_053));
        success = true;
        break;
      }

      case PLUGIN_GET_DEVICEGPIONAMES:
        {
          event->String1 = F("GPIO &larr; TX");
          event->String2 = F("GPIO &rarr; RX");
          event->String3 = F("GPIO &rarr; Reset");
          break;
        }

    case PLUGIN_INIT:
      {
        int rxPin = Settings.TaskDevicePin1[event->TaskIndex];
        int txPin = Settings.TaskDevicePin2[event->TaskIndex];
        int resetPin = Settings.TaskDevicePin3[event->TaskIndex];

        String log = F("PMSx003 : config ");
        log += rxPin;
        log += txPin;
        log += resetPin;
        addLog(LOG_LEVEL_DEBUG, log);

        // Hardware serial is RX on 3 and TX on 1
        if (rxPin == 3 && txPin == 1)
        {
          log = F("PMSx003 : using hardware serial");
          addLog(LOG_LEVEL_INFO, log);
          P053_SerialReader.configureHardwareSerial(ESPEasySerialReader::HardwareSerial0, 9600);
        }
        else
        {
          log = F("PMSx003: using software serial");
          addLog(LOG_LEVEL_INFO, log);
        }

        if (resetPin >= 0) // Reset if pin is configured
        {
          // Toggle 'reset' to assure we start reading header
          log = F("PMSx003: resetting module");
          addLog(LOG_LEVEL_INFO, log);
          pinMode(resetPin, OUTPUT);
          digitalWrite(resetPin, LOW);
          delay(250);
          digitalWrite(resetPin, HIGH);
          pinMode(resetPin, INPUT_PULLUP);
        }

        Plugin_053_init = true;
        success = true;
        break;
      }

    case PLUGIN_EXIT:
      {
        break;
      }

    // The update rate from the module is 200ms .. multiple seconds. Practise
    // shows that we need to read the buffer many times per seconds to stay in
    // sync.
    case PLUGIN_TEN_PER_SECOND:
      {
        if (Plugin_053_init)
        {
          // Check if a complete packet is available in the UART FIFO.
          if (PacketAvailable())
          {
            addLog(LOG_LEVEL_DEBUG_MORE, F("PMSx003 : Packet available"));
            success = Plugin_053_process_data(event);
          }
        }
        break;
      }
    case PLUGIN_READ:
      {
        // When new data is available, return true
        success = values_received;
        values_received = false;
        break;
      }
  }
  return success;
}
#endif // USES_P053
