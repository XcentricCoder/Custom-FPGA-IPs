#include <stdio.h>
#include <stdint.h>
#include "xparameters.h"
#include "xil_io.h"

/*
 * Register map of median_sort AXI4-Lite IP
 *
 * 0x00 : CTRL / STATUS
 *        bit 0 = START
 *        bit 1 = DONE
 *
 * 0x04 : N
 *
 * 0x08 : DATA
 *        Each write stores one array element
 *
 * 0x0C : RESULT
 *        Result = median * 2
 */

#define CTRL_REG    0x00
#define N_REG       0x04
#define DATA_REG    0x08
#define RESULT_REG  0x0C

#define START_BIT   0x01
#define DONE_BIT    0x02

/*
 * IMPORTANT:
 * Check xparameters.h for the exact macro generated
 * for your median_sort IP.
 *
 * It may look like:
 * XPAR_MEDIAN_SORT_0_S00_AXI_BASEADDR
 */
#define MEDIAN_IP_BASE  XPAR_MEDIAN_SORT_V1_0_0_BASEADDR


int main()
{
    int n;
    int32_t value;
    int i;

    uint32_t status;
    int32_t result_x2;

    printf("\r\n");
    printf("=====================================\r\n");
    printf("       AXI4-Lite Median Calculator\r\n");
    printf("=====================================\r\n");

    /*
     * Get number of elements
     */
    printf("Enter number of elements (1-64): ");
    scanf("%d", &n);

    if (n < 1 || n > 64)
    {
        printf("Invalid number of elements.\r\n");
        return 0;
    }

    /*
     * Send N to the IP
     */
    Xil_Out32(MEDIAN_IP_BASE + N_REG, (uint32_t)n);

    /*
     * Send array elements one by one
     */
    printf("Enter %d sorted signed integers:\r\n", n);

    for (i = 0; i < n; i++)
    {
        printf("Element %d: ", i);

        scanf("%ld", (long int *)&value);

        Xil_Out32(MEDIAN_IP_BASE + DATA_REG,
                  (uint32_t)value);
    }

    /*
     * Start median calculation
     */
    Xil_Out32(MEDIAN_IP_BASE + CTRL_REG, START_BIT);

    /*
     * Wait until DONE = 1
     */
    do
    {
        status = Xil_In32(MEDIAN_IP_BASE + CTRL_REG);
    }
    while ((status & DONE_BIT) == 0);

    /*
     * Read result.
     *
     * Hardware stores:
     *
     *     RESULT = median * 2
     *
     * This allows us to represent .5 for even-sized arrays.
     */
    result_x2 = (int32_t)Xil_In32(MEDIAN_IP_BASE + RESULT_REG);

    /*
     * Print median
     */
    if (result_x2 % 2 == 0)
    {
        printf("\r\nMedian = %ld\r\n",
               (long int)(result_x2 / 2));
    }
    else
    {
        if (result_x2 < 0)
        {
            printf("\r\nMedian = -%ld.5\r\n",
                   (long int)((-result_x2) / 2));
        }
        else
        {
            printf("\r\nMedian = %ld.5\r\n",
                   (long int)(result_x2 / 2));
        }
    }

    printf("=====================================\r\n");

    return 0;
}
