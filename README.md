# Getting screen to work
1. Install the TFT_eSPI library on arduino ide
2. Go to the TFT\_eSPI folder (Arduino/libraries/TFT\_eSPI) on device
3. Move the files in the github TFT\_eSPI folder to other folder. Replace if asked
4. Edit the 'User\_Setup\_Select.h' and uncomment include line that corresponds to used screen. You need to comment all other includes.
### Example:
```
#include <User_Setups/Setup666_Integrated_ESP32.h>
//#include <User_Setups/Setup667_OSOYOO_SPI_RPI_35.h>
```

If we were using the osoyoo 3.5" display for RPI, we need to uncomment and comment:

```
//#include <User_Setups/Setup666_Integrated_ESP32.h>
#include <User_Setups/Setup667_OSOYOO_SPI_RPI_35.h>
```
# how to use json:
- when getting JsonDocument data, you need to convert it to its type *first* and then use it, so basically:
```
float test = weatherAPI.doc["latitude"];
Serial.println(test);
//test will work 
```
- see https://arduinojson.org/v6/assistant/ to see how to deal with json data from apis.
- JsonArray represents regular C arrays from the json. JsonObjects represent regular dictionaries.

```
JsonArray daily_temperature_2m_max = daily["temperature_2m_max"];
float daily_temperature_2m_max_0 = daily_temperature_2m_max[0]; // 73.5
float daily_temperature_2m_max_1 = daily_temperature_2m_max[1]; // 68.9

JsonObject current = doc["current"];
const char* current_time = current["time"]; // "2025-05-27T16:00"
int current_interval = current["interval"]; // 900
```

# Random Links:
The esp32 model is 'ESP32-3248S035R'

For more info, go to 
- https://randomnerdtutorials.com/arduino-ide-2-install-esp32-littlefs/
- https://bytesnbits.co.uk/arduino-touchscreen-calibration-coding/#1606910525307-c15b5275-547b
- https://www.elektroda.com/rtvforum/topic4058635.html
- https://github.com/jonpul/ESP32-2432S024C
- https://github.com/ardnew/ESP32-3248S035/
- https://www.openhasp.com/0.7.0/hardware/sunton/esp32-3248s035/
- https://gist.github.com/akeaswaran/b48b02f1c94f873c6655e7129910fc3b

# Ideas & Class structure
- Holiday of the day
- anything of the day
- use calibrate code to get offset first, then manually add to code
- website where they can edit stuff :)‽‽‽‽‽‽‽‽‽‽
