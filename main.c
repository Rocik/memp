/* Copyright 2015~2017 Rocik
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Created on September 15, 2015, 3:37 PM
 */
#include <stdlib.h>
#include <dirent.h>   // To check if directories exists
#include <sys/stat.h> // To create directory

// To read home folder
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <gtk/gtk.h>

#include "notification.h"

#define MEM_MAX_ALLOWED 80

struct Memory {
	unsigned int maxAllowed;
	unsigned int maxAllowedCritical;
	unsigned int total;
	unsigned int free;
	float percent;
};


int readFromFile(char *filename, char *_sscanf)
{
	int var = -1;
	char line[256];
	
	FILE *file = fopen(filename, "r");
	if(file == NULL)
		g_warning("File %s has not been found", filename);
	else {
		while (fgets(line, sizeof(line), file))
		{
			if(sscanf(line, _sscanf, &var) == 1)
				break;
		}
		fclose(file);
	}
	
	return var;
}


pid_t createMessage(char *t, char *m, struct Memory mem)
{
	// this strange value is 1 divided by 1024
	sprintf(t, "Low Memory Space (%.0f MiB)", mem.free * 0.000976563);
	
	int memProcMax = 0;
	pid_t PID = 0;
	
	DIR *dir;
	struct dirent *ent;
	char procDir[32];
	if ( (dir = opendir("/proc/")) != NULL ) {
		while ( (ent = readdir(dir)) != NULL ) {
			int _PID = atoi(ent->d_name);
			if (_PID != 0) {
				sprintf(procDir, "/proc/%i/status", _PID);
				int memProc = readFromFile(procDir, "VmRSS: %d kB");
				
				if (memProc > memProcMax) {
					memProcMax = memProc;
					PID = _PID;
				}
			}
		}
		closedir (dir);
	} else {
		g_error("/proc/ directory has not been found... WTF? are you even using linux?");
	}
	
	if (PID == 0)
		sprintf(m, "%.1f%% of the memory is being used", mem.percent);
	else {		
		sprintf(procDir, "/proc/%i/comm", PID);
		char procName[128];
		FILE *proc = fopen(procDir, "r");
		if ( fgets(procName, sizeof(procName), proc) == NULL )
			g_warning("fgets error for file %s", procDir);
		fclose(proc);
		
		for(int i=0; procName[i] != 0; ++i) {
			if (procName[i] == '\n')
				procName[i] = 0;
		}
		
		sprintf(m, "%.1f%% of the memory is being used. Highest usage: %s - %.0f MiB", 
				mem.percent, procName, memProcMax * 0.000976563);
	}
	
	return PID;
}


gboolean eachSecond(void *data)
{
	static gboolean isNotifyActive = FALSE;
	// Once it get critical it will stay as long as notify is active
	static gboolean isNotifyCritical = FALSE;
	static int sleepCount = 0;
	static NotifyNotification *notif = NULL;
	
	struct Memory mem = *(struct Memory*)data;
	
	mem.free = readFromFile("/proc/meminfo", "MemAvailable: %d kB");
	mem.percent = 100 - (float)mem.free / (float)mem.total * 100.0;
	
	char title[32], message[128];
	if (mem.percent > mem.maxAllowed) {
		if (mem.percent > mem.maxAllowedCritical) {
			if (isNotifyCritical != TRUE) {
				sleepCount = 0;
				isNotifyCritical = TRUE;
				if (!isActiveNotify())
					isNotifyActive = FALSE;
			}
			
			setNotifyImportance(1);
		} else
			setNotifyImportance(0);

		if (isNotifyActive == FALSE) {
			isNotifyActive = TRUE;
			pid_t PID = createMessage(title, message, mem);
			notif = sendNotify(title, message, &PID);
		} else if (isActiveNotify()) {
			pid_t PID = createMessage(title, message, mem);
			updateNotify(notif, title, message);
		} else
			sleepCount = 60;
	} else {
		if (sleepCount > 0) 
			sleepCount--;
		else {
			if (isActiveNotify())
				closeNotify(notif);

			if (mem.percent > mem.maxAllowed*0.9) {
				isNotifyActive = FALSE;
				isNotifyCritical = FALSE;
			}
		}
	}
	
    return TRUE;
}


int main(int argc, char **argv)
{
	gtk_init(&argc, &argv);
	
	if (!notify_init("Memory Protector")) {
		g_error("Failed to initialize libnotify.");
		return EXIT_FAILURE;
	}
	
	char configFilePath[64]; // 32 for user name (2 not needed)
	sprintf(configFilePath, "%s/.config/memp", getpwuid(getuid())->pw_dir);
	mkdir(configFilePath, 0755);
	
	sprintf(configFilePath, "%s/.config/memp/memp.conf", getpwuid(getuid())->pw_dir);
	
	struct Memory mem;
	
	mem.maxAllowed = readFromFile(configFilePath, "maxMemAllowed=%i");
	if (mem.maxAllowed == -1) {
		mem.maxAllowed = MEM_MAX_ALLOWED;
		
		FILE *file = fopen(configFilePath, "w");
		if (file == NULL)
			g_warning("Couldn't open configuration file (%s). This user might have insufficient permissions.", configFilePath);
		else {
			fprintf(file, "maxMemAllowed=%i\n", mem.maxAllowed);
			fclose(file);
		}
	}
	
	//mem.maxAllowedCritical = (mem.maxAllowed + 10) > 98 ? 98 : (mem.maxAllowed + 10);
	mem.maxAllowedCritical = 90;
			
	mem.total = readFromFile("/proc/meminfo", "MemTotal: %d kB");
	if (mem.total == -1) {
		g_error("Failed to load total memory.");
		return EXIT_FAILURE;
	}
	g_timeout_add_seconds(1, eachSecond, &mem);
	gtk_main();

	notify_uninit();
	return EXIT_SUCCESS;
}
