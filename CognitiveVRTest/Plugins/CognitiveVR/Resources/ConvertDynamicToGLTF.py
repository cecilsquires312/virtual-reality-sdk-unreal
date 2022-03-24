import bpy
import mathutils
import math
import bmesh
import sys
import os
import shutil
import os.path

#arguments
scene = bpy.context.scene
ops = bpy.ops
args = sys.argv

exportPath = args[3]

dynamicNames = args[4].replace('\'','').split(",")

sizeFactor = 1;

#foreach dynamic object
#just export into existing folder
#delete bmps and folders

diffuse_ext='_D.bmp'
other_img_ext='.bmp'
dynamics=[]

subdirectories = [d for d in os.listdir(exportPath) if os.path.isdir(os.path.join(exportPath, d))]
#onlyfiles = [f for f in os.listdir(exportPath+"/"+dir) if os.path.isfile(os.path.join(exportPath+"/"+dir, f))]
print("==================================ROOT " + exportPath)

for dir in subdirectories:
    #if dynamicNames does not contain dir, that's not a dynamic we want to modify and should be skipped
    if dir not in dynamicNames:
        print("skip dir " + dir)
        continue

    workingdir = exportPath+"/"+dir
    print("==========working directory "+exportPath+"/"+dir)
    
    #clear scene
    for ob in scene.objects:
        ob.select_set(True)
    ops.object.delete()
    print("=============================================deleted stuff")
    
    mtlpath = ''
    for tempPath, dirs, files in os.walk(workingdir):
        for name in files:
            print (tempPath + "  " + name)
            if name.endswith('.mtl'):
                mtlpath = os.path.join(tempPath, name)
    #foreach file in working directory
    #move image to root? do i need to?
    #add space at beginningl of mtl
    if (mtlpath != ''):
        mo = open(mtlpath, encoding='utf-8-sig')
        readString = mo.read()
        outstrings=[]
        outstrings.append('\n\n')
        #==========================replace mtl with png references to textures
        for line in readString.splitlines():
            outstrings.append(line+'\n')
        mo.close()
        
        #remove the mtl
        os.remove(mtlpath)
        
        #write to new file (pngs)
        nmo = open(mtlpath, 'w+', encoding='utf-8-sig')
        nmo.writelines(outstrings)
        nmo.close()
    
    
    #import fbx or obj. whatever is there
    objname = workingdir + "/" + dir + ".obj"
    fbxname = workingdir + "/" + dir + ".fbx"
    if os.path.exists(objname):
        ops.import_scene.obj(filepath=objname, use_edges=True, use_smooth_groups=True, use_split_objects=True, use_split_groups=True, use_groups_as_vgroups=False, use_image_search=True, split_mode='ON', axis_forward='-Z', axis_up='Y')
        print("=============================================import obj")
    if os.path.exists(fbxname):
        ops.import_scene.fbx(filepath=fbxname)
        print("=============================================import fbx")
    
    #[0] is type, [1] is material name, [2] is diffuse, [3] is normal, [4] is opacity
    #OPAQUE|materialname|path/to/diffuse.bmp|path/to/normal.bmp
    #if materials.txt exists, re-create materials based on the contents
    #find materials by name, set rendering type, set/import images
    jsonMatName = workingdir + "/materiallist.txt"
    if os.path.exists(jsonMatName):
        print("load materials from list")
        #just write lines. no fancy json stuff
        mo2 = open(jsonMatName)
        readString2 = mo2.read()
        for line2 in readString2.splitlines():
            split = line2.split('|')
            print (line2)
            if len(split) < 3:
                print("too short!")
                continue
            for mat in bpy.data.materials:
                if mat.name == split[1]: #found the material. now import/set textures from next parts of split
                    prin = mat.node_tree.nodes["Principled BSDF"]
                    imgnode = mat.node_tree.nodes.new("ShaderNodeTexImage")
                    mat.node_tree.links.new(prin.inputs['Base Color'],imgnode.outputs[0])
                    tex = bpy.data.images.load(split[2])
                    imgnode.image = tex
                    #TODO normal map, opacity/mask map
                elif mat.node_tree is None:
                    print(mat.name + "  has null node tree")
        mo2.close()
    
    #redaw preview, just to show that something is happening
    bpy.ops.wm.redraw_timer(type='DRAW_WIN_SWAP',iterations=1)
    
    #export as gltf to workingdir+"/"+dir
    gltfname = workingdir + "/" + dir + ".gltf"
    ops.export_scene.gltf(export_format='GLTF_SEPARATE',export_animations=False,filepath=gltfname)
    
    #save blend file for debugging
    #blendname = workingdir + "/" + dir + ".blend"
    #ops.wm.save_mainfile(filepath=blendname)
    
    #remove all bmps, mtl and obj
    for tempPath, dirs, files in os.walk(workingdir):
        for name in files:
            print (tempPath + "  " + name)
            if name.endswith('.mtl'):
                os.remove(os.path.join(tempPath, name))
            if name.endswith('.obj'):
                os.remove(os.path.join(tempPath, name))
            if name.endswith('.fbx'):
                os.remove(os.path.join(tempPath, name))
            if name.endswith('.bmp'):
                os.remove(os.path.join(tempPath, name))
            if name.endswith('.txt'):
                os.remove(os.path.join(tempPath, name))
        for dir in dirs:
            print ("remove " + os.path.join(tempPath, dir))
            #os.remove(dir)
            shutil.rmtree(os.path.join(tempPath, dir))
print("ALL DONE")
exit()