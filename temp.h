#ifndef SHO_TEMP_H
#define SHO_TEMP_H

#define PACKAGE		"wmtemp"
#define VERSION		"0.0.4"

#define TO_FAHRENHEIT(t)		(((double)(t) * 1.8) + 32.0)
#define TO_KELVIN(t)			((double)(t) + 273.0)

typedef enum temperature_t {
	CELCIUS, FAHRENHEIT, KELVIN
} temperature_t;

extern char *cpu_feature_name;
extern char *sys_feature_name;

extern temperature_t t_type;

void temp_init(const char *filename);
void temp_getusage(unsigned int *cpu_temp, unsigned int *sys_temp);

#endif
