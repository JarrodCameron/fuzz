/*******************************************
 *                                         *
 *    Author: Jarrod Cameron (z5210220)    *
 *    Date:   11/09/20 11:02               *
 *                                         *
 *******************************************/

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "display.h"
#include "safe.h"
#include "config.h"

/* `clear | xxd` */
#define CLEAR "\x1b\x5b\x48\x1b\x5b\x32\x4a\x1b\x5b\x33\x4a"

/* Helper functions */
static void screen_clear(void);
static time_t get_hours(time_t t);
static time_t get_mins(time_t t);
static time_t get_secs(time_t t);

static struct {
	uint64_t ndeploys;
	time_t start_time;
} display = {0};

void display_init(void)
{
	screen_clear();

	stime(&display.start_time);
}

void display_deploy(void)
{
	time_t t = 0;

	display.ndeploys++;

	/* It is slow to update the display every deploy */
	if (display.ndeploys % 10 != 0)
		return;

	screen_clear();

	stime(&t);

	printf("# of attempts: %lu\n", display.ndeploys);
	printf("Time elapsed: %02lu:%02lu:%02lu\n",
		get_hours(t - display.start_time),
		get_mins(t - display.start_time),
		get_secs(t - display.start_time)
	);

	fflush(stdout);
}

void display_fini(void)
{
	time_t t = 0;

	screen_clear();

	stime(&t);

	printf("Finished!\n");
	printf("Payload in: \"%s\"\n", BAD_FILE);

	printf("# of attempts: %lu\n", display.ndeploys);
	printf("Time elapsed: %02lu:%02lu:%02lu\n",
		get_hours(t - display.start_time),
		get_mins(t - display.start_time),
		get_secs(t - display.start_time)
	);

	fflush(stdout);
}

static
void
screen_clear(void)
{
	swrite(1, CLEAR, sizeof(CLEAR)-1);
}

static
time_t
get_hours(time_t t)
{
	return (t / 60) / 60;
}

static
time_t
get_mins(time_t t)
{
	return (t / 60) % 60;
}

static
time_t
get_secs(time_t t)
{
	return t % 60;
}

