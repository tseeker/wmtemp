#define	DEBUG(x) x;
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sensors.h>
#include <sensors/error.h>

#include "temp.h"

sensor_path_t cpu_sensor_path = { NULL, NULL, NULL };
sensor_path_t sys_sensor_path = { NULL, NULL, NULL };

temperature_t t_type = CELCIUS;

typedef struct sensor_source_t {
    int valid;
    sensors_chip_name const *chip;
    int feature;
} sensor_source_t;

static FILE *f = NULL;
static char sensors = 0;

static sensor_source_t cpu_feature = {0, NULL, 0};
static sensor_source_t sys_feature = {0, NULL, 0};

void temp_set_default(sensor_path_t *path, char const *subfeature)
{
    if (!(path->chip || path->feature || path->subfeature)) {
        path->subfeature = subfeature;
    }
}

void temp_deinit() {
	if (f != NULL) {
		fclose(f);
	}

	if (sensors) {
		sensors_cleanup();
	}
}

void temp_find_feature(sensor_path_t const *path, sensor_source_t *feature)
{
	const sensors_chip_name *s_name;
    const sensors_feature *s_feature;
	const sensors_subfeature *s_subfeature;
    char *s_feat_name;
	char str[256];
    int chip_nr = 0, f_nr;

	while ((s_name = sensors_get_detected_chips(NULL, &chip_nr)) != NULL) {
		sensors_snprintf_chip_name(str, 256, s_name);
        DEBUG(printf("L%d - chip_nr %d, name %s\n", __LINE__, chip_nr, str));
        if (path->chip != NULL && strcasecmp(str, path->chip)) {
            continue;
        }

        f_nr = 0;
        while ((s_feature = sensors_get_features(s_name, &f_nr)) != NULL) {
		    s_feat_name = sensors_get_label(s_name, s_feature);
            DEBUG(printf("L%d - feature #%d : %s\n", __LINE__, f_nr, s_feat_name));
            if (path->feature != NULL && strcasecmp(s_feat_name, path->feature)) {
                continue;
            }

            s_subfeature = sensors_get_subfeature(s_name, s_feature,
                            SENSORS_SUBFEATURE_TEMP_INPUT);
		    if (!s_subfeature) {
                continue;
            }
            DEBUG(printf("L%d - subfeature %s\n", __LINE__, s_subfeature->name));

            if (path->subfeature == NULL
                    || !strcasecmp(s_subfeature->name, path->subfeature)) {
                feature->valid = 1;
                feature->chip = s_name;
                feature->feature = s_subfeature->number;
                DEBUG(printf("L%d - found\n", __LINE__));
                return;
            }
        }
    }
}

void temp_init(const char *filename) {
    temp_set_default(&cpu_sensor_path, "temp1_input");
    temp_set_default(&sys_sensor_path, "temp2_input");

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
    temp_find_feature(&cpu_sensor_path, &cpu_feature);
    temp_find_feature(&sys_sensor_path, &sys_feature);
}

unsigned int temp_read(sensor_source_t const *source) {
    double value;
    if (source->valid) {
        if (sensors_get_value(source->chip, source->feature, &value)) {
            return 0;
        }
        if (t_type == FAHRENHEIT) {
            value = TO_FAHRENHEIT(value);
        } else if (t_type == KELVIN) {
            value = TO_KELVIN(value);
        }
        return (unsigned int)(value);
    } else {
        return 0;
    }
}

void temp_getusage(unsigned int *cpu_temp, unsigned int *sys_temp) {
    *cpu_temp = temp_read(&cpu_feature);
    *sys_temp = temp_read(&sys_feature);
}
