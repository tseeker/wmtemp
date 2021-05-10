/*    WMtemp - A dockapp to monitor CPU and MB temperatures
 *    Copyright (C) 2004  Peter Gnodde <peter@gnodde.org>
 *
 *    Some ideas loaned from the wmgtemp dockapp
 *
 *    Based on work by Mark Staggs <me@markstaggs.net>
 *    Copyright (C) 2002  Mark Staggs <me@markstaggs.net>
 *
 *    Based on work by Seiichi SATO <ssato@sh.rim.or.jp>
 *    Copyright (C) 2001,2002  Seiichi SATO <ssato@sh.rim.or.jp>

 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.

 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.

 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <string.h>

#include "dockapp.h"
#include "temp.h"
#include "xpm/xpm.h"

#define SIZE	    58
#define WINDOWED_BG "  \tc #AEAAAE"
#define MAX_HISTORY 16
#define CPUNUM_NONE -1

typedef enum { LIGHTON, LIGHTOFF } light;

Pixmap pixmap;
Pixmap backdrop_on;
Pixmap backdrop_off;
Pixmap parts;
Pixmap mask;
static char	*display_name = "";
static char	*light_color = NULL;	/* back-light color */
static unsigned update_interval = 1;
static light    backlight = LIGHTOFF;

static unsigned int cpu_temp = 0;
static unsigned int sys_temp = 0;
static unsigned int alarm_cpu = 60;
static unsigned int alarm_sys = 60;
char *configfile = NULL;

static char **backlight_on_xpm;
static char **backlight_off_xpm;

/* prototypes */
static void update(void);
static void switch_light(void);
static void draw_cpudigit(int per);
static void draw_sysdigit(int per);
static void parse_arguments(int argc, char **argv);
static void print_help(char *prog);

int main(int argc, char **argv)
{
   XEvent event;
   XpmColorSymbol colors[2] = { {"Back0", NULL, 0}, {"Back1", NULL, 0} };
   int ncolor = 0;

   /* Parse CommandLine */
   parse_arguments(argc, argv);

   if (t_type == FAHRENHEIT) {
	   alarm_cpu = TO_FAHRENHEIT(alarm_cpu);
	   alarm_sys = TO_FAHRENHEIT(alarm_sys);

	   backlight_on_xpm = fahrenheit_on_xpm;
	   backlight_off_xpm = fahrenheit_off_xpm;
   } else if (t_type == KELVIN) {
	   alarm_cpu = TO_KELVIN(alarm_cpu);
	   alarm_sys = TO_KELVIN(alarm_sys);

	   backlight_on_xpm = kelvin_on_xpm;
	   backlight_off_xpm = kelvin_off_xpm;
   } else {
	   backlight_on_xpm = celcius_on_xpm;
	   backlight_off_xpm = celcius_off_xpm;
   }
   
   /* Initialize Application */
   temp_init(configfile);
   dockapp_open_window(display_name, PACKAGE, SIZE, SIZE, argc, argv);
   dockapp_set_eventmask(ButtonPressMask);

   if(light_color)
   {
      colors[0].pixel = dockapp_getcolor(light_color);
      colors[1].pixel = dockapp_blendedcolor(light_color, -24, -24, -24, 1.0);
      ncolor = 2;
   }

   /* change raw xpm data to pixmap */
   if(dockapp_iswindowed)
      backlight_on_xpm[1] = backlight_off_xpm[1] = WINDOWED_BG;

   if(!dockapp_xpm2pixmap(backlight_on_xpm, &backdrop_on, &mask, colors, ncolor))
   {
      fprintf(stderr, "Error initializing backlit background image.\n");
      exit(1);
   }
   if(!dockapp_xpm2pixmap(backlight_off_xpm, &backdrop_off, NULL, NULL, 0))
   {
      fprintf(stderr, "Error initializing background image.\n");
      exit(1);
   }
   if(!dockapp_xpm2pixmap(parts_xpm, &parts, NULL, colors, ncolor))
   {
      fprintf(stderr, "Error initializing parts image.\n");
      exit(1);
   }

   /* shape window */
   if(!dockapp_iswindowed)
      dockapp_setshape(mask, 0, 0);
   if(mask) XFreePixmap(display, mask);

   /* pixmap : draw area */
   pixmap = dockapp_XCreatePixmap(SIZE, SIZE);

   /* Initialize pixmap */
   if(backlight == LIGHTON) 
      dockapp_copyarea(backdrop_on, pixmap, 0, 0, SIZE, SIZE, 0, 0);
   else
      dockapp_copyarea(backdrop_off, pixmap, 0, 0, SIZE, SIZE, 0, 0);

   dockapp_set_background(pixmap);
   dockapp_show();

   update();

   /* Main loop */
   while(1)
   {
      if (dockapp_nextevent_or_timeout(&event, update_interval * 1000))
      {
         /* Next Event */
         switch(event.type)
         {
            case ButtonPress:
               switch_light();
               break;
            default: /* make gcc happy */
               break;
         }
      }
      else
      {
         /* Time Out */
         update();
      }
   }

   return 0;
}

