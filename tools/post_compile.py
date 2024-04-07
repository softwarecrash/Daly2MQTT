Import("env")
import os
import shutil
import gzip

def post_program_action(source, target, env):

    targetfile = os.path.abspath(target[0].get_abspath())
    filename = os.path.basename(targetfile)
    startpath = os.path.dirname(targetfile)
    destpath = os.path.normpath(os.path.join(startpath, '../../../.firmware'))
    
    # if it should be placed in a subfolder of the environment (e.g. 'd1_mini'), comment out the line above and uncomment the two below
    #basedir = os.path.basename(startpath)
    #destpath = os.path.normpath(os.path.join(startpath, '../../../.firmware', basedir))

    print("\nCopying " + filename + " file to the build directory...\n")
    print("Target file: " + targetfile)
    print("Destination directory: " + destpath)

    # create directories if they don't exist
    if not os.path.exists(destpath):
        os.makedirs(destpath)

    # copy the target file to the destination, if it exist
    if os.path.exists(targetfile):
        shutil.copy(targetfile, destpath)
        with open(destpath+'/'+filename, 'rb') as src, gzip.open(destpath+'/'+os.path.splitext(filename)[0]+'_OTA.bin.gz', 'wb') as dst: dst.writelines(src)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", post_program_action)
