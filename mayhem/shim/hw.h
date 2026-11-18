/* Host stand-ins for the WCH CH582 vendor API. Declarations only; bodies are in shim.c. */
#ifndef MAYHEM_HW_H
#define MAYHEM_HW_H
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint8_t bStatus_t;
typedef uint8_t tmosTaskID;
typedef uint16_t tmosEvents;
typedef uint32_t tmosTimer;
typedef int FunctionalState;

#define __HIGH_CODE
#define __INTERRUPT
#define INVALID_TASK_ID 0xFF
#define SUCCESS 0x00
#define bleInvalidRange 0x18
#define SYS_EVENT_MSG 0x8000
#define ENABLE 1
#define DISABLE 0
#define TRUE 1
#define FALSE 0
#define MS_TO_TMOS(x) ((x) * 1600 / 1000)
#define FREQ_SYS 60000000
#define TMR0_3_IT_CYC_END 1
#define TMR0_IRQn 1
#define TMR3_IRQn 2
#define GPIO_A_IRQn 3
#define GPIO_Pin_All 0xFFFFFFFF

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

void tmos_memcpy(void *dst, const void *src, uint32_t len);
void tmos_memset(void *dst, uint8_t v, uint32_t len);
uint16_t tmos_rand(void);
tmosTaskID TMOS_ProcessEventRegister(uint16_t (*fn)(tmosTaskID, uint16_t));
uint8_t tmos_start_task(tmosTaskID, tmosEvents, uint32_t);
uint8_t tmos_start_reload_task(tmosTaskID, tmosEvents, uint32_t);
uint8_t tmos_stop_task(tmosTaskID, tmosEvents);
uint8_t tmos_set_event(tmosTaskID, tmosEvents);
uint8_t *tmos_msg_receive(tmosTaskID);
uint8_t tmos_msg_deallocate(uint8_t *);
void TMOS_SystemProcess(void);

void SYS_ResetExecute(void);
void SetSysClock(int);
void DelayMs(uint32_t);
void RTC_InitTime(uint16_t y, uint16_t mon, uint16_t d, uint16_t h, uint16_t m, uint16_t s);
void RTC_GetTime(uint16_t *y, uint16_t *mon, uint16_t *d, uint16_t *h, uint16_t *m, uint16_t *s);
void PFIC_EnableIRQ(int);
void LowPower_Shutdown(int);
void Calibration_LSI(int);

/* GPIO, timers, UART, ADC: no-ops on the host */
#define GPIO_Pin_0 (1u << 0)
#define GPIO_Pin_1 (1u << 1)
#define GPIO_Pin_5 (1u << 5)
#define GPIO_Pin_8 (1u << 8)
#define GPIO_Pin_9 (1u << 9)
#define GPIO_Pin_13 (1u << 13)
#define GPIO_Pin_22 (1u << 22)
#define MAX_PACKET_SIZE 64
int GPIOA_ReadPortPin(uint32_t);
int GPIOB_ReadPortPin(uint32_t);
void GPIOA_ModeCfg(uint32_t, int);
void GPIOB_ModeCfg(uint32_t, int);
void GPIOA_SetBits(uint32_t);
void GPIOA_ITModeCfg(uint32_t, int);
void TMR0_TimerInit(uint32_t);
void TMR3_TimerInit(uint32_t);
void TMR0_ITCfg(int, int);
void TMR3_ITCfg(int, int);
int TMR0_GetITFlag(int);
int TMR3_GetITFlag(int);
void TMR0_ClearITFlag(int);
void TMR3_ClearITFlag(int);
void UART1_DefInit(void);
void UART1_BaudRateCfg(uint32_t);
void ADC_ExtSingleChSampInit(int, int);
int16_t ADC_DataCalib_Rough(void);
void ADC_ChannelCfg(int);
uint16_t ADC_ExcutSingleConver(void);
void PWR_PeriphWakeUpCfg(int, int, int);
#define GPIO_ModeIN_Floating 0
#define GPIO_ModeIN_PU 1
#define GPIO_ModeIN_PD 2
#define GPIO_ModeOut_PP_5mA 3
#define GPIO_ITMode_RiseEdge 0
#define GPIO_ITMode_FallEdge 1
#define RB_SLP_GPIO_WAKE 1
#define Long_Delay 0
#define Level_128 0
#define SampleFreq_3_2 0
#define ADC_PGA_0 0
#define CLK_SOURCE_PLL_60MHz 0
#define DEBUG_UART 1

/* Data-Flash (EEPROM), modelled as a 32 KB array: ISP583.h in the vendor tree. */
#define EEPROM_PAGE_SIZE 256
#define EEPROM_BLOCK_SIZE 4096
#define EEPROM_MAX_SIZE 0x8000
#define FLASH_ROM_MAX_SIZE 0x070000
uint32_t shim_eeprom(int op, uint32_t addr, void *buf, uint32_t len);
#define EEPROM_READ(a, b, l) shim_eeprom(0, (a), (b), (l))
#define EEPROM_WRITE(a, b, l) shim_eeprom(1, (a), (void *)(b), (l))
#define EEPROM_ERASE(a, l) shim_eeprom(2, (a), NULL, (l))

/* Code flash (the OTA image slots), modelled as a flat FLASH_ROM_MAX_SIZE array. 0 = success.
 * Read, write and verify copy the caller's buffer inline (statement expressions), so an overrun of
 * the caller's buffer is reported in the calling firmware function, as the ROM routine would read it. */
uint8_t *shim_flash_at(uint32_t addr, uint32_t len);
uint32_t shim_flash_erase(uint32_t addr, uint32_t len);
#define FLASH_ROM_READ(a, b, l) ({ uint32_t l_ = (l); void *b_ = (void *)(b); uint8_t *p_ = shim_flash_at((a), l_); \
	if (p_) memcpy(b_, p_, l_); p_ ? 0 : 1; })
#define FLASH_ROM_WRITE(a, b, l) ({ uint32_t l_ = (l); const void *b_ = (b); uint8_t *p_ = shim_flash_at((a), l_); \
	if (p_) memcpy(p_, b_, l_); p_ ? 0 : 1; })
#define FLASH_ROM_VERIFY(a, b, l) ({ uint32_t l_ = (l); const void *b_ = (b); uint8_t *p_ = shim_flash_at((a), l_); \
	p_ ? memcmp(p_, b_, l_) != 0 : 1; })
#define FLASH_ROM_ERASE(a, l) shim_flash_erase((a), (l))
#define R8_CHIP_ID 0x82
void SYS_DisableAllIrq(void *);
void mDelaymS(uint32_t);

/* src/power.h: reset_jump() is RISC-V asm; same declarations, host body */
#define __RESET_H__
#define CHARGE_STT_PIN GPIO_Pin_0
void shim_reset_jump(void);
static inline void reset_jump(void) { shim_reset_jump(); }
void power_init();
void poweroff();
int batt_raw();
int charging_status();
int batt_raw2percent(int r);
#endif