/* called by timer */
static void update(void)
{
   static light pre_backlight;
   static Bool in_alarm_mode = False;

   /* get current cpu usage in percent */
   temp_getusage(&cpu_temp, &sys_temp);

   /* alarm mode */
   if(cpu_temp >= alarm_cpu || sys_temp >= alarm_sys) 
   {
      if(!in_alarm_mode)
      {
         in_alarm_mode = True;
         pre_backlight = backlight;
      }
      if(backlight == LIGHTOFF)
      {
         switch_light();
         return;
      }
   } 
   else
   {
      if(in_alarm_mode)
      {
         in_alarm_mode = False;
         if (backlight != pre_backlight) 
         {
            switch_light();
            return;
         }
      }
   }

   /* all clear */
   if (backlight == LIGHTON) 
      dockapp_copyarea(backdrop_on, pixmap, 0, 0, 58, 58, 0, 0);
   else 
      dockapp_copyarea(backdrop_off, pixmap, 0, 0, 58, 58, 0, 0);

   /* draw digit */
   draw_cpudigit(cpu_temp);
   draw_sysdigit(sys_temp);

   /* show */
   dockapp_copy2window(pixmap);
}

/* called when mouse button pressed */
static void switch_light(void)
{
   switch (backlight)
   {
      case LIGHTOFF:
         backlight = LIGHTON;
         dockapp_copyarea(backdrop_on, pixmap, 0, 0, 58, 58, 0, 0);
         break;
      case LIGHTON:
         backlight = LIGHTOFF;
         dockapp_copyarea(backdrop_off, pixmap, 0, 0, 58, 58, 0, 0);
         break;
   }

   /* redraw digit */
   temp_getusage(&cpu_temp, &sys_temp);
   draw_cpudigit(cpu_temp);
   draw_sysdigit(sys_temp);

   /* show */
   dockapp_copy2window(pixmap);
}

static void draw_cpudigit(int per)
{
   int v100, v10, v1;
   int y = 0;

   if (per < 0) per = 0;

   v100 = per / 100;
   v10  = (per - v100 * 100) / 10;
   v1   = (per - v100 * 100 - v10 * 10);

   if (backlight == LIGHTON) y = 20;

   /* draw digit */
   dockapp_copyarea(parts, pixmap, v1 * 10, y, 10, 20, 29, 7);
   if (v10 != 0 || v100 != 0) {
	   dockapp_copyarea(parts, pixmap, v10 * 10, y, 10, 20, 17, 7);
	   
	   if (v100 != 0) {
		   dockapp_copyarea(parts, pixmap, v100 * 10, y, 10, 20,  5, 7);
	   }
   }
}


static void draw_sysdigit(int per)
{
   int v100, v10, v1;
   int y = 0;

   if (per < 0) per = 0;

   v100 = per / 100;
   v10  = (per - v100 * 100) / 10;
   v1   = (per - v100 * 100 - v10 * 10);

   if (backlight == LIGHTON) y = 20;

   /* draw digit */
   dockapp_copyarea(parts, pixmap, v1 * 10, y, 10, 20, 29, 34);
   if (v10 != 0 || v100 != 0) {
	   dockapp_copyarea(parts, pixmap, v10 * 10, y, 10, 20, 17, 34);

	   if (v100 != 0) {
		   dockapp_copyarea(parts, pixmap, v100 * 10, y, 10, 20, 5, 34);
	   }
   }
}

