#include <Arduino.h>
#include "Tools.h"

void printChipInfo() {
    uint32_t chipId = 0;
    for (int i = 0; i < 17; i = i + 8)
      chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;

    Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
    Serial.printf("This chip has %d cores\n", ESP.getChipCores());
    Serial.print("Chip ID: ");
    Serial.println(chipId);

    Serial.printf(
        "Free Heap: %u bytes\n"
        //"  MALLOC_CAP_8BIT      %7zu bytes\n"
        "  MALLOC_CAP_DMA       %7zu bytes\n"
        "  MALLOC_CAP_SPIRAM    %7zu bytes\n"
        "  MALLOC_CAP_INTERNAL  %7zu bytes\n"
        "  MALLOC_CAP_DEFAULT   %7zu bytes\n"
        "  MALLOC_CAP_IRAM_8BIT %7zu bytes\n"
        "  MALLOC_CAP_RETENTION %7zu bytes\n",
        xPortGetFreeHeapSize(),
        //heap_caps_get_free_size(MALLOC_CAP_8BIT),
        heap_caps_get_free_size(MALLOC_CAP_DMA),
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        heap_caps_get_free_size(MALLOC_CAP_DEFAULT),
        heap_caps_get_free_size(MALLOC_CAP_IRAM_8BIT),
        heap_caps_get_free_size(MALLOC_CAP_RETENTION)
    );    
}

void printMemorySpaceOfAddressEsp32S3(const void* address)
{
    //https://dl.espressif.com/public/esp32s3-mm.pdf
    //https://europe1.discourse-cdn.com/arduino/original/4X/f/3/b/f3b2b1e21fc9f5f7b63b48414fc55f9e302c123c.jpeg
    //https://developer.espressif.com/blog/esp32-memory-map-101/#internal-memories-and-cache-address-space-2

    // External RAM uses the same cache region as the external flash. This means that frequently accessed variables in external RAM can be read and modified almost
    // as quickly as in internal RAM. However, when accessing large chunks of data (> 32 KB), the cache can be insufficient, and speeds will fall back to the access
    // speed of the external RAM. Moreover, accessing large chunks of data can "push out" cached flash, possibly making the execution of code slower afterwards.

    uint32_t iAddress = (uint32_t)address;

    Serial.print(String("\e[35m"));

    Serial.print(iAddress, HEX);
    Serial.print(" ");

    if (0x40000000 <= iAddress && iAddress <= 0x4003FFFF)
        Serial.println("ROM0; 256 KB; via I-bus");
    else
    if (0x3FF00000 <= iAddress && iAddress <= 0x3FF1FFFF)
        Serial.println("ROM1; 128 KB; via D-bus");
    else

    if (0x40370000 <= iAddress && iAddress <= 0x40377FFF)
        Serial.println("SRAM0; 32 KB; via I-bus");
    else
    if (0x40378000 <= iAddress && iAddress <= 0x403DFFFF)
        Serial.println("SRAM1; 320 KB (+ 96 KB ROM-code reserved); via I-bus");
    else
    if (0x3FC88000 <= iAddress && iAddress <= 0x3FCEFFFF)
        Serial.println("SRAM1; 416 KB; via D-bus; DMA access");
    else
    if (0x3FCF0000 <= iAddress && iAddress <= 0x3FCFFFFF)
        Serial.println("SRAM2; 64 KB; via D-bus");
    else

    if (0x50000000 <= iAddress && iAddress <= 0x50001FFF)
        Serial.println("RTCSLOW 8 KB; I-bus");
    else
    if (0x50000000 <= iAddress && iAddress <= 0x50001FFF)
        Serial.println("RTCSLOW 8 KB; D-bus");
    else
    if (0x600FE000 <= iAddress && iAddress <= 0x600FFFFF)
        Serial.println("RTCFAST 8 KB; I-bus");
    else
    if (0x600FE000 <= iAddress && iAddress <= 0x600FFFFF)
        Serial.println("RTCFAST 8 KB; D-bus");

    else
    if (0x42000000 <= iAddress && iAddress <= 0x43FFFFFF)
        Serial.println("Ext. Flash or PSRAM; Cached; I-Cache 32 MB; via I-bus");
    else
    if (0x3C000000 <= iAddress && iAddress <= 0x3DFFFFFF)
        Serial.println("Ext. Flash or PSRAM; Cached; D-Cache 32 MB; via D-bus");

    else
    if (0x60000000 <= iAddress && iAddress <= 0x600D0FFF)
        Serial.println("Peripherals");

    else
        Serial.println("Invalid address");
        
    Serial.print(String("\e[39;49m"));
}
