/*
 * Last updated: July 22, 2008
 * ~Keripo
 *
 * Copyright (C) 2008 Keripo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

 
/* == Volume code == */

#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>

static int ipod_mixer;
static int ipod_volume;
static int volume_current = -1;

static void ipod_update_volume()
{
	if (volume_current != ipod_volume) {
		volume_current = ipod_volume;
		int vol;
		vol = volume_current << 8 | volume_current;
		ioctl(ipod_mixer, SOUND_MIXER_WRITE_PCM, &vol);
	}
}

static void ipod_init_sound()
{
	ipod_mixer = open("/dev/mixer", O_RDWR);
	ipod_volume = 50; // Good default
	ipod_update_volume();
}

static void ipod_exit_sound()
{
	close(ipod_mixer);
}


/* == Backlight code == */

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define BACKLIGHT_OFF	0
#define BACKLIGHT_ON	1
#define FBIOSET_BACKLIGHT	_IOW('F', 0x25, int)

static int backlight_current;

static int ipod_ioctl(int request, int *arg)
{
	int fd;
	fd = open("/dev/fb0", O_NONBLOCK);
	if (fd < 0) fd = open("/dev/fb/0", O_NONBLOCK);
	if (fd < 0) return -1;
	if (ioctl(fd, request, arg) < 0) {
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

static void ipod_set_backlight(int backlight)
{
	ipod_ioctl(FBIOSET_BACKLIGHT, (int *)(long)backlight);
}

static void ipod_toggle_backlight()
{
	if (backlight_current == 0) {
		ipod_set_backlight(BACKLIGHT_ON);
		backlight_current = 1;
	} else {
		ipod_set_backlight(BACKLIGHT_OFF);
		backlight_current = 0;
	}
}

static void ipod_init_backlight()
{
	ipod_set_backlight(BACKLIGHT_ON);
	backlight_current = 1;
}


/* == Input code == */

#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <linux/kd.h>

#define KEY_MENU	50 // Up
#define KEY_PLAY	32 // Down
#define KEY_REWIND	17 // Left
#define KEY_FORWARD	33 // Right
#define KEY_ACTION	28 // Select
#define KEY_HOLD	35 // Exit
#define SCROLL_L	38 // Counter-clockwise
#define SCROLL_R	19 // Clockwise
#define KEY_NULL	-1 // No key event

#define KEYCODE(a)	(a & 0x7f) // Use to get keycode of scancode.
#define KEYSTATE(a)	(a & 0x80) // Check if key is pressed or lifted

static int console;
static struct termios stored_settings;

static int ipod_get_keypress()
{
	int press = 0;
	if (read(console, &press, 1) != 1)
		return KEY_NULL;
	return press;
}

static void ipod_init_input()
{
	struct termios new_settings;
	console = open("/dev/console", O_RDONLY | O_NONBLOCK);
	tcgetattr(console, &stored_settings);
	
	new_settings = stored_settings;
	new_settings.c_lflag &= ~(ICANON | ECHO | ISIG);
	new_settings.c_iflag &= ~(ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON | BRKINT);
	new_settings.c_cc[VTIME] = 0;
	new_settings.c_cc[VMIN] = 0;
	
	tcsetattr(console, TCSAFLUSH, &new_settings);
	ioctl(console, KDSKBMODE, K_MEDIUMRAW);
}

static void ipod_exit_input()
{
	// Causes screwy characters to appear
	//tcsetattr(console, TCSAFLUSH, &stored_settings);
	close(console);
}


/* == Main loop == */

#define SCROLL_MOD_NUM 3 // Via experimentation
#define SCROLL_MOD(n) \
	({ \
		static int scroll_count = 0; \
		int use = 0; \
		if (++scroll_count >= n) { \
			scroll_count -= n; \
			use = 1; \
		} \
		(use == 1); \
	})

void ipod_init_volume_control()
{
	ipod_init_sound();
	ipod_init_input();
	ipod_init_backlight();
	
	int input, exit;
	exit = 0;
	while (exit != 1) {
		input = ipod_get_keypress();
		if (!KEYSTATE(input)) { // Pressed
			input = KEYCODE(input);
			switch (input) {
				case SCROLL_R:
					if (SCROLL_MOD(SCROLL_MOD_NUM)) {
						ipod_volume++;
						if (ipod_volume > 70)
							ipod_volume = 70; // To be safe - 70 is VERY loud
						ipod_update_volume();
					}
					break;
				case KEY_ACTION:
					ipod_toggle_backlight();
					break;
				case SCROLL_L:
					if (SCROLL_MOD(SCROLL_MOD_NUM)) {
						ipod_volume--;
						if (ipod_volume < 0)
							ipod_volume = 0; // Negative volume DNE!
						ipod_update_volume();
					}
					break;
				case KEY_MENU:
					exit = 1;
					break;			
				default:
					break;		
			}
		}
	}
	printf("Exiting...\n");
	ipod_exit_sound();
	ipod_exit_input();
}

