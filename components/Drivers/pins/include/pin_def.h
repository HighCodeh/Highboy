#ifndef PIN_DEF_H
#define PIN_DEF_H

// buttons
#define BTN_LEFT     5
#define BTN_BACK     7
#define BTN_UP     15
#define BTN_DOWN   6
#define BTN_OK     4
#define BTN_BACK   7
#define BTN_LEFT 5  
#define BTN_RIGHT 16

#define I2C_MASTER_SDA_IO   8    // Pino para o SDA
#define I2C_MASTER_SCL_IO   9      // Pino para o SCL
#define I2C_MASTER_NUM      I2C_NUM_0 // Porta I2C a usar (a mesma do bq25896.c)
#define I2C_MASTER_FREQ_HZ  100000    // Clock I2C de 100kHz


// led
#define LED_GPIO     45
#define LED_COUNT    1

#endif // !PIN_DEF_H
