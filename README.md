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

# Integrated ESP32:
The esp32 model is 'ESP32-3248S035R'

For more info, go to 
- https://www.elektroda.com/rtvforum/topic4058635.html
- https://github.com/jonpul/ESP32-2432S024C
- https://github.com/ardnew/ESP32-3248S035/
- https://www.openhasp.com/0.7.0/hardware/sunton/esp32-3248s035/

# Ideas
- Holiday of the day
- anything of the day
