# SpyroEdit
Official repository for the SpyroEdit emulator plugin

# Features
* Editing Spyro  

# Compiling
The latest updated project file, SpyroEdit.vcxproj, requires Visual Studio 2017.  
C++17 recommended (I actually have no idea if I used any C++17 so far)  

# Status Quo
SpyroEdit contains some of my oldest, rustiest, dirtiest code from 2009 and beyond. It was always 'open-source' after its release, but I have only recently properly done collaborative programming as part of my university course, and it's about time that the source code finally gets a home here on GitHub.  

It needs a good cleanup, particularly in the areas where the program breaks the most. While I'm happy to see

# Collaboration
## Just Do It!
SpyroEdit has never had any collaborative work done on it before. It's an exciting prospect. 

## Target coding style
SpyroEdit is in need of a cleanup. However, this doesn't necessarily mean a total revamp of the coding style. Some things are intended to be kept to their current unconventional status quo, other things need changing. Examples of things that are likely to stay the same are:  

* The hybrid of functional and object-oriented programming  

Examples of things that should be changed include:  

* Inconsistent use of braces (unpopular opinion, but opening brace on new line ftw!)  
* Inconsistent naming conventions  
* Unnecessary code reuse  
* Unencapsulated compatibility checks between different versions (these should be somehow encapsulated for ease of use)  