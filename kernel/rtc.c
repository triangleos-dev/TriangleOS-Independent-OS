#include "rtc.h"
#include "io.h"

#define CMOS_INDEX 0x70
#define CMOS_DATA  0x71

#define RTC_SECOND 0x00
#define RTC_MINUTE 0x02
#define RTC_HOUR   0x04
#define RTC_DAY    0x07
#define RTC_MONTH  0x08
#define RTC_YEAR   0x09

#define RTC_STATUS_A 0x0A
#define RTC_STATUS_B 0x0B
#define RTC_CENTURY  0x32


static unsigned char rtc_read_register(
    unsigned char reg
)
{
    /*
     * Bit 7 of the CMOS index disables NMI.
     * Keep it set while selecting the register.
     */
    outb(CMOS_INDEX, reg | 0x80);

    return inb(CMOS_DATA);
}


static unsigned char bcd_to_binary(
    unsigned char value
)
{
    return (unsigned char)(
        (value & 0x0F) +
        ((value >> 4) * 10)
    );
}


static void rtc_wait_for_update(void)
{
    /*
     * Status Register A bit 7:
     *
     * 1 = RTC update in progress
     * 0 = safe to read
     */
    while (rtc_read_register(RTC_STATUS_A) & 0x80)
    {
        __asm__ volatile ("pause");
    }
}


static unsigned char rtc_read_binary_value(
    unsigned char reg,
    unsigned char status_b
)
{
    unsigned char value =
        rtc_read_register(reg);

    /*
     * Bit 2 of Status B:
     *
     * 0 = BCD
     * 1 = binary
     */
    if (!(status_b & 0x04))
        value = bcd_to_binary(value);

    return value;
}


void rtc_init(void)
{
    /*
     * Nothing needs to be initialized for
     * basic RTC polling.
     */
}


void rtc_read(struct rtc_time *time)
{
    if (time == 0)
        return;

    unsigned char second_a;
    unsigned char second_b;

    unsigned char status_b;

    do
    {
        rtc_wait_for_update();

        status_b =
            rtc_read_register(RTC_STATUS_B);

        time->second =
            rtc_read_binary_value(
                RTC_SECOND,
                status_b
            );

        time->minute =
            rtc_read_binary_value(
                RTC_MINUTE,
                status_b
            );

        unsigned char hour_raw =
            rtc_read_register(RTC_HOUR);

        /*
         * Handle binary / BCD.
         */
        unsigned char hour;

        if (status_b & 0x04)
        {
            hour = hour_raw & 0x7F;
        }
        else
        {
            hour =
                bcd_to_binary(
                    hour_raw & 0x7F
                );
        }

        /*
         * Bit 1 of Status B:
         *
         * 1 = 24-hour mode
         * 0 = 12-hour mode
         */
        if (!(status_b & 0x02))
        {
            /*
             * Original high bit means PM.
             */
            int pm = hour_raw & 0x80;

            if (hour == 12)
                hour = 0;

            if (pm)
                hour += 12;
        }

        time->hour = hour;

        time->day =
            rtc_read_binary_value(
                RTC_DAY,
                status_b
            );

        time->month =
            rtc_read_binary_value(
                RTC_MONTH,
                status_b
            );

        unsigned char year =
            rtc_read_binary_value(
                RTC_YEAR,
                status_b
            );

        /*
         * Try the standard CMOS century register.
         */
        unsigned char century =
            rtc_read_binary_value(
                RTC_CENTURY,
                status_b
            );

        if (century >= 20 && century <= 99)
            time->year =
                (century * 100) + year;
        else
            time->year = 2000 + year;

        /*
         * Read seconds again.
         *
         * If it changed during the read,
         * repeat the entire snapshot.
         */
        second_a =
            rtc_read_binary_value(
                RTC_SECOND,
                status_b
            );

        second_b = time->second;

    }
    while (second_a != second_b);
}
