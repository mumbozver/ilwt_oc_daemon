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

#define CONFIG_ROOT "/system/etc/ilwt_oc/"

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

#define APPNAME "ILWT_OC"

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
} ocConfig;

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

int get_config_value(char *config_key, char *reference)
{
    char config_path[60];
    
    strcpy(config_path, CONFIG_ROOT);
    strcat(config_path, config_key);

    return read_from_file(config_path, 30, reference);
}

int  load_config(ocConfig *conf)
{      
  if (conf == NULL)
    return -1;
	
  if (get_config_value("wake_min_freq", conf->wake_min_freq) == -1)
    return -1;
  if (get_config_value("wake_max_freq", conf->wake_max_freq) == -1)
    return -1;
  if (get_config_value("wake_governor", conf->wake_governor) == -1)
    return -1;

  if (get_config_value("sleep_min_freq", conf->sleep_min_freq) == -1)
    return -1;
  if (get_config_value("sleep_max_freq", conf->sleep_max_freq) == -1)
    return -1;
  if (get_config_value("sleep_governor", conf->sleep_governor) == -1)
    return -1;
	
  if (get_config_value("battery_temp", conf->battery_temp) == -1)
    return -1;
  if (get_config_value("battery_temp_governor", conf->battery_temp_governor) == -1)
    return -1;
  if (get_config_value("battery_temp_min_freq", conf->battery_temp_min_freq) == -1)
    return -1;
  if (get_config_value("battery_temp_max_freq", conf->battery_temp_max_freq) == -1)
    return -1;
	
  if (get_config_value("battery_cap", conf->battery_cap) == -1)
    return -1;
  if (get_config_value("battery_cap_governor", conf->battery_cap_governor) == -1)
    return -1;
  if (get_config_value("battery_cap_min_freq", conf->battery_cap_min_freq) == -1)
    return -1;
  if (get_config_value("battery_cap_max_freq", conf->battery_cap_max_freq) == -1)
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

int main (int argc, char **argv)
{
  ocConfig  conf;
  pid_t pid, sid;
  char input_buffer[9];
  int asleep = 0;
  
  __android_log_write(ANDROID_LOG_INFO, APPNAME, "Starting service.");
  if (load_config(&conf) == -1)
  {          
    __android_log_write(ANDROID_LOG_ERROR, APPNAME, "Unable to load configuration. Stopping.");
    return 1;
  }
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
    
  while (1)
  {
	asleep = check_sleep();
	
	if (asleep == 1)
	{
		__android_log_write(ANDROID_LOG_ERROR, APPNAME, "Unable to get data from file. Cannot continue.");
		return 1;
	}	
    else if (asleep == 2)
    {
      __android_log_write(ANDROID_LOG_INFO, APPNAME, "Setting sleep profile.");
      set_cpu_params(conf.sleep_governor, conf.sleep_min_freq, conf.sleep_max_freq);
    }
  
	input_buffer[0] = '\0';
  
    if (read_from_file(SYS_WAKE, 6, input_buffer) == -1)
    {                  
      __android_log_write(ANDROID_LOG_ERROR, APPNAME, "Unable to get data from file. Cannot continue.");
      return 1;
    }
    if (strcmp(input_buffer, "awake") == 0)
    {
		input_buffer[0] = '\0';	
	
		if (read_from_file(SYS_BATT_TEMP, 4, input_buffer) == -1)
		{                  
		  __android_log_write(ANDROID_LOG_ERROR, APPNAME, "Unable to get data from file. Cannot continue.");
		  return 1;
		}
		if (atoi(input_buffer) >= atoi(conf.battery_temp))
		{
		  __android_log_write(ANDROID_LOG_INFO, APPNAME, "Setting heat profile.");
		  set_cpu_params(conf.battery_temp_governor, conf.battery_temp_min_freq, conf.battery_temp_max_freq);
		  
		  while (asleep != 2 && atoi(input_buffer) >= atoi(conf.battery_temp) + 15) {
			sleep(1);
			
			asleep = check_sleep();
			
			input_buffer[0] = '\0';	
			
			if (read_from_file(SYS_BATT_TEMP, 4, input_buffer) == -1)
			{                 
			  __android_log_write(ANDROID_LOG_ERROR, APPNAME, "Unable to get data from file. Cannot continue.");
			  return 1;
			}
		  }
		}
		else {
			input_buffer[0] = '\0';	
		
			if (read_from_file(SYS_BATT_CAP, 4, input_buffer) == -1)
			{                  
			  __android_log_write(ANDROID_LOG_ERROR, APPNAME, "Unable to get data from file. Cannot continue.");
			  return 1;
			}
			if (atoi(input_buffer) <= atoi(conf.battery_cap))
			{
			  __android_log_write(ANDROID_LOG_INFO, APPNAME, "Setting capacity profile.");
			  set_cpu_params(conf.battery_cap_governor, conf.battery_cap_min_freq, conf.battery_cap_max_freq);
			  while (asleep != 2 && atoi(input_buffer) <= atoi(conf.battery_cap)) {
				sleep(1);
				
				asleep = check_sleep();
				
				input_buffer[0] = '\0';	
				
				if (read_from_file(SYS_BATT_CAP, 4, input_buffer) == -1)
				{                  
				  __android_log_write(ANDROID_LOG_ERROR, APPNAME, "Unable to get data from file. Cannot continue.");
				  return 1;
				}
			  }
			}
			else
			{
				__android_log_write(ANDROID_LOG_INFO, APPNAME, "Setting awake profile.");
				set_cpu_params(conf.wake_governor, conf.wake_min_freq, conf.wake_max_freq);
			}
		}
    }
  }
  return 0;
}
