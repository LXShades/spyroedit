# SpyroEdit
SpyroEdit is a specialised emulator plugin for modifying the Spyro the Dragon games on the PlayStation 1. It's compatible with Windows emulators such as ePSXe and PCSX.  

# Features
* Edit level textures and colours!
* Replace level skies with skies from different levels! (Spyro 2 and 3 cross-compatible!)  
* Give Spyro cool and unruly powers, such as the Headbashpocalypse!  
* Edit object properties in-game!  
* Edit scenery and object positions with Genesis!  

Officially compatible with Spyro 2 and 3, with limited compatibilty for Spyro 1.

# Compiling
The latest project file, SpyroEdit.vcxproj, requires Visual Studio 2017.  
C++17 recommended (I actually have no idea if I used any C++17 so far)  

# State of the Code
SpyroEdit contains some of my oldest, rustiest, dirtiest code from 2009 and beyond. It was always 'open-source' after its release, but I have only recently properly done collaborative programming as part of my university course, and it's about time that the source code finally gets a home here on GitHub.  

It needs a good cleanup, particularly in the areas where the program breaks the most. Any clean-ups made to the code while working on new features is welcomed so long as they come at no performance penalty. This is how I've been working the past few weeks.  

On that note, I very much prefer that features are added firstly and cleaned up secondly. This means contributions that solely refactor the code into a 'cleaner' style, but don't add any features, are at risk of rejection if I feel the style differs too much from that which a feature contributor (including myself) uses. Sure our code might be messy, but we work best in familiar environments, and we're the ones adding features!  

# Collaboration
## Just Do It!
SpyroEdit has never had any collaborative work done on it before. It's an exciting prospect. If you've made something cool, do a pull request!  

## Target coding style
SpyroEdit is in need of a cleanup. However, this doesn't necessarily mean a total revamp of the coding style. Some things are intended to be kept to their current unconventional status quo, other things need changing. Examples of things that are likely to stay the same are:  

* The hybrid of functional and object-oriented programming  

Examples of things that should be changed include:  

* Inconsistent use of braces (unpopular opinion, but opening brace on new line ftw!)  
* Inconsistent naming conventions  
* Unnecessary code reuse  
* Some components could use their own class - for example, the collision  