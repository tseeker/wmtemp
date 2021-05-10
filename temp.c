#define	DEBUG(x) x                 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sensors.h>

#include "temp.h"

static FILE *f = NULL;
static char sensors = 0;

static const sensors_chip_name *chip_name = NULL;
static const sensors_feature *feature;

char *cpu_feature_name = "temp1_input";
char *sys_feature_name = "temp2_input";

temperature_t t_type = CELCIUS;

static int cpu_feature = 0;
static int sys_feature = 0;

void temp_deinit() {
	if (f != NULL) {
		fclose(f);
	}

	if (sensors) {
		sensors_cleanup();
	}
}

void temp_init(const char *filename) {
	const sensors_chip_name *name;
	int chip_nr = 0, f1, f2;

	char str[256];

	char *feattext = NULL;
	const sensors_subfeature *subfeature;

	atexit(temp_deinit);

	if (filename) {
		f = fopen(filename, "r");
		if (f == NULL) {
			fprintf(stderr, "could not open configfile %s: %s\n", filename,
					strerror(errno));
			exit(1);
		}
	}

	if (sensors_init(f)) {
		fprintf(stderr, "could not initialize sensors\n");
		exit(1);
	}

	sensors = 1;
	while ((name = sensors_get_detected_chips(NULL, &chip_nr)) != NULL &&
			chip_name == NULL) {
		f1 = f2 = 0;
		DEBUG(printf("chip_nr=%d %d\n",chip_nr,__LINE__);)

		sensors_snprintf_chip_name(str, 256, name);
		DEBUG(printf("chip name = %s (%d)\n",str,__LINE__);)

		while ((feature = sensors_get_features( name, &f1)) != NULL) {

		    feattext = sensors_get_label( name, feature );
		    DEBUG(printf("f1=%d feattext=%s (%d) \n",f1,feattext,__LINE__);)
  
		    if ( (subfeature = sensors_get_subfeature (name, feature, SENSORS_SUBFEATURE_TEMP_INPUT)) ) {
			DEBUG(printf("subfeature name =%s (%d) \n",subfeature->name,__LINE__);)
			if (strcmp(subfeature->name, cpu_feature_name) == 0) {
			    cpu_feature = subfeature->number;
			    chip_name = name;
			} 
			else if (strcmp(subfeature->name, sys_feature_name) == 0) {
			    sys_feature = subfeature->number;
			}
		    }
		}
	}

	if (chip_name == NULL) {
		fprintf(stderr, "could not find a suitable chip\n");
		exit(1);
	}
}

void temp_getusage(unsigned int *cpu_temp, unsigned int *sys_temp) {
	double cpu, sys;

	sensors_get_value(chip_name, cpu_feature, &cpu);
	sensors_get_value(chip_name, sys_feature, &sys);

	if (t_type == FAHRENHEIT) {
		cpu = TO_FAHRENHEIT(cpu);
		sys = TO_FAHRENHEIT(sys);
	} else if (t_type == KELVIN) {
		cpu = TO_KELVIN(cpu);
		sys = TO_KELVIN(sys);
	}
	
	*cpu_temp = (unsigned int)(cpu);
	*sys_temp = (unsigned int)(sys);
}
