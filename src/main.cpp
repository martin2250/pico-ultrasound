#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "chirp.pio.h"

#include "chirp_buffer.h"

#include <stdio.h>



static PIO  pio_chirp;
static uint sm_chirp;

static uint dma_chirp;
static uint dma_adc;

static int16_t adc_buffer_i[CHIRP_LENGTH];
static int16_t adc_buffer_q[CHIRP_LENGTH];

static volatile uint32_t pio_count = 0;

static void pio_irq_handler() {
    pio_interrupt_clear(pio_chirp, 0);

    int16_t value = adc_read();

    // sequence: +I +Q -I -Q
    bool is_q = (pio_count & 1) != 0;
    int16_t *buffer = is_q ? adc_buffer_q : adc_buffer_i;

    bool is_negative = (pio_count & 2) != 0;
    value = is_negative ? -value : value;

    // 8 cycles of oversampling and 4 samples each
    buffer[pio_count / 32] += value;

    pio_count += 1;
}

static void core1() {
    irq_set_exclusive_handler(PIO0_IRQ_0, pio_irq_handler);
    irq_set_enabled(PIO0_IRQ_0, true);

    while (true) {
        __wfi();
    }
}

int main() {
    // clock_configure(clk_sys,
    //             CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
    //             CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
    //             256 * MHZ,
    //             256 * MHZ);

    set_sys_clock_khz(256000, true);
    stdio_init_all();

    printf("starting\n");

    multicore_launch_core1(core1);

    adc_init();
    adc_select_input(0);
    adc_fifo_setup(
        true,    // Write each completed conversion to the sample FIFO
        true,    // Enable DMA data request (DREQ)
        1,       // DREQ (and IRQ) asserted when at least 1 sample present
        false,   // We won't see the ERR bit because of 8 bit reads; disable.
        false     // dont Shift each sample to 8 bits when pushing to FIFO
    );
    adc_set_clkdiv(0);
    
    dma_chirp = dma_claim_unused_channel(true);
    dma_adc = dma_claim_unused_channel(true);

    pio_chirp = pio0;
    sm_chirp = 0;
    uint offset = pio_add_program(pio_chirp, &chirp_program);
    chirp_program_init(pio_chirp, sm_chirp, offset, 4);
    
    pio_set_irq0_source_enabled(pio_chirp, pis_interrupt0, true); 

    while (true) {
        pio_count = 0;
        for (size_t i = 0; i < CHIRP_LENGTH; i++) {
            adc_buffer_i[i] = 0;
            adc_buffer_q[i] = 0;
        }

        {
            dma_channel_abort(dma_chirp);
            dma_channel_config c = dma_channel_get_default_config(dma_chirp);
            channel_config_set_dreq(&c, pio_get_dreq(pio_chirp, sm_chirp, true));
            channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
            channel_config_set_write_increment(&c, false);
            channel_config_set_read_increment(&c, true);
            dma_channel_configure(
                dma_chirp, &c,
                &pio_chirp->txf[sm_chirp],
                chirp_buffer,
                CHIRP_LENGTH,
                true
            );
        }

        dma_channel_wait_for_finish_blocking(dma_chirp);

        sleep_ms(20);

        printf("pio count: %d\n", pio_count);
        printf("data i:\n");
        for (size_t i = 0; i < CHIRP_LENGTH; i++) {
            printf("%02X", ((uint16_t)adc_buffer_i[i]) & 0xff);
            printf("%02X", ((uint16_t)adc_buffer_i[i]) >> 8);
        }
        printf("\ndata q:\n");
        for (size_t i = 0; i < CHIRP_LENGTH; i++) {
            printf("%02X", ((uint16_t)adc_buffer_q[i]) & 0xff);
            printf("%02X", ((uint16_t)adc_buffer_q[i]) >> 8);
        }
        printf("\nend:\n");
    }

}
