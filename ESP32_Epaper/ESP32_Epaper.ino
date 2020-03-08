/////////////////////////////////////////////////////////////////
//             Leah's Dashboard Display Module                 //
/////////////////////////////////////////////////////////////////
#include "Dashboard.h"
#include "CO_driver.h"
#include "CANopen.h"
#include "driver/timer.h"
#include <EEPROM.h>


// include library to read and write from flash memory
#define EEPROM_SIZE 100

#define TMR_TASK_INTERVAL   (1000)          /* Interval of tmrTask thread in microseconds */
#define INCREMENT_1MS(var)  (var++)         /* Increment 1ms variable in tmrTask */

#define TIMER_INTR_SEL TIMER_INTR_LEVEL  /*!< Timer level interrupt */
#define TIMER_DIVIDER   80               /*!< Hardware timer clock divider, 80 to get 1MHz clock to timer */
#define TIMER_SCALE    (TIMER_BASE_CLK / TIMER_DIVIDER)  /*!< used to calculate counter value BASE CLK is 80MHz */
#define TIMER_FINE_ADJ   (0*(TIMER_BASE_CLK / TIMER_DIVIDER)/1000000) /*!< used to compensate alarm value */
#define TIMER_INTERVAL0_SEC   (0.001)    /*!< test interval for timer 0 CANopen */
#define TIMER_INTERVAL1_SEC   (0.1)      /*!< test interval for timer 1 I2C */

TaskHandle_t DashboardTask;
TaskHandle_t tmrTask_thread1;

//User-defined CAN base structure, passed as argument to CO_init.

struct CANbase {
  uintptr_t baseAddress;  /**< Base address of the CAN module */
};

/* Global variables and objects */
volatile uint16_t   CO_timer1ms = 0U;   /* variable increments each millisecond */


CO_NMT_reset_cmd_t reset;
uint16_t timer1msPrevious;

static bool test_set_bits;
static bool test_clear_bits;

/* CAN interrupt function *****************************************************/

void setup() {
  reset = CO_RESET_NOT;
  xTaskCreatePinnedToCore(&DashboardTask1, "DashTask_thread", 10000, NULL, 1, &DashboardTask, 0);
  /* initialize EEPROM */
  // initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);
  Serial.begin(115200);
  while (reset != CO_RESET_APP) {
    /* CANopen communication reset - initialize CANopen objects *******************/
    CO_ReturnError_t err;


    /* disable CAN and CAN interrupts */
    struct CANbase canBase = {
      .baseAddress = 0u,  /* CAN module address */
    };

    /* initialize CANopen */
    err = CO_init(&canBase, 8/* NodeID */, 125 /* bit rate */);
    if (err != CO_ERROR_NO) {
      while (1);
      /* CO_errorReport(CO->em, CO_EM_MEMORY_ALLOCATION_ERROR, CO_EMC_SOFTWARE_INTERNAL, err); */
    }
    /* Configure CAN transmit and receive interrupt */
    Serial.println("Before start can");
    /* start CAN */
    CO_CANsetNormalMode(CO->CANmodule[0]);

    reset = CO_RESET_NOT;
    timer1msPrevious = CO_timer1ms;
    xTaskCreatePinnedToCore(&tmrTask_thread,"tmrTask_thread",10000,NULL,1,&tmrTask_thread1,0);


    while (reset == CO_RESET_NOT) {
      /* loop for normal program execution ******************************************/
      uint16_t timer1msCopy, timer1msDiff;

      timer1msCopy = CO_timer1ms;
      timer1msDiff = timer1msCopy - timer1msPrevious;
      timer1msPrevious = timer1msCopy;

      /* CANopen process */
      reset = CO_process(CO, timer1msDiff, NULL);

      /* Nonblocking application code may go here. */


      /* Process EEPROM */
    }

  }
}

void loop() {
  delay(500);
}

void DashboardTask1( void * parameter )
{
  Dashboard dashboard;
  for (;;) {
    Serial.println("task0");
    dashboard.startDashboard();
  }
}



/* timer thread executes in constant intervals ********************************/
static void tmrTask_thread(void * parameter ) {

  for (;;) {

    /* sleep for interval */

    INCREMENT_1MS(CO_timer1ms);

    if (CO->CANmodule[0]->CANnormal) {
      bool_t syncWas;

      CO_CANinterrupt(CO->CANmodule[0]);

      /* Process Sync */
      syncWas = CO_process_SYNC(CO, TMR_TASK_INTERVAL);

      /* Read inputs */
      CO_process_RPDO(CO, syncWas);

      /* Further I/O or nonblocking application code may go here. */

      /* Write outputs */
      CO_process_TPDO(CO, syncWas, TMR_TASK_INTERVAL);

      /* verify timer overflow */
      if (0) {
        CO_errorReport(CO->em, CO_EM_ISR_TIMER_OVERFLOW, CO_EMC_SOFTWARE_INTERNAL, 0U);
      }
    }
    delay(1);
  }
}
