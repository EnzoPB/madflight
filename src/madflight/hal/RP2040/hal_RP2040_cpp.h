
//Arduino version string
#define HAL_ARDUINO_STR "Arduino-Pico v" ARDUINO_PICO_VERSION_STR 

#ifndef HAL_MCU
  #ifdef PICO_RP2350
    #define HAL_MCU "RP2350"
  #else
    #define HAL_MCU "RP2040"
  #endif
#endif

//======================================================================================================================//
//                    IMU
//======================================================================================================================//
#ifndef IMU_EXEC
  #define IMU_EXEC IMU_EXEC_FREERTOS_OTHERCORE
#endif
#ifndef IMU_FREERTOS_TASK_PRIORITY
  #define IMU_FREERTOS_TASK_PRIORITY 7
#endif

//======================================================================================================================//
//                    hal_setup()
//======================================================================================================================//

//-------------------------------------
//Include Libraries
//-------------------------------------
#include <Wire.h> //I2C communication
#include <SPI.h> //SPI communication
#include "RP2040_PWM.h"  //Servo and oneshot
#include "RP2040_SerialIRQ.h"  //Replacement high performance serial driver
#include "../MF_Serial.h"
#include "../MF_I2C.h"

//-------------------------------------
//Bus Setup
//-------------------------------------

#define HAL_SER_NUM 2
#define HAL_I2C_NUM 2
#define HAL_SPI_NUM 2

MF_I2C    *hal_i2c[HAL_I2C_NUM] = {};
MF_Serial *hal_ser[HAL_SER_NUM] = {};
SPIClass  *hal_spi[HAL_SPI_NUM] = {};

//prototype
void hal_eeprom_begin();

uint8_t ser0_txbuf[256];
uint8_t ser0_rxbuf[256];
uint8_t ser1_txbuf[256];
uint8_t ser1_rxbuf[256];

void hal_setup() {
  //Serial BUS
  if(cfg.pin_ser0_tx >= 0 && cfg.pin_ser0_rx >= 0) {
    //uncomment one: SerialIRQ, SerialUART or SerialPIO and use uart0 or uart1
    auto *ser = new SerialIRQ(uart0, cfg.pin_ser0_tx, ser0_txbuf, sizeof(ser0_txbuf), cfg.pin_ser0_rx, ser0_rxbuf, sizeof(ser0_rxbuf));
    //auto *ser = new SerialIRQ(uart0, cfg.pin_ser0_tx, pin_ser0_rx, 256, 256); //TODO
    //auto *ser = new SerialDMA(uart0, cfg.pin_ser0_tx, pin_ser0_rx, 256, 256); //TODO
    //auto *ser = new SerialUART(uart0, cfg.pin_ser0_tx, pin_ser0_rx); //SerialUART default Arduino impementation (had some problems with this)
    //auto *ser = new SerialPIO(cfg.pin_ser0_tx, pin_ser0_rx, 32); //PIO uarts, any pin allowed (not tested, but expect same as SerialUART)
    hal_ser[0] = new MF_SerialPtrWrapper<decltype(ser)>( ser );
  }
  if(cfg.pin_ser1_tx >= 0 && cfg.pin_ser1_rx >= 0) {
    //uncomment one: SerialIRQ, SerialUART or SerialPIO and use uart0 or uart1
    auto *ser = new SerialIRQ(uart1, cfg.pin_ser1_tx, ser1_txbuf, sizeof(ser1_txbuf), cfg.pin_ser1_rx, ser1_rxbuf, sizeof(ser1_rxbuf));
    //auto *ser = new SerialIRQ(uart1, cfg.pin_ser1_tx, pin_ser1_rx, 256, 256); //TODO
    //auto *ser = new SerialDMA(uart1, cfg.pin_ser1_tx, pin_ser1_rx, 256, 256); //TODO
    //auto *ser = new SerialUART(uart1, cfg.pin_ser1_tx, pin_ser1_rx); //SerialUART default Arduino impementation (had some problems with this)
    //auto *ser = new SerialPIO(cfg.pin_ser1_tx, pin_ser1_rx, 32); //PIO uarts, any pin allowed (not tested, but expect same as SerialUART)
    hal_ser[1] = new MF_SerialPtrWrapper<decltype(ser)>( ser );
  }

  //I2C BUS
  if(cfg.pin_i2c0_sda >= 0 && cfg.pin_i2c0_scl >= 0) {
    auto *i2c = &Wire; //type is TwoWire
    i2c->setSDA(cfg.pin_i2c0_sda);
    i2c->setSCL(cfg.pin_i2c0_scl);
    i2c->setClock(1000000);
    i2c->setTimeout(25, true); //timeout, reset_with_timeout
    i2c->begin();
    hal_i2c[0] = new MF_I2CPtrWrapper<decltype(i2c)>( i2c );
  }
  if(cfg.pin_i2c1_sda >= 0 && cfg.pin_i2c1_scl >= 0) {
    auto *i2c = &Wire1; //type is TwoWire
    i2c->setSDA(cfg.pin_i2c1_sda);
    i2c->setSCL(cfg.pin_i2c1_scl);
    i2c->setClock(1000000);
    i2c->setTimeout(25, true); //timeout, reset_with_timeout
    i2c->begin();
    hal_i2c[1] = new MF_I2CPtrWrapper<decltype(i2c)>( i2c );
  }

  //SPI BUS
  if(cfg.pin_spi0_miso >= 0 && cfg.pin_spi0_mosi >= 0 && cfg.pin_spi0_sclk >= 0) {
    hal_spi[0] = new SPIClassRP2040(spi0, cfg.pin_spi0_miso, -1, cfg.pin_spi0_sclk, cfg.pin_spi0_mosi);
    hal_spi[0]->begin();
  }
  if(cfg.pin_spi1_miso >= 0 && cfg.pin_spi1_mosi >= 0 && cfg.pin_spi1_sclk >= 0) {
    hal_spi[1] = new SPIClassRP2040(spi1, cfg.pin_spi1_miso, -1, cfg.pin_spi1_sclk, cfg.pin_spi1_mosi);
    hal_spi[1]->begin();
  }

  hal_eeprom_begin();

  //IMU
  if(cfg.pin_imu_int >= 0) {
    pinMode(cfg.pin_imu_int, INPUT); //apparently needed for RP2350, should not hurt for RP2040
  }
}

