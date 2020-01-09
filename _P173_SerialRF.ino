//#ifdef USES_P173
#include <ESPeasySerial.h>

#define PLUGIN_173
#define PLUGIN_ID_173 173
#define PLUGIN_NAME_173 "Communication - Serial RF"
#define PLUGIN_VALUENAME1_173 "RF"
#define SRF_SIG 0XFD
#define SRF_ESIG 0XDF
#define SRF_SIZE 6

ESPeasySerial *P173_easySerial = nullptr;
boolean Plugin_173_init = false;
boolean P173_values_received = false;

void P173_SerialFlush() {
  if (P173_easySerial != nullptr) {
    P173_easySerial->flush();
  }
}

boolean P173_PacketAvailable(void)
{
  if (P173_easySerial != nullptr) // Software serial
  {
    // When there is enough data in the buffer, search through the buffer to
    // find header (buffer may be out of sync)
    if (!P173_easySerial->available()) return false;
    while ((P173_easySerial->peek() != SRF_SIG) && P173_easySerial->available()) {
      P173_easySerial->read(); // Read until the buffer starts with the first byte of a message, or buffer empty.
    }
    if (P173_easySerial->available() < SRF_SIZE) return false; // Not enough yet for a complete packet
  }
  return true;
}

/*
 uint8_t P173_CheckSum(uint8_t *data) {
   uint8_t tempq = 0;
   data += 1;
   for(int j=0; j< (SRF_SIZE - 2); j++) {
     tempq += *data;
     data++;
   }
   tempq = (~tempq) + 1;
   return(tempq);
}
*/

boolean Plugin_173_process_data(struct EventStruct *event) {
  uint8_t data[SRF_SIZE - 1];

  for(int i=0; i < SRF_SIZE; i++) {
    data[i] = P173_easySerial->read();
  }
  if (data[0] !=  SRF_SIG && data[SRF_SIZE -1] != SRF_ESIG) {
    // Not the start of the packet, stop reading.
    return false;
  }

  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    String log = F("SerialRF: Recv ");
    for(int i=0; i < SRF_SIZE; i++ ) {
      log += data[i];
      log += F(" ");
    }
    //log += F("checksum ");
    //log += P173_CheckSum(data);
    addLog(LOG_LEVEL_DEBUG, log);
  }

  //if(P173_CheckSum(data) != data[SRF_SIZE - 1]) {
  //  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
  //    String log = F("SerialRF : checksum error ");
  //    addLog(LOG_LEVEL_DEBUG, log);
  //  }
  //  return false;
  //}

  if (loglevelActiveFor(LOG_LEVEL_DEBUG)) {
    String log = F("SerialRF: RF=");
    log += (data[1] << 16 | data[2] << 8 | data[3]);
    addLog(LOG_LEVEL_DEBUG, log);
  }
  P173_SerialFlush();
  UserVar[event->BaseVarIndex] = data[1] << 16 | data[2] << 8 | data[3];
  P173_values_received = true;
  return true;
}

boolean Plugin_173(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_173;
        Device[deviceCount].Type = DEVICE_TYPE_DUAL;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        success = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_173);
        success = true;
        break;
      }

    case PLUGIN_GET_DEVICEGPIONAMES:
      {
        serialHelper_getGpioNames(event);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_173));
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_LOAD: {
      serialHelper_webformLoad(event);
      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SAVE: {
      serialHelper_webformSave(event);
      success = true;
      break;
    }

    case PLUGIN_INIT:
      {
        int rxPin = CONFIG_PIN1;
        int txPin = CONFIG_PIN2;

        String log = F("Serial RF: config ");
        log += rxPin;
        log += txPin;
        addLog(LOG_LEVEL_DEBUG, log);

        if (P173_easySerial != nullptr) {
          // Regardless the set pins, the software serial must be deleted.
          delete P173_easySerial;
          P173_easySerial = nullptr;
        }

        // Hardware serial is RX on 3 and TX on 1
        if (rxPin == 3 && txPin == 1)
        {
          log = F("Serial RF: using hardware serial");
          addLog(LOG_LEVEL_INFO, log);
        }
        else
        {
          log = F("Serial RF: using software serial");
          addLog(LOG_LEVEL_INFO, log);
        }
        P173_easySerial = new ESPeasySerial(rxPin, txPin, false, 64); // 96 Bytes buffer, enough for up to 3 packets.
        P173_easySerial->begin(9600);
        P173_easySerial->flush();

        Plugin_173_init = true;
        success = true;
        break;
      }

    case PLUGIN_EXIT:
      {
          if (P173_easySerial)
          {
            delete P173_easySerial;
            P173_easySerial=nullptr;
          }
          break;
      }

    // The update rate from the module is 200ms .. multiple seconds. Practise
    // shows that we need to read the buffer many times per seconds to stay in
    // sync.
    case PLUGIN_TEN_PER_SECOND:
      {
        if (Plugin_173_init)
        {
          // Check if a complete packet is available in the UART FIFO.
          if (P173_PacketAvailable())
          {
            addLog(LOG_LEVEL_DEBUG_MORE, F("SerialRF : Packet available"));
            success = Plugin_173_process_data(event);
          }
        }
        break;
      }
    case PLUGIN_READ:
      {
        // When new data is available, return true
        success = P173_values_received;
        P173_values_received = false;
        break;
      }
  }
  return success;
}
//#endif // USES_P173
