// #include <stdio.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <sys/mman.h>
// #include "hwlib.h"
// #include "socal/socal.h"
// #include "socal/hps.h"
// #include "socal/alt_gpio.h"
// #include "hps_0.h"
// #define HW_REGS_BASE ( ALT_STM_OFST )
// #define HW_REGS_SPAN ( 0x04000000 )
// #define HW_REGS_MASK ( HW_REGS_SPAN - 1 )
// int main() {
//  void *virtual_base;
//  int fd;
//  int loop_count;
//  int led_direction;
//  int led_mask;
//  void *h2p_lw_led_addr;
//  if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
//  printf( "ERROR: could not open \"/dev/mem\"...\n" );
//  return( 1 );
//  }
//  virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ),
//  MAP_SHARED, fd, HW_REGS_BASE );
//  if( virtual_base == MAP_FAILED ) {
//  printf( "ERROR: mmap() failed...\n" );
//  close( fd );
//  return( 1 );
//  }
//  h2p_lw_led_addr = virtual_base +
//  ( ( unsigned long )( ALT_LWFPGASLVS_OFST + PIO_LED_BASE )
//  & ( unsigned long )( HW_REGS_MASK ) );
//  loop_count = 0;
//  led_mask = 0x01;
//  led_direction = 0; // 0: left to right direction
//  while( loop_count < 60 ) {
//  *(uint32_t *) h2p_lw_led_addr = ~led_mask;
//  usleep( 100*1000 );
//  if (led_direction == 0){
//  led_mask <<= 1;
//  if (led_mask == (0x01 << (PIO_LED_DATA_WIDTH-1)))
//  led_direction = 1;
//  } else {
//  led_mask >>= 1;
//  if (led_mask == 0x01){
//  led_direction = 0;
//  loop_count++;
//  }
//  }
//  } // while
//  if( munmap( virtual_base, HW_REGS_SPAN ) != 0 ) {
//  printf( "ERROR: munmap() failed...\n" );
//  close( fd );
//  return( 1 );
//  }
//  close( fd );
//  return( 0 );
// }
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include "hwlib.h"
#include "socal/socal.h"
#include "socal/hps.h"
#include "socal/alt_gpio.h"
#include "hps_0.h"

// Стандартний базовий адрес Lightweight HPS-to-FPGA мосту
#define HW_REGS_BASE ( ALT_LWFPGASLVS_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )

int main() {
    void *virtual_base;
    int fd;
    
    volatile uint32_t *h2p_lw_led_addr = NULL;
    volatile uint32_t *h2p_lw_sw_addr = NULL;
    uint32_t sw_state;

    // Відкриваємо доступ до фізичної пам'яті
    if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
        printf("ERROR: could not open \"/dev/mem\"...\n");
        return 1;
    }

    // Відображаємо периферію HPS/FPGA в віртуальний адресний простір
    virtual_base = mmap(NULL, HW_REGS_SPAN, (PROT_READ | PROT_WRITE),
                        MAP_SHARED, fd, HW_REGS_BASE);

    if (virtual_base == MAP_FAILED) {
        printf("ERROR: mmap() failed...\n");
        close(fd);
        return 1;
    }

    // Обчислюємо віртуальні адреси для LED та Перемикачів (SW)
    h2p_lw_led_addr = (volatile uint32_t *)(virtual_base + 
        (((unsigned long)(ALT_LWFPGASLVS_OFST + PIO_LED_BASE)) & (unsigned long)(HW_REGS_MASK)));

    h2p_lw_sw_addr = (volatile uint32_t *)(virtual_base + 
        (((unsigned long)(ALT_LWFPGASLVS_OFST + PIO_SW_BASE)) & (unsigned long)(HW_REGS_MASK)));

    printf("Програма запущена. Натисніть Ctrl+C для виходу.\n");

    // Нескінченний цикл зчитування та відображення
    while (1) {
        // 1. Зчитуємо поточний стан перемикачів
        sw_state = *h2p_lw_sw_addr;

        // 2. Інвертуємо значення (побітове НІ) та обрізаємо за шириною шини світлодіодів
        // Враховуємо активний рівень світлодіодів та маскуємо лишні біти
        *(h2p_lw_led_addr) = (~sw_state) & ((1 << PIO_LED_DATA_WIDTH) - 1);

        // Невелика затримка (10 мс) для зменшення навантаження на процесор
        usleep(10 * 1000);
    }

    // Очищення ресурсів (якщо вихід з циклу буде передбачений)
    if (munmap(virtual_base, HW_REGS_SPAN) != 0) {
        printf("ERROR: munmap() failed...\n");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}