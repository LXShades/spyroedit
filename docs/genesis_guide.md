# Genesis
Genesis was a 3D modeller project I made, originally intended for game development. It has basic support for hard-model editing, and is largely inactive now due to other game projects.  

Save regularly! I understand Genesis can be very broken at times. On the other hand, I don't want to withold it from fans anymore. Originally I wanted to release it when it was finished and fully working--but that might never happen now. For what it is however, it's quite functional, and I look forward to seeing what you all do with it. Have fun!  

# Getting started
Firstly, if you're reading this, you're most likely here exclusively to use it with SpyroEdit! So here's a quick guide in that context:  

0: Extract Genesis into a folder  
1: Start up Genesis  
2: Start up the latest Genesis-compatible version of SpyroEdit in your favourite emulator  
3: Run Spyro 2 or 3. Spyro 1 may work too, but don't expect it to work reliably  
4: Enter a level in the game  
5: On the Genesis tab in SpyroEdit, click Connect. If all goes well, you should see some debug text in Genesis.  
6: Click the Send Scene button. The scene should start downloading into Genesis  
7: Refer to the controls below to browse the scene!  
8: Hit the 5 key. Click on a part of the level you want to edit. Don't try to move it with just yet!  
9: Edit the segment. The following keys will set the editor to do the following:  
 1: Edit the segment's vertices (points)  
 2: Edit the segment's edges (lines)  
 3: Edit the segment's faces (polygons)  
 5: Select a different segment(s). You can edit multiple segments at once at your own risk. Try it out!  
10: Use the following keys to change the tool to edit stuff with/gizmo. (You can read more about this below)  
 W: Move  
 E: Scale  
 R: Rotate  

# Tips:
- It's recommended to **only move** elements. Creating or destroying elements such as polygons can sometimes work, but often with unexpected side-effects, including broken collision and an inability to reload levels.  
- **There is no Save/Load** You can -kind of- load changes by connecting up the level, loading it in Genesis, then making changes to update it on the game's side.  
- **Polygons with more than four sides will be invisible.** Reduce them to rectangles or triangles to fix this.  
- **Send Collision** can be used to view collision triangles, but any changes you make **won't take effect**. This means you can move it away from the main level model and see if there are any holes.  
- Collision polygons **have a size limit**. If you make a polygon notably bigger than normal, the collision may not work until you shrink it back.  
- Level sections **have a size limit**. If you make it too big, SpyroEdit simply won't be able to fit all the polygons in the space. This may distort it, so be wary!  
- **SAVE REGULARLY.** This is a crash-prone app. Quality crash material. Autosave.gen is automatically saved regularly in the root folder of Genesis. If Genesis crashes, see if that file has a surviving version of your work.  

# Using Genesis: Terminology
## Objects ##
**Mesh:** A 3D model made of vertexes, edges, or faces  
**Vertex:** A point in a mesh  
**Vertices:** Awkward plural for vertex  
**Edge:** A line connecting two points in a mesh, usually forming a Face  
**Face:** Solid, flat part of a mesh--also called a polygon  
**Element:** A vertex, edge, face or instance, depending on the mode  
**Gizmo:** The colourful 3D tool for moving, rotating or scaling elements  

## Concepts ##
**Modifier:** A type of tool that can be used for a variety of things, automatic or user-controlled  
**LiveGen:** The name of the system used by Genesis and compatible apps to enable real-time editing or viewing  
**Transform:** A broad word describing a movement, rotation, scale, warp, or more advanced systematic point movements

# Modifiers
**Edit Mesh:** Main modifier for editing meshes in a hard-surface-oriented manner
**UV Edit:** Main modifier for editing UV coordinates (texture coordinates) on meshes

# Using Genesis: Controls
## Camera ##
**Left button (hold):** Select area  
**Middle button (hold):** Rotate camera around centre point  
**Right button (hold):** Pan camera/move centre point  
**Scroll wheel:** Zoom in/out  
**Z:** Center camera on selection  
**Return:** Toggle perspective/orthographic mode  

## Camera flicks ##
**Numpad 2/4/6/8:** (imagine 5 is the top of a character looking towards you): Front, Left, Right, Back view  
**Numpad 1/3/7/9:** (imagine 5 is the top of a character looking towards you): Quarter angles  
**Numpad 5:** Top view  
**Numpad 0:** Raise 45-degrees  

## Transform modes ##
**W:** Move  
**E:** Rotate  
**R:** Scale  

## Selection scopes ##
**1:** Vertex  
**2:** Edge  
**3:** Face  
**4:** ???  
**5:** Instance  

## Selection modes (hold while selecting) ##
**Shift:** Add selections to area  
**Alt:** Remove selections from area  
**Ctrl:** Toggle area selection  

## Gizmo ##
**Left button (hold):** Move, rotate or scale along highlighted axis  
**Alt (hold):** While transforming, snaps transform to a hard multiple of a unit, such as 45 degrees or 1 metre  
**G:** Lock/unlock gizmo (selections will take precedence)  

## General editing ##
**Backspace:** Soft delete element (element is destroyed, but associated elements are preserved and re-linked if possible)  
**Del:** Hard delete element (element and associated elements are destroyed)  
**Ctrl+Z:** Undo and pray it doesn't crash  
**Ctrl+Y:** Redo and pray it doesn't crash  