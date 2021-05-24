# EU4 Time Notifier

A notification system for the Paradox game Europa Universalis 4

## What do I do

I read EU4 memory to find the date in memory. Then keep track of the date, and allow people to add notifications to a given date. Currently I only support audio clues, meaning a sound will be played when the user-set date hits.

## How to use

TLDR: Watch this [Quick Guide YouTube Video](https://youtu.be/aycPIAfqn1s )

When you this software, you should be greeted with a screen like this one: 

![](https://i.imgur.com/lLs8hND.png)

Here you have a few options, but to keep it simple, the "submit" button is the only important one. However, we need to start the actual game (Eu4) before I can do anything useful. Once Eu4 is booted, and a save is loaded/started, enter the current date of the game into here: 

![](https://i.imgur.com/TARYF9R.png)

Then a search of that date will happen, this will take 2-5 seconds depending on your hardware. Once the date has been found, you will most likely be greeted with a message saying "Enter a different EU4 Date:" 

![](https://i.imgur.com/FmSXtpr.png)

If so you should just need to un-pause the game and let the date tick one day, and enter that date into the Eu4-Notifi application ![](https://i.imgur.com/d3r1PEW.png). Once this is done the Eu4-Notifi should look like this: 

![](https://i.imgur.com/4fSHEcc.png)

Now "Current Eu4 Date" will display the ingame date when you go back to the game and continue playing, it will update accordingly.

Now you can use this: 

![](https://i.imgur.com/NLs9gA9.png)

to add notification that plays when that date is reached. Using the -/+ button, drawdown menu, or simply typing it in.

## Build

Requires: [CMake](https://cmake.org/download/) and a C++ Compiler (Tested: MSVC 14.29, Clang 12, GCC 10).

Open your favorite terminal/shell, navigate to where you want to the folder have the `Eu4_TimeNotifi folder, then one after the other write the following commands:

```bash
git clone https://github.com/Togtja/Eu4_TimeNotifi
cd Eu4_TimeNotifi/
mkdir build
cd build/
cmake ../
```

## Dependency Libraries

The 3rd party libraries that EU4 Time Notifier depends on

### Included in the build

[Dear ImGui](https://github.com/ocornut/imgui)
Used for the UI

[glad](https://github.com/Dav1dde/glad)
The OpenGL Loading Library I went with

[stb](https://github.com/nothings/stb)
Used to be able to load png/jpg images to OpenGL textures

### Cmake extracted (CMake fill fetch these from their repos)

[OpenAL soft](https://github.com/kcat/openal-soft)
Used to be  able to fetch your audio driver and play sounds

[libsndfile](https://github.com/libsndfile/libsndfile)
Used for reading wav file to load in to OpenAL-soft. I want to be able to support more audio formats than just `.wav`

[GLFW](https://github.com/glfw/glfw)
A simple API for OpenGL to make windows and stuff

[spdlog](https://github.com/gabime/spdlog)
Used for logging messages cleanly and faster than std::cout

### [OpenGL](https://www.opengl.org/) is not included

However most system/graphics driver has at least a version of OpenGL

## Todo/Missing features

- [ ] Write this read me
- [ ] Spend some time improving the UI
