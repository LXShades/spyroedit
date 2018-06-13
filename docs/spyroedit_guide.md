# SpyroEdit!
SpyroEdit is an old PS1 emulator plugin I made specifically for Spyro 2 and 3 many years back. Initially made for simplifying small hacking tasks, like finding secret butterfly jars, finding hidden animations and tinkering with game objects, it has since expanded greatly to include some pretty nifty features such as custom level textures, wacky powers like the Headbashpocalypse (with or without a death wave of doom, or a doom wave of death, yep even the name is customisable! Well not really), object extraction and viewing in full 3D via Genesis, and limited level editing via Genesis.  

# Setting up
As a graphics plugin, SpyroEdit should be compatible with most plugin-based emulators. However some later versions of ePSXe, beyond version 1.6.0, may have problems on some systems. I can guarantee ePSXe 1.6.0 and PCSX should work.  

Before you can run SpyroEdit you must supply it with a real graphics plugin. To do this, copy and paste your favourite graphics plugin in the plugins folder. Rename the copy to Spyro3GPU (.dll if extensions are visible). Place gpuSpyroEdit alongside it in the plugins folder.  

If all goes well, when you run your emulator SpyroEdit 2.1 should now show up in its plugin settings. Check SpyroEdit’s config: if your Spyro3GPU file is loaded successfully, the config button should take you to that plugin’s configuration popup. You’re all set!  

**Warning**: Some users experience an odd fatal error ‘cannot load GPU plugin’. To alleviate this try any or all of the following: a) rename gpuSpyroEdit to something shorter, such as just gpu; b) make sure a firewall or AV isn’t blocking SpyroEdit from running, c) running the emulator in Administrator mode, or d) redownloading SpyroEdit.  

# Main interface
## Objects
The Objects tab enables you to view and edit various numbered parameters of the objects in the environment. Each parameter can be edited as either an integer (normal number) or hex number.  

Probably the best way to learn what each parameter means (some are object type-specific) is to find an object of interest, such as an enemy or NPC, hit the Find Nearby Object button, identify the object of interest and check the Auto-update checkbox. If you’re not sure which object you’re looking at, one way to check is to increment its Z by maybe 1000, and see which object moves up.

**Object ID:** This is the 0-based number of the object you’re currently editing. Every object has an ID. Some objects are ‘dead’: Objects with a negative ‘state’ parameter are usually considered deleted and may eventually be replaced by a new object. It’s not recommended to destroy objects this way however due to a possible (emulated) memory leak.
**Set values:** Applies any changes you’ve made to the object.
**Get values:** Retrieves information about the object’s state
**Auto-update:** Automatically updates the parameters, except for the last parameter that you edited (this is to allow you to change it without it constantly being replaced).
**Drag with Spyro:** Tick this box to force the object to follow you.
**Next type:** [Broken] May check the list of available object types and change the object’s type to the next.
**Prev type:** [Broken] Ditto, but for the previous type.
**Find Nearby Objects:** Shows a messagebox displaying all objects close to Spyro. The format shown is [Object ID]: Type [Object type in decimal]/[Object type in hex] ([Alive or Dead])

## MultiSpyro
This tab allows you to connect to another player and see them. To do this, two things are needed:  

* One player must be the Host, which, if behind a router, requires that they forward UDP port 2012  
* You must have the Host’s IP  

Most network setup problems are the result of routers and firewalls. Make sure they are all configured appropriately when trying to use this mode.  

**IP box:** If Joining, insert the IP address of the host here. If hosting, you can leave this.  
**Host:** Host the session.
**Join:** Join the session at the IP address in the box above.

In MultiSpyro, you can see other players and some of their powers. As a warning most physical actions are entirely unsynchronised. The effect for the most part is purely visual and doesn’t amount to a full multiplayer Spyro experience--unless you’re creative.  

## Powers
Powers tab! If you're playing Spyro 2 or Spyro 3 (with some exceptions), you can use powers!  

There is no guide to powers. It is your destiny alone. Experiment and Be the Chosen One.  

Powers marked ‘S3’ work on Spyro 3 only. Powers marked with ‘S3PAL’ work on Spyro 3 PAL only.  

## Scene (How-to) ##
This is where you can edit the textures, skies and colours of a level. In this version of SpyroEdit, files are stored in the folder SpyroEdit/[Name of level]. Individual textures are stored in SpyroEdit/[Name of level]/Textures [HQ/LQ].  

### Textures ###
Editing Spyro textures is as simple as saving the textures, locating them in their level folder, and then loading them. Make sure the game is unpaused while you do either of these, as pausing the game can overwrite the VRAM.  

Texture editing should be done with caution for a few reasons. One is that the textures are actually split into four tiles each, all using 256-colour palettes. Firstly, some tiles may be shared between multiple textures, possibly flipped or rotated. Secondly, the palettes too can be shared randomly between textures. This is all at the game’s discretion, in the name of saving memory. You can view what’s happening in detail with the texture viewer (see Open viewer below)  

When it comes to palettes, SpyroEdit tries to do most of the work for you. It can regenerate the palettes for replaced textures, updating them with the same shared palettes they already used in-game (Generate palettes). Optionally, it can even go a little further, moving all textures into optimal palettes (Shuffle palettes). This has decent but imperfect results. Finally, the Spyro games have both high-quality or low quality textures, and it’s best to let SpyroEdit deal with them automatically (Auto LQ).  

When saving or loading textures, SpyroEdit now lets you choose whether to save/load them separately or as one image. You can change this with the Separate bitmaps checkbox.  