static void parse_arguments(int argc, char **argv)
{
   int i;
   int integer;
   for (i = 1; i < argc; i++) 
   {
      if (!strcmp(argv[i], "-v"))
         printf("%s version %s\n", PACKAGE, VERSION), exit(0);
      else if (!strcmp(argv[i], "-d")) 
      {
         display_name = argv[i + 1];
         i++;
      }
      else if (!strcmp(argv[i], "-ac")) 
      {
         if (argc == i + 1)
            alarm_cpu = 60;
         else if (sscanf(argv[i + 1], "%i", &integer) != 1)
            alarm_cpu = 60;
         else if (integer < 0 || integer > 100)
            fprintf(stderr, "%s: argument %s must be from 0 to 100\n",
                    argv[0], argv[i]), exit(1);
         else
            alarm_cpu = integer, i++;
      }
      else if (!strcmp(argv[i], "-as")) 
      {
         if (argc == i + 1)
            alarm_sys = 60;
         else if (sscanf(argv[i + 1], "%i", &integer) != 1)
            alarm_sys = 60;
         else if (integer < 0 || integer > 100)
            fprintf(stderr, "%s: argument %s must be from 0 to 100\n",
                    argv[0], argv[i]), exit(1);
         else
            alarm_sys = integer, i++;
      }
      else if (!strcmp(argv[i], "-bl"))
         backlight = LIGHTON;
      else if (!strcmp(argv[i], "-lc")) 
      {
         light_color = argv[i + 1];
         i++;
      }
      else if (!strcmp(argv[i], "-i")) 
      {
         if (argc == i + 1)
            fprintf(stderr, "%s: error parsing argument for option %s\n",
                    argv[0], argv[i]), exit(1);
         if (sscanf(argv[i + 1], "%i", &integer) != 1)
             fprintf(stderr, "%s: error parsing argument for option %s\n",
                     argv[0], argv[i]), exit(1);
         if (integer < 1)
             fprintf(stderr, "%s: argument %s must be >=1\n",
                     argv[0], argv[i]), exit(1);
         update_interval = integer;
         i++;
      }
      else if (!strcmp(argv[i], "-w"))
         dockapp_iswindowed = True;
      else if (!strcmp(argv[i], "-bw"))
         dockapp_isbrokenwm = True;
	  else if (!strcmp(argv[i], "-c")) {
		  if (argc == i + 1) {
			fprintf(stderr, "%s: error parsing argument for option %s\n", argv[0], argv[i]);
			exit(1);
		  }

		  configfile = argv[i + 1];
		  i++;
	  }
	  else if (!strcmp(argv[i], "-s")) {
		  char *t;

		  t = cpu_feature_name;
		  cpu_feature_name = sys_feature_name;
		  sys_feature_name = t;
	  }
	  else if (!strcmp(argv[i], "-cf")) {
		  if (argc == i + 1) {
			fprintf(stderr, "%s: error parsing argument for option %s\n", argv[0], argv[i]);
			exit(1);
		  }

		  cpu_feature_name = argv[i + 1];
		  i++;
	  }
	  else if (!strcmp(argv[i], "-sf")) {
		  if (argc == i + 1) {
			fprintf(stderr, "%s: error parsing argument for option %s\n", argv[0], argv[i]);
			exit(1);
		  }

		  sys_feature_name = argv[i + 1];
		  i++;
	  }
	  else if (!strcmp(argv[i], "-f"))
		  t_type = FAHRENHEIT;
	  else if (!strcmp(argv[i], "-k"))
		  t_type = KELVIN;
      else
      {
         print_help(argv[0]), exit(1);
      }
   }
}

static void print_help(char *prog)
{
   printf("Usage : %s [OPTIONS]\n", prog);
   printf("WMtemp - temperature monitor dockapp\n");
   printf("  -d <string>                    display to use\n");
   printf("  -bl                            turn on back-light\n");
   printf("  -lc <string>                   back-light color (rgb:6E/C6/3B is default)\n");
   printf("  -i <number>                    number of secs between updates (1 is default)\n");
   printf("  -h                             show this help text and exit\n");
   printf("  -v                             show program version and exit\n");
   printf("  -w                             run the application in windowed mode\n");
   printf("  -bw                            activate broken window manager fix\n");
   printf("  -ac <number>                   activate alarm mode of CPU\n");
   printf("                                 <number> is threshold from 0 to 100\n");
   printf("                                 (60 is default)\n");
   printf("  -as <number>                   activate alarm mode of system\n");
   printf("                                 <number> is threshold from 0 to 100\n");
   printf("                                 (60 is default)\n");
   printf("  -c <path>                      location of the sensors.conf file\n");
   printf("                                 ('/etc/sensors.conf' is default)\n");
   printf("  -cf <feature>                  which feature to use for cpu temperature\n");
   printf("                                 ('temp1_input' is default)\n");
   printf("  -sf <feature>                  which feature to use for sys temperature\n");
   printf("                                 ('temp2_input' is default)\n");
   printf("  -s                             swap the cpu and sys temperatures\n");
   printf("                                 (/etc/sensors.conf is default)\n");
   printf("  -f                             show temperatures in Fahrenheit\n");
   printf("  -k                             show temperatures in Kelvin\n");
}
