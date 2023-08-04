# This script expects to be installed:
# sudo apt install nodejs npm
# sudo npm install html-minifier -g
# sudo npm install clean-css-cli -g
# sudo npm install uglify-js -g


#Import("env", "projenv")
import os
import glob

import sys
#import subprocess
import pip


def install(package):
    if hasattr(pip, 'main'):
        pip.main(['install', package])
    else:
        pip._internal.main(['install', package])
install('minify-html')


import minify_html

filePath = 'src/webpages/'

def generateHtml(debug):
    try:
        #startpath = os.path.dirname(targetfile);
        print("==========================")
        print("Preparing html.h file from source")
        print("  -insert header")

        print("list files:")
        print(glob.glob(filePath+"*.html")) 

        cpp_output = "#pragma once\n\n#include <Arduino.h>  // PROGMEM\n\n"
        print("  -insert html")
        cpp_output += "const char index_html[] PROGMEM = R\"***("

        f = open("src/webpages\\HTML_MAIN.html", "r")
        cpp_output += f.read()
        f.close()
        cpp_output += ")***\";\n\n"

        f = open ("./src/html.h", "w")
        f.write(cpp_output)
        f.close()
        print("==========================\n")


        try:
            print( minify_html.minify("<p>  Hello, world!  </p>", minify_js=False, minify_css=False))
        except SyntaxError as e:
          print(e)
    except SyntaxError as e:
      print(e)
    #except:
      #print("error preparing webpage")

# true for debug, false for production
#buildType = projenv["PIOENV"].endswith("_debug")
buildType = True

print("\n==========================")
if (buildType):
  print("Generating webpage in debug mode")
  generateHtml(True)
else:
  print("Generating webpage in release mode")
  generateHtml(False)
