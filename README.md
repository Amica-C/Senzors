# Vyvojové prostredie pre STM32Cube
Ako základ je použitý RAK 3172 založený na CPU STM32WLE5CC.
<br>
Na všetkých sezoroch a moduloch som musel odpájkovať odpory na SDA/SCL výstupoch - pullup na 3.3V
<br>
Samotný RAK má tiež tieto odpory, 10kOhm, to som tam nechal, ale aby som dostal na zbernicu SDA/SCL 5kOhm, musel som dodatočne pridať ďal paralerne odpory 10kOhm a tak mi to spadlo na požadovaných 5kOhm

## RAK3172: 
- https://www.mikroe.com/lr-14-click
- datasheet RAK: [RAK 3172 datasheet](datasheets/RAK3172_datasheet.pdf)
- datasheet STM32WLEx: [STM32WLEx](datasheets/rm0461-stm32wlex-RAK.pdf)

## podporovane senzory
Temp&Hum 23 Click, SHT4x (I2C)
- https://www.mikroe.com/temphum-23-click
- https://download.mikroe.com/documents/datasheets/SHT45-AD1B-R2_datasheet.pdf

Ambient 21 Click, TSL2591 (I2C)
- https://www.mikroe.com/ambient-21-click
- https://download.mikroe.com/documents/datasheets/TSL2591%20Datasheet.pdf

Barometer 8 click, ILPS22QS (I2C)
- https://www.mikroe.com/barometer-8-click
- https://download.mikroe.com/documents/datasheets/ILPS22QS_datasheet.pdf


## Moduly
Flash 12 Click, AT25EU0041A (SPI)
- https://www.mikroe.com/flash-12-click
- https://download.mikroe.com/documents/datasheets/AT25EU0041A_datasheet.pdf

NFC 4 tag Click, ST25R3916
- https://www.mikroe.com/nfc-tag-4-click
- datasheet ST25R3916: [ST25R3916](datasheets/st25r3916.pdf)
  
