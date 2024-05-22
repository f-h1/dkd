![Cover image](https://github.com/f-h1/dkd/blob/master/front.jpg?raw=true)
# Dedicated Keyboard Daemon (DKD)

Dedicate another physichal keyboard solely to key-bindings without conflicting with main-keyboard.

All keys on the dedicated keyboard are routed to DKD, in turn matching them against predefined key-bindings to execute corresponding shell command.

Linux.

## Use case

Connect 2 keyboards to your computer. Then the key `A` on *keyboard 1* does one thing, and key `A` on *keyboard 2* does another thing.

Specifically: You use your main keyboard as usual for typing and your ordinary key-bindings. You use the second keyboard only for additional keybindings, not for typing.

You can add as many keyboards as you want.

Doesn't interfere with other key-binders set for your other keyboard(s).


## Support
Supports combination of keys.

## Limitations
The keyboard bound to this daemon can not be used to type with as you do with your main keyboard. All input goes to the DKD and it responds only to the keys you have bound in the config-file.

No support for chorded key bindings, where multiple keys must be pressed in sequence to trigger a command.

## How it works
Using *libevdev*, this utility grabs the target keyboard device. This prevents other applications (including the X server) from receiving input from this device. All input is mapped against the config file, and executes accordingly.

A blocking key-binder, executing shell-commands in non-blocking mode.


### Reflections
The non-blocking while loop expectedly spiked my cpu, being above things like chromium web browser. Thus this is used: `libevdev_next_event(dev, LIBEVDEV_READ_FLAG_BLOCKING, &ev)`.

## Options



```
Usage: ./dkd -c <config file> -i <input device> [-v]
```

`-c` path to your config file.

`-i` path to target keyboard device to occupy and read from.

`-v` verbose, for debug purpose.

## Config file syntax

Currently, use the key name given by `evtest`.

```
[KEY NAME]: [YOUR SHELL COMMAND]
[KEY NAME]+[KEY NAME 2]: [YOUR SHELL COMMAND 2]
[KEY NAME]+[KEY NAME 2]+...: [YOUR SHELL COMMAND 2]
```

Use `evtest` to detect the name of keys.

*Your shell command* uses the shell set by `$SHELL` environment variable of the user. If the *SHELL* environment variable is not set, it defaults to `/bin/sh`.

The command can be any shell command; a script of yours, any command or sequence of commands.


Example:
```
KEY_LEFTALT+KEY_LEFTCTRL+KEY_C: echo "Hello"


KEY_A: echo "World!"
```

## Bugs
* Pressing CTRL+A triggers a binding set for only A.

## TODO
* Remove the need in the config-file to type KEY_..., just CTRL, A, B, SHIFT etc.
* Comments in config file, like # ...


## Install

You need to compile and then find the device file and if you want it to connect to the same device on reboot, you need to create an udev rule and symlink.

### Requires

Requires `libevdev`, and you will benefit from `evtest` to set it all up.

If you are using Arch Linux:
```
sudo pacman -S evtest
sudo pacman -S libevdev
```

### Compile

Compile with gcc, and if you get:

```
dkd.c:4:10: fatal error: libevdev/libevdev.h: No such file or directory
    4 | #include <libevdev/libevdev.h>
      |          ^~~~~~~~~~~~~~~~~~~~~
compilation terminated.
```

... you need to find where yuor `libevdev.h` is placed.

My `livevdev.h` installed as `/usr/include/libevdev-1.0/libevdev/libevdev.h`

so to compile, I do: 

```
gcc -I/usr/include/libevdev-1.0 dkd.c -levdev -o dkd
```

To find the path of `libevdev.h`, do

```
sudo find / -name libevdev.h
```

### Setting up

Assuming you compiled successfully, you now have the executable `dkd`.

```
>dkd
Usage: ./dkd -c <config file> -i <input device> [-v]
```

#### Finding the input device

You can find devices by `evtest`, but you may run into problems. In my case, my target keyboard is the "Razer..." which is listed several times with identical name. By trying all of them, I discover that `event5` responds when I presses its keys.

```
>sudo evtest
No device specified, trying to scan all of /dev/input/event*
Available devices:
/dev/input/event0:	Power Button
/dev/input/event1:	Power Button
/dev/input/event10:	Razer Razer Huntsman Elite Keyboard
/dev/input/event11:	Razer Razer Huntsman Elite
/dev/input/event12:	Razer Razer Huntsman Elite
/dev/input/event13:	HD-Audio Generic Mic
/dev/input/event14:	HD-Audio Generic Mic
/dev/input/event15:	HD-Audio Generic Headphone Front
/dev/input/event16:	HD-Audio Generic Front Headphone Surround
/dev/input/event17:	CX 2.4G Receiver
/dev/input/event18:	CX 2.4G Receiver Mouse
/dev/input/event19:	CX 2.4G Receiver
/dev/input/event2:	PC Speaker
/dev/input/event20:	CX 2.4G Receiver Consumer Control
/dev/input/event21:	CX 2.4G Receiver System Control
/dev/input/event3:	C-Media Electronics Inc.       USB PnP Sound Device
/dev/input/event4:	PC Speaker
/dev/input/event5:	Razer Razer Huntsman Elite
/dev/input/event6:	HDA NVidia HDMI/DP,pcm=3
/dev/input/event7:	HDA NVidia HDMI/DP,pcm=7
/dev/input/event8:	HDA NVidia HDMI/DP,pcm=8
/dev/input/event9:	HDA NVidia HDMI/DP,pcm=9
Select the device event number [0-21]: 
```
_However_, after reboot, the devices may be ordered differently, and it my case, it changed to `event18`. You can not trust these event numbers to stay the same for your devices.

To solve this, you create an udev rule.

### Next step: Create udev rule

Use to find right event id of the device
```	
sudo evtest
```

Test the different devices and see which one responds to key-presses on the target keyboard. Say it is `event5`.

You now need to get info unique to the device:
```
sudo udevadm info -a -n /dev/input/event5
```

In my case, the most unique i can find is this:
```
ATTRS{phys}=="usb-0000:09:00.3-1.1.3/input0"
```

In my case, my keyboard is identified as `usb-0000:09:00.3-1.1.3/input0`, and is what we take as example here.

You may also find like
```
ATTRS{uniq}=="ABC12345"
```

Now create an udev rule file
```
sudo nano /etc/udev/rules.d/99-custom-input.rules
```

Put in that file this:
```
SUBSYSTEM=="input", ATTRS{phys}=="usb-0000:09:00.3-1.1.3/input0", SYMLINK+="input/razer_keyboard"
```

If you had more unique identifying attributes, like `ATTRS{uniq`, you can do like
```
SUBSYSTEM=="input", ATTRS{phys}=="usb-0000:09:00.3-1.1.3/input0", ATTRS{uniq}=="ABC12345", SYMLINK+="input/razer_keyboard"

```

The `phys` attribute was enough for me, and `uniq` was empty in my case.

In my case, I named it `razer_keyboard`.

	
Reload udev and trigger:
```
sudo udevadm control --reload-rules
sudo udevadm trigger
```	

Verify symlink that should have been created:

```
>ls -l /dev/input/razer_keyboard
lrwxrwxrwx - root 20 May 19:30 /dev/input/razer_keyboard -> event5
```
	This should point to the correct /dev/input/event* device.

### Permissions

For DKD to have access to your input device from your user account, you need to add your user account to the `input` group. The files `/dev/input/*` are owned by the group `input`.

```
sudo usermod -aG input [YOUR USER NAME]
```

You want to start it from your user account and not from root, because (of many reasons, but also because) your user account may have things like audio set up, to run for example `paplay` to play a sound as part of a keypress.

## Starting DKD

Given the setup succeeded, you can now run DKD. I use my `razer_keyboard` symlink as example:

Create the config file named `conf`:

```
KEY_LEFTALT+KEY_LEFTCTRL+KEY_C: echo "Hello, this is the command being executed. Combination pressed."


KEY_A: echo "Hello, this is the command to execute when key A is pressed."
```

Then run:

```
./dkd -v -c ./conf -i /dev/input/razer_keyboard
```

And this will output debug info and tell you what keys you press and what commands gets executed, so you can see it works.

When all is configured, start it as an ordinary daemon from your user account, given you have added your username to the `input` group (se above).

```
./dkd -v -c ./[YOUR_CONFIG] -i /dev/input/[YOUR_DEVICE] &
```

## Make it start at login

Given you created the udev rule to make a symlink to the right device (as shown above), you can now make it start at login.

Put the starting of the daemon in your startup script where you put other key binders. I have it in my `~/.config/bspwm/bspwm-session`:

```
pgrep -x dkd > /dev/null || $HOME/src/dkd/dkd -c "$HOME/src/dkd/conf" -i /dev/input/razer_keyboard
```

Starts it only if it isn't started, but if you want to set it for several dedicated keyboards, you shouldn't do the `pgrep -x dkd > /dev/null ||` part, as you want several instances to run, one for each target device. 
