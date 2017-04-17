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
 * Created on September 16, 2015, 2:51 PM
 */
#define _POSIX_SOURCE

#include <stdlib.h>

#include <signal.h>

#include <libnotify/notify.h>

GError *error = NULL;
const char *iconType;
int isNotifyActive = 0;


void killApp(NotifyNotification *notification, const char *action, gpointer user_data)
{
	int PID = *(pid_t*)user_data;
	if (PID > 0 && PID < 32768)
		kill(PID, SIGKILL);
}


void notifyClosed(NotifyNotification *notification, gpointer user_data)
{
	isNotifyActive = 0;
	g_object_unref(notification);
}


NotifyNotification *sendNotify(char *title, char *message, pid_t *p_PID)
{
	NotifyNotification *notification;
	
	notification = notify_notification_new(title, message, iconType);
	
	notify_notification_set_urgency(notification, NOTIFY_URGENCY_CRITICAL);
	notify_notification_set_timeout(notification, NOTIFY_EXPIRES_NEVER);
	
    notify_notification_add_action(notification, "button", "Kill this process",
	   (NotifyActionCallback)killApp, p_PID, NULL
	);

    if (!notify_notification_show(notification, &error)) {
		g_error("Sending notification failed: %s", error->message);
        g_error_free(error);
    }
	
	g_signal_connect(notification, "closed", G_CALLBACK(notifyClosed), NULL);
	
	isNotifyActive = 1;
	return notification;
}


void updateNotify(NotifyNotification *notification, char *title, char *message)
{
	if (notify_notification_update(notification, title, message, iconType) == FALSE) {
		g_error("updateNotify: wrong parameters");
		g_error_free(error);
	}
	
	if (!notify_notification_show(notification, &error)) {
		g_error("Updating notification failed: %s", error->message);
        g_error_free(error);
    }
}


void closeNotify(NotifyNotification *notification)
{
	if (!notify_notification_close(notification, &error)) {
		g_error("Closing notification failed: %s", error->message);
        g_error_free(error);
	}
	
	isNotifyActive = 0;
	g_object_unref(notification);
}


void setNotifyImportance(int v)
{
	static const char ICON_CRITICAL[16] = "dialog-error";
	static const char ICON_WARNING[16] = "dialog-warning";
	
	if (v)
		iconType = ICON_CRITICAL;
	else
		iconType = ICON_WARNING;
}


int isActiveNotify()
{
	return isNotifyActive;
}
