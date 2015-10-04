# Memory Protecion

It is in fact system protection from memory overflow.
If you are this type of person who doesn't want swap, consider using this application.

# How it works

When the amount of memory used exceeds certain value a notification is shown.
It includes a button to kill a process with the highest memory usage.

# Configuration

> $USER/.config/memp/memp.conf

```sh
maxMemAllowed=80 # minimum memory usage in percents to trigger the notification
```

# Dependencies

libnotify

Ubuntu: 
```
sudo apt-get install libnotify-dev
```

Arch:
```
sudo pacman -S libnotify
```
