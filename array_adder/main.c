
#include "xparameters.h"
#include "xaxidma.h"
#include "xil_printf.h"
#include "xil_cache.h"
#include "sleep.h"

#define ARRAY_SIZE 16

/*
 * Change this macro if your AXI DMA instance
 * has a different name in xparameters.h.
 */
#define DMA_DEVICE_ID XPAR_AXIDMA_0_DEVICE_ID


/* ============================================================
 * Global buffers
 *
 * Each array element occupies one 32-bit word because our
 * AXI4-Stream TDATA is 32 bits wide.
 *
 * Only bits [7:0] contain the actual 8-bit array element.
 * ============================================================ */

static u32 A[ARRAY_SIZE] __attribute__((aligned(64)));
static u32 B[ARRAY_SIZE] __attribute__((aligned(64)));
static u32 C[ARRAY_SIZE] __attribute__((aligned(64)));


/* DMA instance */
XAxiDma AxiDma;


/* ============================================================
 * Main
 * ============================================================ */

int main()
{
    int Status;
    int i;

    xil_printf("\r\n");
    xil_printf("========================================\r\n");
    xil_printf("       ARRAY ADDITION TEST\r\n");
    xil_printf("========================================\r\n");


    /* ========================================================
     * Initialize AXI DMA
     * ======================================================== */

    XAxiDma_Config *CfgPtr;

    CfgPtr = XAxiDma_LookupConfig(DMA_DEVICE_ID);

    if (CfgPtr == NULL) {
        xil_printf("ERROR: DMA configuration not found\r\n");
        return XST_FAILURE;
    }

    Status = XAxiDma_CfgInitialize(&AxiDma, CfgPtr);

    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: DMA initialization failed\r\n");
        return XST_FAILURE;
    }

    xil_printf("DMA initialized successfully\r\n");


    /* ========================================================
     * Make sure Scatter Gather mode is disabled
     * ======================================================== */

    if (XAxiDma_HasSg(&AxiDma)) {
        xil_printf("ERROR: DMA is configured in Scatter-Gather mode\r\n");
        return XST_FAILURE;
    }


    /* ========================================================
     * Initialize input arrays
     *
     * A = 1,2,3,...16
     *
     * B = 10,20,30,...160
     * ======================================================== */

    for (i = 0; i < ARRAY_SIZE; i++) {

        A[i] = i + 1;

        B[i] = (i + 1) * 10;

        C[i] = 0;
    }


    /* ========================================================
     * Print input arrays
     * ======================================================== */

    xil_printf("\r\nArray A:\r\n");

    for (i = 0; i < ARRAY_SIZE; i++) {
        xil_printf("%d ", A[i] & 0xFF);
    }

    xil_printf("\r\n");

    xil_printf("\r\nArray B:\r\n");

    for (i = 0; i < ARRAY_SIZE; i++) {
        xil_printf("%d ", B[i] & 0xFF);
    }

    xil_printf("\r\n");


    /* ========================================================
     * Flush CPU cache before DMA accesses memory
     * ======================================================== */

    Xil_DCacheFlushRange(
        (UINTPTR)A,
        sizeof(A)
    );

    Xil_DCacheFlushRange(
        (UINTPTR)B,
        sizeof(B)
    );

    Xil_DCacheFlushRange(
        (UINTPTR)C,
        sizeof(C)
    );


    /* ========================================================
     * IMPORTANT:
     *
     * First configure the S2MM channel.
     *
     * This prepares the DMA to receive the result from
     * add_array before we start sending the input data.
     * ======================================================== */

    Status = XAxiDma_SimpleTransfer(
        &AxiDma,
        (UINTPTR)C,
        sizeof(C),
        XAXIDMA_DEVICE_TO_DMA
    );

    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: S2MM transfer setup failed\r\n");
        return XST_FAILURE;
    }


    /* ========================================================
     * Send Array A
     *
     * One uint32_t = one AXI Stream beat.
     *
     * sizeof(A) = 16 * 4 = 64 bytes
     *
     * Therefore DMA generates:
     *
     * A0 A1 A2 ... A15 TLAST
     * ======================================================== */

    Status = XAxiDma_SimpleTransfer(
        &AxiDma,
        (UINTPTR)A,
        sizeof(A),
        XAXIDMA_DMA_TO_DEVICE
    );

    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: Array A transfer failed\r\n");
        return XST_FAILURE;
    }


    /* ========================================================
     * Wait until Array A transfer is complete
     * ======================================================== */

    while (XAxiDma_Busy(
            &AxiDma,
            XAXIDMA_DMA_TO_DEVICE)) {
    }


    xil_printf("\r\nArray A sent successfully\r\n");


    /* ========================================================
     * Send Array B
     *
     * This creates the second AXI Stream packet:
     *
     * B0 B1 B2 ... B15 TLAST
     * ======================================================== */

    Status = XAxiDma_SimpleTransfer(
        &AxiDma,
        (UINTPTR)B,
        sizeof(B),
        XAXIDMA_DMA_TO_DEVICE
    );

    if (Status != XST_SUCCESS) {
        xil_printf("ERROR: Array B transfer failed\r\n");
        return XST_FAILURE;
    }


    /* ========================================================
     * Wait until Array B transfer is complete
     * ======================================================== */

    while (XAxiDma_Busy(
            &AxiDma,
            XAXIDMA_DMA_TO_DEVICE)) {
    }


    xil_printf("Array B sent successfully\r\n");


    /* ========================================================
     * Wait for result transfer
     * ======================================================== */

    while (XAxiDma_Busy(
            &AxiDma,
            XAXIDMA_DEVICE_TO_DMA)) {
    }


    /* ========================================================
     * Invalidate cache so CPU sees DMA-written data
     * ======================================================== */

    Xil_DCacheInvalidateRange(
        (UINTPTR)C,
        sizeof(C)
    );


    /* ========================================================
     * Print results
     * ======================================================== */

    xil_printf("\r\nResult Array C:\r\n");

    for (i = 0; i < ARRAY_SIZE; i++) {

        xil_printf("%d ", C[i] & 0xFF);

    }

    xil_printf("\r\n");


    /* ========================================================
     * Verify results
     * ======================================================== */

    for (i = 0; i < ARRAY_SIZE; i++) {

        u32 expected;

        expected = ((A[i] & 0xFF) + (B[i] & 0xFF)) & 0xFF;

        if ((C[i] & 0xFF) != expected) {

            xil_printf(
                "\r\nERROR: C[%d] = %d, expected %d\r\n",
                i,
                C[i] & 0xFF,
                expected
            );

            xil_printf("\r\nTEST FAILED\r\n");

            return XST_FAILURE;
        }
    }


    /* ========================================================
     * Test passed
     * ======================================================== */

    xil_printf("\r\n");
    xil_printf("========================================\r\n");
    xil_printf("          TEST PASSED\r\n");
    xil_printf("========================================\r\n");


    return XST_SUCCESS;
}

