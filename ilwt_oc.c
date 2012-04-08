/* 
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <android/log.h>
#include <dirent.h>

#define CONFIG_FILE "/system/ilwt/ilwt_oc.conf"
#define CONFIG_FILE_SDCARD "/mnt/sdcard/ILWT/ilwt_oc.conf"

//IOSCHED-CHANGE - start
#define CONFIG_FILE_IO_SCHED "/system/ilwt/ilwt_io_sched.conf"
#define CONFIG_FILE_SDCARD_IO_SCHED "/mnt/sdcard/ILWT/ilwt_io_sched.conf"

#define IO_BLOCK_DEVICES_BASE_PATH "/sys/block/" 
#define IO_SCHEDULER_RELATIVE_PATH "/queue/scheduler"
//IOSCHED-CHANGE - end

#define SYS_CGOV_C0 "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
#define SYS_CMAX_C0 "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
#define SYS_CMIN_C0 "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq"

#define SYS_CGOV_C1 "/sys/devices/system/cpu/cpu1/cpufreq/scaling_governor"
#define SYS_CMAX_C1 "/sys/devices/system/cpu/cpu1/cpufreq/scaling_max_freq"
#define SYS_CMIN_C1 "/sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq"

#define SYS_WAKE "/sys/power/wait_for_fb_wake"
#define SYS_SLEEP "/sys/power/wait_for_fb_sleep"

#define SYS_BATT_TEMP "/sys/class/power_supply/battery/batt_temp"
#define SYS_BATT_CAP "/sys/class/power_supply/battery/capacity"
#define SYS_CHARGE "/sys/class/power_supply/battery/charging_enabled"

#define APPNAME "ILWT_OC"
#define APPVERSION "140"

// 0: SD Card, 1: /system
int configFile = 0;
int io_configFile = 0;

typedef struct s_ocConfig
{
  char wake_min_freq[30];
  char wake_max_freq[30];
  char wake_governor[30];

  char sleep_min_freq[30];
  char sleep_max_freq[30];
  char sleep_governor[30];
  
  char battery_temp[30];
  char battery_temp_governor[30];
  char battery_temp_min_freq[30];
  char battery_temp_max_freq[30];
  
  char battery_cap[30];
  char battery_cap_governor[30];
  char battery_cap_min_freq[30];
  char battery_cap_max_freq[30];
  
  char charge_governor[30];
  char charge_min_freq[30];
  char charge_max_freq[30];
} ocConfig;

//IOSCHED-CHANGE - start
typedef struct s_ioSchedConfig
{
  char io_scheduler[30];
} ioSchedConfig;
//IOSCHED-CHANGE - end

void my_trim(char *str)
{
  int i;
  for (i = 0; str[i] != 0; i++)
    if ((str[i] == '\n' || str[i] == '\r'))
      str[i] = 0;
}

int write_to_file(char *path, char *value)
{
  FILE  *fd;
  int   res = 0;
  
  fd = fopen(path, "w");
  if (fd == NULL)
    return -1;
  if (fputs(value, fd) < 0)
    res = -1;
  fclose(fd);
  return res;
}

//IOSCHED-CHANGE - start
int set_io_sched_params(char *scheduler)
{
    DIR *dir = opendir(IO_BLOCK_DEVICES_BASE_PATH);
    if (NULL == dir)
        return -1;

    my_trim(scheduler);

    struct dirent *dp; 
    while ((dp=readdir(dir)) != NULL) {
        if(0 == strncmp(dp->d_name, "mmcblk", strlen("mmcblk")) ||
           0 == strncmp(dp->d_name, "bml", strlen("bml")) ||
           0 == strncmp(dp->d_name, "stl", strlen("stl")) ||
           0 == strncmp(dp->d_name, "tfsr", strlen("tfsr")) ) { 
            __android_log_write(ANDROID_LOG_INFO, APPNAME, dp->d_name);
            char final_path[1024];
            memset(final_path, '\0',1024);
            strcpy(final_path, IO_BLOCK_DEVICES_BASE_PATH);
            strcat(final_path, dp->d_name);
            strcat(final_path, IO_SCHEDULER_RELATIVE_PATH);
            if(write_to_file(final_path, scheduler) != 0)
	        return -1;
        }
    }
    char buf[255];
    buf[0] = 0;
    strcat(buf, "Setting IO Scheduler: Scheduler=");
    strcat(buf, scheduler);  
    __android_log_write(ANDROID_LOG_INFO, APPNAME, buf);

    closedir(dir);
    return 0;
}
//IOSCHED-CHANGE - end


int set_cpu_params(char *governor, char *min_freq, char *max_freq)
{
    if (write_to_file(SYS_CGOV_C0, governor) != 0)
      return -1; 
    if (write_to_file(SYS_CMAX_C0, max_freq) != 0)
      return -1;
    if (write_to_file(SYS_CMIN_C0, min_freq) != 0)
      return -1;

    write_to_file(SYS_CGOV_C1, governor);
    write_to_file(SYS_CMAX_C1, max_freq);
    write_to_file(SYS_CMIN_C1, min_freq);

    char buf[255];
    buf[0] = 0;
    strcat(buf, "Setting CPU Params: Governor=");
    my_trim(governor);
    strcat(buf, governor);  
    strcat(buf, " min_freq=");
    my_trim(min_freq);
    strcat(buf, min_freq);
    strcat(buf, " max_freq=");
    my_trim(max_freq);
    strcat(buf, max_freq);
    __android_log_write(ANDROID_LOG_INFO, APPNAME, buf);
    return 0;
}

int read_from_file(char *path, int len, char *result)
{
  FILE *fd;
  int res = 0;
  
  fd = fopen(path, "r");
  if (fd == NULL)
    return -1;
  if (fgets(result, len, fd) == NULL)
    res = -1;
  fclose(fd);
  return res;
}

int get_config_value(char *config_key, char *reference)
{
	FILE *config_file;
	if (configFile == 1)
		config_file = fopen(CONFIG_FILE, "r");
	else {
		config_file = fopen(CONFIG_FILE_SDCARD, "r");
		if (config_file == NULL) {
			__android_log_write(ANDROID_LOG_INFO, APPNAME, "No config file in SD card.");
			config_file = fopen(CONFIG_FILE, "r");
			configFile = 1;
			__android_log_write(ANDROID_LOG_INFO, APPNAME, "Reading config from system partition.");
		}
	}
	
	if (config_file == NULL) {
		__android_log_write(ANDROID_LOG_INFO, APPNAME, "Cannot open configuration file for input.");
		config_file = fopen(CONFIG_FILE, "r");
	}
	
	char line[30];
	char log[40];
	char temp[30];
	int res = -1;
	while (fgets(line, sizeof line, config_file)) {
		if (strncmp(line, config_key, strlen (config_key)) == 0) {
			strcpy(temp, config_key);
			strcat(temp, "%s");

			if (sscanf(line, temp, reference)) {
				strcpy(log, "Found ");
				strcat(log, config_key);
				strcat(log, reference);
				__android_log_write(ANDROID_LOG_INFO, APPNAME, log);
				res = 0;
				break;
			}
		}
	}

	fclose(config_file);
	
	if (res == -1) {
		strcpy(log, "Could not find ");
		strcat(log, config_key);
		__android_log_write(ANDROID_LOG_INFO, APPNAME, log);
	}
    return res;
}

//IOSCHED-CHANGE - start
int load_iosched_config(ioSchedConfig *io_conf)
{
        if (io_conf == NULL)
            return -1;
        if (io_configFile == 1)
            __android_log_write(ANDROID_LOG_INFO, APPNAME, "Reading IO Scheduler config from system partition.");
        else
            __android_log_write(ANDROID_LOG_INFO, APPNAME, "Reading IO Schduler config from SD card.");

        FILE *config_file;
	if (io_configFile == 1)
		config_file = fopen(CONFIG_FILE_IO_SCHED, "r");
	else {
		config_file = fopen(CONFIG_FILE_SDCARD_IO_SCHED, "r");
		if (config_file == NULL) {
			__android_log_write(ANDROID_LOG_INFO, APPNAME, "No IO Scheduler config file in SD card.");
			config_file = fopen(CONFIG_FILE_IO_SCHED, "r");
			io_configFile = 1;
			__android_log_write(ANDROID_LOG_INFO, APPNAME, "Reading IO Scheduler config from system partition.");
		}
	}
	
	if (config_file == NULL) {
		__android_log_write(ANDROID_LOG_INFO, APPNAME, "Cannot open IO Scheduler configuration file for input.");
		config_file = fopen(CONFIG_FILE_IO_SCHED, "r");
	}
	
	char line[30];
	char log[40];
	char temp[30];
	int res = -1;
        char * config_key = "io_scheduler=";
	while (fgets(line, sizeof line, config_file)) {
		if (strncmp(line, config_key, strlen (config_key)) == 0) {
			strcpy(temp, config_key);
			strcat(temp, "%s");
			if (sscanf(line, temp, io_conf->io_scheduler)) {
				strcpy(log, "Found ");
				strcat(log, config_key);
				strcat(log, io_conf->io_scheduler);
				__android_log_write(ANDROID_LOG_INFO, APPNAME, log);
				res = 0;
				break;
			}
		}
	}

	fclose(config_file);
	
	if (res == -1) {
		strcpy(log, "Could not find ");
		strcat(log, config_key);
		__android_log_write(ANDROID_LOG_INFO, APPNAME, log);
	}
        return res;
}
//IOSCHED-CHANGE - end

int load_config(ocConfig *conf)
{      
  if (conf == NULL)
    return -1;

  if (configFile == 1)
	__android_log_write(ANDROID_LOG_INFO, APPNAME, "Reading config from system partition.");
  else
    __android_log_write(ANDROID_LOG_INFO, APPNAME, "Reading config from SD card.");
	
  if (get_config_value("wake_min_freq=", conf->wake_min_freq) == -1)
    return -1;
  if (get_config_value("wake_max_freq=", conf->wake_max_freq) == -1)
    return -1;
  if (get_config_value("wake_governor=", conf->wake_governor) == -1)
    return -1;
	
   if (get_config_value("wake_min_freq=", conf->wake_min_freq) == -1)
    return -1;
  if (get_config_value("wake_max_freq=", conf->wake_max_freq) == -1)
    return -1;
  if (get_config_value("wake_governor=", conf->wake_governor) == -1)
    return -1;

  if (get_config_value("sleep_min_freq=", conf->sleep_min_freq) == -1)
    return -1;
  if (get_config_value("sleep_max_freq=", conf->sleep_max_freq) == -1)
    return -1;
  if (get_config_value("sleep_governor=", conf->sleep_governor) == -1)
    return -1;
	
  if (get_config_value("battery_temp=", conf->battery_temp) == -1)
    return -1;
  if (get_config_value("battery_temp_min_freq=", conf->battery_temp_min_freq) == -1)
    return -1;
  if (get_config_value("battery_temp_max_freq=", conf->battery_temp_max_freq) == -1)
    return -1;
  if (get_config_value("battery_temp_governor=", conf->battery_temp_governor) == -1)
    return -1;
	
  if (get_config_value("battery_cap=", conf->battery_cap) == -1)
    return -1;
  if (get_config_value("battery_cap_min_freq=", conf->battery_cap_min_freq) == -1)
    return -1;
  if (get_config_value("battery_cap_max_freq=", conf->battery_cap_max_freq) == -1)
    return -1;
  if (get_config_value("battery_cap_governor=", conf->battery_cap_governor) == -1)
    return -1;
	
  if (get_config_value("charge_min_freq=", conf->charge_min_freq) == -1)
    return -1;
  if (get_config_value("charge_max_freq=", conf->charge_max_freq) == -1)
    return -1;
  if (get_config_value("charge_governor=", conf->charge_governor) == -1)
    return -1;

  return 0;
}

int check_sleep()
{
	char input_buffer[9];
	input_buffer[0] = '\0';
      
    if (read_from_file(SYS_SLEEP, 9, input_buffer) == -1)              
      return 1;
	  
	if (strcmp(input_buffer, "sleeping") == 0)
		return 2;
	
	return 0;
}

int check_charge()
{
	char input_buffer[2];
	input_buffer[0] = '\0';
      
    if (read_from_file(SYS_CHARGE, 2, input_buffer) == -1) {
		__android_log_write(ANDROID_LOG_ERROR, APPNAME, "Unable to get charge status from file. Cannot check profile.");
		return 1;
	}
	  
	if (strcmp(input_buffer, "0") == 0)
		return 0;
	
	return 2;
}

int check_batt_cap(int battery_cap)
{
	char input_buffer[4];
	input_buffer[0] = '\0';	
		
	if (read_from_file(SYS_BATT_CAP, 4, input_buffer) == -1) {
		__android_log_write(ANDROID_LOG_ERROR, APPNAME, "Unable to get battery level from file. Cannot check profile.");
		return 1;
	}
	  
	if (atoi(input_buffer) <= battery_cap)
	  return 2;
	  
	return 0;
}

int check_batt_temp(int battery_temp)
{
	char input_buffer[4];
	input_buffer[0] = '\0';	
	
	if (read_from_file(SYS_BATT_TEMP, 4, input_buffer) == -1) {	
	  __android_log_write(ANDROID_LOG_ERROR, APPNAME, "Unable to get battery temperature from file. Cannot check profile.");
	  return 1;
	}
	
	if (atoi(input_buffer) >= battery_temp)
		return 2;
		
	return 0;
}

int main (int argc, char **argv)
{
	if (argc == 2) {
		if (strncmp(argv[1], "-v", 2) == 0) {
			__android_log_write(ANDROID_LOG_ERROR, APPNAME, "Getting ILWT OCD version");
			printf(APPVERSION);
		}	
	}
	else {
	  ocConfig  conf;
	  pid_t pid, sid;
	  char input_buffer[9];
	  int asleep = 0;
	  int charging = 0;
	  int low_batt = 0;
	  int hot_batt = 0;
	  
	  __android_log_write(ANDROID_LOG_INFO, APPNAME, "Starting service.");
	  if (load_config(&conf) == -1){
		if (configFile == 0) {
			configFile = 1;
			if (load_config(&conf) == -1) {
				__android_log_write(ANDROID_LOG_ERROR, APPNAME, "Unable to load configuration. Stopping.");
				return 1;
			}
		}
		else {
			__android_log_write(ANDROID_LOG_ERROR, APPNAME, "Unable to load configuration. Stopping.");
			return 1;
		}
	  }

          //IOSCHED-CHANGE - start
          ioSchedConfig io_config;
          if(load_iosched_config(&io_config) == -1){
              if(io_configFile == 0) {
                  io_configFile = 1;
                  if(load_iosched_config(&io_config) == -1){
                      __android_log_write(ANDROID_LOG_ERROR, APPNAME, "Unable to load io configuration. Stopping.");
                      return 1;
                  }
              } else {
                  __android_log_write(ANDROID_LOG_ERROR, APPNAME, "Unable to load io configuration. Stopping.");
                  return 1;
              }
          }
          //IOSCHED-CHANGE - end

	  
	  input_buffer[0] = 0;
		
	  pid = fork();
	  if (pid < 0)
		exit(2);
	  if (pid > 0)
		exit(0);
	  umask(0);
	  
	  sid = setsid();
	  if (sid < 0)
		exit(2);
	  if ((chdir("/")) < 0)
		exit(2);
	  close(STDIN_FILENO);
	  close(STDOUT_FILENO);
	  close(STDERR_FILENO);

          //IOSCHED-CHANGE
          set_io_sched_params(io_config.io_scheduler);
	  
	  char* my_governor = conf.wake_governor;
	  char* my_min_freq = conf.wake_min_freq;
	  char* my_max_freq = conf.wake_max_freq;
	  
	  input_buffer[0] = '\0';
	  
	  if (read_from_file(SYS_WAKE, 6, input_buffer) == -1)
	  {                  
		__android_log_write(ANDROID_LOG_ERROR, APPNAME, "Unable to get data from file. Cannot continue.");
		return 1;
	  }
	  if (strcmp(input_buffer, "awake") == 0)
	  {
		__android_log_write(ANDROID_LOG_INFO, APPNAME, "Setting awake profile for boot sequence.");
		set_cpu_params(my_governor, my_min_freq, my_max_freq);
	  }

	  while (1)
	  {
		asleep = check_sleep();
		
		if (asleep == 2)
		{
		  __android_log_write(ANDROID_LOG_INFO, APPNAME, "Setting sleep profile.");
		  set_cpu_params(conf.sleep_governor, conf.sleep_min_freq, conf.sleep_max_freq);
		}
		else if (asleep == 1)
		{
			__android_log_write(ANDROID_LOG_ERROR, APPNAME, "Unable to get data from file. Cannot continue.");
			return 1;
		}
	  
		input_buffer[0] = '\0';
	  
		if (read_from_file(SYS_WAKE, 6, input_buffer) == -1) {                  
		  __android_log_write(ANDROID_LOG_ERROR, APPNAME, "Unable to get data from file. Cannot continue.");
		  return 1;
		}
		if (strcmp(input_buffer, "awake") == 0)
		{
			hot_batt = check_batt_temp(atoi(conf.battery_temp));
		
			if (hot_batt == 2) {
				__android_log_write(ANDROID_LOG_INFO, APPNAME, "Setting heat profile.");
				my_governor = conf.battery_temp_governor;
				my_min_freq = conf.battery_temp_min_freq;
				my_max_freq = conf.battery_temp_max_freq;
			}
			else {
				charging = check_charge();
				if (charging == 2) {
					__android_log_write(ANDROID_LOG_INFO, APPNAME, "Setting charge profile.");
					my_governor = conf.charge_governor;
					my_min_freq = conf.charge_min_freq;
					my_max_freq = conf.charge_max_freq;
				}
				else {
					low_batt = check_batt_cap(atoi(conf.battery_cap));
					
					if (low_batt == 2) {
						__android_log_write(ANDROID_LOG_INFO, APPNAME, "Setting capacity profile.");
						my_governor = conf.battery_cap_governor;
						my_min_freq = conf.battery_cap_min_freq;
						my_max_freq = conf.battery_cap_max_freq;
					}
					else {
						__android_log_write(ANDROID_LOG_INFO, APPNAME, "Setting awake profile.");
						my_governor = conf.wake_governor;
						my_min_freq = conf.wake_min_freq;
						my_max_freq = conf.wake_max_freq;
					}
				}
			}
				
			set_cpu_params(my_governor, my_min_freq, my_max_freq);
		}
	  }
  }
 
  return 0;
}
