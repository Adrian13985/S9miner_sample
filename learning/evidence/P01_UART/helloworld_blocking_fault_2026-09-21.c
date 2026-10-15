#include "platform.h"
#include "xil_printf.h"
#include "sleep.h"
#include "xparameters.h"
#include "xuartps_hw.h"

/*
 * 主循环每隔 10 ms 检查一次串口，并累计到 1000 ms 后打印计数值。
 * 较短的轮询周期可以避免 sleep(1) 导致串口命令最多延迟一秒才响应。
 */
#define POLL_INTERVAL_US  10000U
#define PRINT_INTERVAL_MS 1000U

/* 输出所有可用命令，启动程序或输入 h 时调用。 */
static void print_help(void)
{
    xil_printf("Commands:\r\n");
    xil_printf("  h - show this help\r\n");
    xil_printf("  i - show the current count\r\n");
    xil_printf("  r - reset the count to zero\r\n");
    xil_printf("  q - pause periodic printing\r\n");
    xil_printf("  c - continue periodic printing\r\n");
}

/*
 * 解析并执行一个串口命令。
 * count 指向主函数中的计数变量，因此 r 命令可以直接将其清零；
 * periodic_print_enabled 指向周期打印状态，0 表示暂停，1 表示运行。
 */
static void handle_command(char command, unsigned int *count,
                           int *periodic_print_enabled)
{
    /* 将收到的字符回显到串口，便于确认开发板接收到了什么。 */
    outbyte(command);
    xil_printf("\r\n");

    switch (command)
    {
    case 'h':
    case 'H':
        /* 同时接受大写和小写命令。 */
        print_help();
        break;

    case 'i':
    case 'I':
        /* count 保存下一次周期打印时要输出的计数值。 */
        xil_printf("Current count = %u\r\n", *count);
        break;

    case 'r':
    case 'R':
        /* 通过指针修改 main() 中的 count。 */
        *count = 0U;
        xil_printf("Count reset to 0\r\n");
        break;

    case 'q':
    case 'Q':
        /* 只暂停周期打印，主循环仍继续接收串口命令。 */
        *periodic_print_enabled = 0;
        xil_printf("Periodic printing paused\r\n");
        break;

    case 'c':
    case 'C':
        /* 恢复周期打印。 */
        *periodic_print_enabled = 1;
        xil_printf("Periodic printing continued\r\n");
        break;

    default:
        /* 收到未定义字符时给出提示，避免用户误以为程序没有响应。 */
        xil_printf("Unknown command '%c'. Press h for help.\r\n", command);
        break;
    }
}

int main()
{
    /* count：下一次打印的序号；elapsed_ms：距上次打印经过的时间。 */
    unsigned int count = 0U;
    unsigned int elapsed_ms = 0U;

    /* 状态变量：1 表示允许周期打印，0 表示打印已暂停。 */
    int periodic_print_enabled = 1;

    /* 初始化缓存、串口等板级资源。 */
    init_platform();

    /* 上电后先显示标题、帮助信息和第一条计数信息。 */
    xil_printf("Zynq UART command experiment\r\n");
    print_help();
    xil_printf("Hello Zynq, count = %u\r\n", count++);

    while (1)
    {
        /*
         * inbyte() 是阻塞函数：没有数据时直接调用会让程序一直等待，
         * 周期打印也会随之停止。因此先检查 UART 接收 FIFO，确认有数据
         * 后再调用 inbyte()。while 可一次处理 FIFO 中积压的多个字符。
         */
        /*
         * FAULT INJECTION BEGIN: deliberately bypass the RX FIFO-ready guard.
         * With no received byte, inbyte() will block in XUartPs_RecvByte().
         * Restore the original while condition after recording the experiment.
         * Original: while (XUartPs_IsReceiveData(STDIN_BASEADDRESS))
         */
        {
            char command = inbyte();

            /* 串口终端按 Enter 时通常会附加 CR/LF，这两个字符不作为命令。 */
            if ((command != '\r') && (command != '\n'))
            {
                handle_command(command, &count, &periodic_print_enabled);
            }
        }
        /* FAULT INJECTION END */

        /* 短暂休眠可降低 CPU 空转占用，同时仍保持较快的命令响应。 */
        usleep(POLL_INTERVAL_US);

        if (periodic_print_enabled != 0)
        {
            /* 将微秒轮询周期换算为毫秒并累加。 */
            elapsed_ms += POLL_INTERVAL_US / 1000U;
            if (elapsed_ms >= PRINT_INTERVAL_MS)
            {
                /* 保留超出 1000 ms 的余量，避免长期运行时产生时间漂移。 */
                elapsed_ms -= PRINT_INTERVAL_MS;
                xil_printf("Hello Zynq, count = %u\r\n", count++);
            }
        }
        else
        {
            /* 恢复打印后重新等待完整的一秒，而不是沿用暂停前的余量。 */
            elapsed_ms = 0U;
        }
    }

    /* 当前程序不会退出；保留清理代码以符合平台程序的标准结构。 */
    cleanup_platform();
    return 0;
}