Some textures, such as moving textures, may tend to suffer from glitches no matter how you try to replace them. To alleviate this, scribble absolute green (RGB: 255, 0, 0) into any offending textures. Textures with 1/10th of this colour will be registered ‘ignored’, and SpyroEdit will preserve their original state as a workaround.  

Most problematic textures tend to appear blank. Sometimes these may even be the blank textures at the very bottom of the texture bitmap, so if you’re getting errors try scribbling out that area too.  

The Open viewer button opens the texture viewer window. To hide it again, just close the window. The viewer shows you which textures are using which palettes. Hold the left button and move the mouse up or down to scroll through the texture palettes on the left side. Hover over any texture, each divided into four tiles, to see its relationships to other texture tiles. For other tiles that share the same palette, a red box is shown. For other tiles which entirely reuse the same tile, a blue tile is shown. It is recommended to close the viewer during play as it can be quite slow!  

After making major changes to textures, I recommend hitting the Apply level colours button in the Colours section. This will refresh the texture fade-out colours, which are baked into the polygons, based on the new texture colours.  

**Quick load:** By the way, you can instantly load skies by dragging a compatible .sky file over the SpyroEdit window.  

### Sky ###
Saving and loading skies with Spyro is relatively easy with a few catches:

* Due to memory constraints, you **should not load a bigger sky into a level with a smaller sky**, speaking in terms of file size. SpyroEdit will allow it, but with no guarantees!  
* Some skies appear physically broken when loaded into a different level. Try the **Disable sky occlusion** checkbox, which may fix the issue at a slight framerate penalty.  
* Spyro 2 and 3 skies are compatible with each other. However Spyro 1 skies are only compatible among each other.  

### Colours ###
The aesthetics of the Spyro levels are hugely influenced by polygon colours. These are used for lighting among other things. Unfortunately due to Spyro’s great use of saturation, editing level colours in general is a fiddly process. One thing to bear in mind that every change you make to the sky may sacrifice quality. This used to apply to the Level tweak as well, but the latter is no longer relative and is safe to change or restore repeatedly.  

The three-box parameters for tweaks are red/green/blue factors in percentage. 100 means 100% of the original colour channel, i.e. no change. It’s allowed and safe to go beyond 100% if it’s in your interests. The three-box parameter for average fog is a colour modifier, where grey (127,127,127) makes the distant parts of the level fade out to roughly their original colour.  

**Avg. fog:** Determines fade-out colours for objects in the distance, i.e. fog. By default, this is 127,127,127, which basically means no change from the original colours. The Make button allows you to visually define the fog colour.  
**Light tweak:** Global R,G,B percentage multipliers for the whole level. The Make button is a work-in-progress.  
**Sky tweak:** Global R,G,B percentage multiplier for the sky. The make button is a work-in-progress  

### Autoload ###
Autoload on level entry: If checked, SpyroEdit will load the textures, colours, etc from a level’s folder when you enter it.  

## Genesis ##
Ah, the oversaturated Genesis tab! This is what you use to connect with Genesis.  

I don't words.  

**Connect:** Connects to Genesis, usually locally. (It may also be possible to connect over the Internet, but it's been a long time since I've touched that!)  
**Send Scene:** Sends the level geometry to Genesis.  
**Reset Scene:** Tries to revert the scene to its original state when you entered the level.  
**Send Spyro:** Not used.
**Send Collision:** Sends the collision mesh to Genesis. The collision mesh is unaffected by any edits you make.  
**Send Object:** Sends the model of the currently selected object in the Objects tab, on its current animation frame.  
**Send All Objects:** Sends the models and positions of every living object in the level.  
**Rebuild Colltree:** Refreshes the collision octree. SpyroEdit usually does this automatically.  
**Regen Colltris:** SpyroEdit will attempt to regenerate the entire level's collision by assigning all collision triangles to as many scenery polygons as possible. Lower places are prioritised; higher places may be missing triangles. This may mix up portals, surfaces, etc. I only added this to make it possible to add geometry in Genesis without clipping through it. All in all, an unrecommended, Big, Shiny Red Button. Do Not Press  

## Status ##
This tab lists all of the required game memory addresses (which may be usable in GameShark codes) detected by SpyroEdit.  

Open VRAM viewer is a debugging function that lets you see the game’s VRAM in multiple colour formats. The first one is in the top-left corner, 1024x512 large, which displays the VRAM in 16-bit colour format. This is the main one, and the only format where colour palettes are visible. The second one is colour area to the right of the first. This is 1024x512 large also, and displays pixels in 8-bit colour according to the active palette. The third one is at the bottom, this one is a massive 2048x512, and displays pixels in 4-bit colour according to the active palette.  

The other two formats are shown to the right and below, but first you need to have an active palette in order to see anything useful. If you know where a palette is, hold the Space button and hover the mouse over the palette in the 16-bit view. This will become the active palette. As you move the mouse around, you’ll see the colours changing in the 8-bit and 4-bit sections of the window. Most of these will be pixel vomit, but if you’ve hovered over a valid palette, the texture you’re looking for is there somewhere!  

This feature is for advanced users/hackers, so it won’t be covered in much detail, but if you’re curious, try space+hovering over one of the many distinctive 16x16 blocks where the colours fades to grey as you go down. Set the active palette to the top-left corner. Then look around the third (4-bit) area of the window and see if you can find a valid texture.  

Summary of controls:
Space (hold): Change active palette to mouse position (8-pixel-aligned)
Up/Down/Left/Right arrow: Fine-tune the position of the palette (needed for some palettes, but not most)