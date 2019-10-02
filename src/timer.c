#include "timer.h"

int timer_counter = 0;

void timer_handler_100ms()
{
	t_period.period_100ms = true;
	timer_counter++;

	if ((timer_counter % TIMER_1S) == 0) {
		t_period.period_1s = true;
	}

	// Maximum period - TIMER_5S
	if (timer_counter == TIMER_5S) {
		t_period.period_1m = true;
		timer_counter = 0;
	}
}

int timer_start(int timer_intr)
{
	struct itimerval tv;
	int rc = 0;

	tv.it_interval.tv_sec = 0;
	tv.it_interval.tv_usec = timer_intr;
	tv.it_value.tv_sec = 0;
	tv.it_value.tv_usec = timer_intr;

	rc = setitimer(ITIMER_REAL, &tv, NULL);	

	return rc;
}

int timer_stop()
{
	struct itimerval tv;
	int rc = 0;

	tv.it_interval.tv_sec = 0;
	tv.it_interval.tv_usec = 0;
	tv.it_value.tv_sec = 0;
	tv.it_value.tv_usec = 0;

	rc = setitimer(ITIMER_REAL, &tv, NULL);	

	return rc;
}

void timer_init()
{
	struct sigaction psa;
	memset(&psa, 0, sizeof(psa));
	psa.sa_handler = &timer_handler_100ms;
	sigaction(SIGALRM, &psa, NULL);
}