#ifndef TRIANGLEOS_RTC_H
#define TRIANGLEOS_RTC_H

struct rtc_time
{
    unsigned int second;
    unsigned int minute;
    unsigned int hour;

    unsigned int day;
    unsigned int month;
    unsigned int year;
};

void rtc_init(void);
void rtc_read(struct rtc_time *time);

#endif
