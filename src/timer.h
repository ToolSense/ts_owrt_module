#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <stdbool.h>

#define TIMER_100mc 100000
#define TIMER_5S 50
#define TIMER_1S 10

typedef struct TimerPeriod
{
	bool period_1m;
	bool period_1s;
	bool period_100ms;
} timer_periods;

timer_periods t_period;

void timer_handler_100ms();

int timer_start(int timer_intr);

int timer_stop();

void timer_init();