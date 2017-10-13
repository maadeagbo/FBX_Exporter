"""Interactive program to convert a customly formatted skeleton animation csv
file to the Day Dream engine's dda animation and ddb skeleton file formats.
The custom file csv format is the following:
   time,b1:posx,b1:posy,b1:posz,b1:rotw,b1:rotx,b1:rotx,b1:roty,b1:rotz,...
   0.1...,0.356...,...
   ...
"""

from math import degrees, atan2, asin
import os

"""Float values per bone: x, y, z, quaternion(w, x, y, z)"""
DATA_PER_NAME = 7
"""Conversion factor from meters to centimeters"""
M_2_CM = 100

def in_csv_name():
    """Take and verify user input for file name, return .csv filename"""
    f = input("Please input csv to convert: ")
    while f[-4:] != ".csv":
        f = input("Please input csv to convert: ")
    return f

def read_header(header):
    """Return the names of the bones in the header line"""
    headers = header.strip().strip(",").split(",")
    names = []
    for i in range(1, len(headers), DATA_PER_NAME):
        names += [headers[i].split(":")[0]]
    return names

def read_csv(csv_file):
    """Parses the data in the csv format into a list of timeframes, bone names,
    a list of position data for each bone, and a list of bone rotation data for
    each bone."""
    times = []
    poss = []
    rots = []
    with open(csv_file, 'r') as f:
        bone_names = read_header(f.readline())
        lines = f.readlines()
        for line in lines:
            data = line.strip().strip(",").split(",")
            times += [float(data[0])]
            pos = []
            rot = []
            for i in range(len(bone_names)):
                j = DATA_PER_NAME*i + 1
                pos += [[float(s) for s in data[j:j+3]]]
                rot += [[float(s) for s in data[j+3:j+7]]]
            poss += [pos]
            rots += [rot]
    return (times, bone_names, poss, rots)

def quat2euler(quaternion):
    """Converts quaternion rotation into a 3D euler angle (degrees...)"""
    #https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles#Quaternion_to_Euler_Angles_Conversion
    (w, x, y, z) = quaternion
    y2 = y * y

    t0 = +2.0 * (w * x + y * z)
    t1 = +1.0 - 2.0 * (x * x + y2)
    X = atan2(t0, t1)

    t2 = +2.0 * ( w * y - z * x)
    t2 = 1 if t2 > 1 else t2
    t2 = -1 if t2 < -1 else t2
    Y = asin(t2)

    t3 = +2.0 * (w * z + x * y)
    t4 = +1.0 - 2.0 * (y2 + z * z)
    Z = atan2(t3, t4)

    return [X, Y, Z]

"""Each index specifies that bone index's parent index. e.g. head (index 12)
has spine (index 11) as a parent."""
hierarchy_map = [
    0,#hips : root
    0,#leftupleg -> hips
    1,#leftleg -> leftupleg
    2,#leftfoot -> leftleg
    3,#lefttoebase -> leftfoot
    4,#lefttoeend -> lefttoebase
    0,#rightupleg -> hips
    6,#rightleg -> rightupleg
    7,#rightfoot -> rightleg
    8,#righttoebase -> rightfoot
    9,#righttoeend -> righttoebase
    0,#spine -> hips
    11,#head -> spine
    12,#head_end -> head
    11,#leftShoulder -> spine
    14,#leftarm -> leftshoulder
    15,#leftforearm -> leftarm
    16,#lefthand -> leftforearm
    17,#lefthandend -> left hand
    17,#lefthandthumb1 -> lefthand
    19,#lefthandthumb2 -> lefthandthumb1
    11,#rightshoulder -> spine
    21,#rightarm -> rightshoulder
    22,#rightforearm -> rightarm
    23,#righthand -> rightforearm
    24,#righthandend -> righthand
    24,#righthandthumb1 -> righthand
    26,#righthandthumb2 -> righthandthumb1
]

def export_dda(data, name):
    """converts internal time, name, position, and rotation data into dda"""
    (frame_times, names, poss, rots) = data
    framerate = 1
    if len(frame_times) > 1:
        framerate = 1/frame_times[1]
    repeat = False
    #buffer
    num_joints = len(names)
    num_frames = len(frame_times)

    #animation
    anims = []
    for n in range(len(names)):
        motion = []
        for t in range(len(frame_times)):
            motion += [(quat2euler(rots[t][n]), poss[t][n])]
        anims += [motion]

    #write to dda format
    print(name)
    with open(name, 'w') as f:
        f.write("<format>\n")
        f.write("global\n")
        f.write("</format>\n")
        f.write("<framerate>\n")
        f.write(str(framerate) + "\n")
        f.write("</framerate>\n")
        f.write("<repeat>\n")
        f.write(str(int(repeat)) + "\n")
        f.write("</repeat>\n")
        f.write("<buffer>\n")
        f.write("j " + str(num_joints) + "\n")
        f.write("f " + str(num_frames) + "\n")
        f.write("</buffer>\n")
        for bone in range(len(anims)):
            f.write("<animation>\n")
            f.write("- " + str(bone) + "\n")
            for (rot, pos) in anims[bone]:
                f.write("r " + " ".join([str(x) for x in rot]) + "\n")
                f.write("p " + " ".join([str(x*M_2_CM) for x in pos]) + "\n")
            f.write("</animation>\n")
        f.write("\n")

def export_ddb(data, name):
    """Converts internal time, name, position, and rotation data into ddb.
    Specifically takes the first frame of the csv animation as the "T"-pose."""
    (frame_times, names, poss, rots) = data
    num_joints = len(names)

    #write to ddb format
    with open(name, 'w') as f:
        f.write("<size>\n")
        f.write(str(num_joints) + "\n")
        f.write("</size>\n")
        f.write("<global>\n")
        f.write("p 0 0 0\n")
        f.write("r 0 0 0\n")
        f.write("s 1 1 1\n")
        f.write("</global>\n")
        for i in range(len(names)):
            f.write("<joint>\n")
            f.write(names[i] + " " + str(i) + " " + str(hierarchy_map[i]) + "\n")
            f.write("p " + " ".join([str(x * M_2_CM) for x in poss[i][0]]) + "\n")
            f.write("r " + " ".join([str(x) for x in quat2euler(rots[i][0])]) + "\n")
            f.write("s 1 1 1\n")
            f.write("</joint>\n")

#if called from CLI, read in file name, parse into data, export to specific format
if __name__ == '__main__':
    #name = in_csv_name()
    in_dir_name = input("please enter dir of m.csv to convert to dda:")
    out_dir_name = input("please enter dir to ouptut dda files:")
    for f in os.listdir(in_dir_name):
        if f[-4:] == ".csv":
            data = read_csv(in_dir_name+"/"+f)
            print(out_dir_name+"/"+f[:-4]+".dda")
            export_dda(data, out_dir_name+"/"+f[:-4]+".dda")
            #export_ddb(data, out_dir_name+f[-4:]+".ddb")
        else:
            continue
    #if 'b' in input("dd(a)/dd(b)? "):
    #    export_ddb(data, name[:-4]+".ddb")
    #else:
    #    export_dda(data, name[:-4]+".dda")