//======================================================================================================================//
//  EEPROM
//======================================================================================================================//
#include <EEPROM.h>

//#define DEBUG_EEPROM

void hal_eeprom_begin() {
  #ifdef DEBUG_EEPROM
    Serial.printf("hal_eeprom_begin()\n");
  #endif 
  EEPROM.begin(4096);
}

uint8_t hal_eeprom_read(uint32_t adr) {
  uint8_t val =EEPROM.read(adr);
  #ifdef DEBUG_EEPROM
    Serial.printf("EEr %04X:%02X\n", adr, val);
  #endif  
  return val;
}

void hal_eeprom_write(uint32_t adr, uint8_t val) {
  #ifdef DEBUG_EEPROM
    Serial.printf("EEw %04X:%02X\n", adr, val);
  #endif
  EEPROM.write(adr, val);
}

void hal_eeprom_commit() {
  #ifdef DEBUG_EEPROM
    Serial.printf("hal_eeprom_commit()\n");
  #endif 
  EEPROM.commit();
}


//======================================================================================================================//
//  MISC
//======================================================================================================================//

void hal_reboot() {
  //does not always work...
  taskENTER_CRITICAL(); //stop FreeRtos scheduler
  watchdog_enable(10, false); //uint32_t delay_ms, bool pause_on_debug
  while(1){}

  //does not always work...
  //taskENTER_CRITICAL(); //stop FreeRtos scheduler
  //watchdog_reboot(0, 0, 10); //uint32_t pc, uint32_t sp, uint32_t delay_ms
  //while(1);

  //alternate method - does not work...
  //#define AIRCR_Register (*((volatile uint32_t*)(PPB_BASE + 0x0ED0C)))
  //AIRCR_Register = 0x5FA0004;
}

inline uint32_t hal_get_core_num() {
  return get_core_num();
}

int hal_get_pin_number(String val) {
  return val.toInt();
}

void hal_print_pin_name(int pinnum) {
  Serial.printf("%d",pinnum);
}
