# This script expects to be installed:
# sudo apt install nodejs npm
# sudo npm install html-minifier -g
# sudo npm install clean-css-cli -g
# sudo npm install uglify-js -g

import os

Import("env", "projenv")

def generateHtml(debug):
    try:
        print("==========================")
        print("Preparing html.h file from source")
        print("  -insert header")
        cpp_output = "#pragma once\n\n#include <Arduino.h>  // PROGMEM\n\n"
        print("  -insert html")
        cpp_output += "const char index_html[] PROGMEM = R\"***("
        if debug:
          f = open("./html/index.html", "r")
          cpp_output += f.read()
          f.close()    
        else:
          stream = os.popen('html-minifier --collapse-whitespace --remove-comments --remove-optional-tags --remove-redundant-attributes --remove-script-type-attributes --remove-tag-whitespace --use-short-doctype --minify-css true --minify-js true ./html/index.html')
          cpp_output += stream.read()   
        cpp_output += ")***\";\n\n"
        print("  -insert css")
        cpp_output += "const char style_css[] PROGMEM = R\"***("
        if debug:
          f = open("./html/style.css", "r")
          cpp_output += f.read()
          f.close()
        else:
          stream = os.popen('cleancss ./html/style.css')
          cpp_output += stream.read()
        cpp_output += ")***\";\n\n"
        print("  -insert js")
        cpp_output += "const char script_js[] PROGMEM = R\"***("
        f = open("./html/colourpicker.js", "r")
        cpp_output += f.read()
        f.close()
        cpp_output += "\n\n/* start custom scripts */\n"
        if debug:
          f = open("./html/script.js", "r")
          cpp_output += f.read()
          f.close()
        else:
          stream = os.popen('uglifyjs ./html/script.js --compress --mangle')
          cpp_output += stream.read()
        cpp_output += ")***\";\n\n"

        f = open ("./src/html.h", "w")
        f.write(cpp_output)
        f.close()
        print("==========================\n")
    except:
      print("error preparing webpage")

# true for debug, false for production
buildType = projenv["PIOENV"].endswith("_debug")

print("\n==========================")
if (buildType):
  print("Generating webpage in debug mode")
  generateHtml(True)
else:
  print("Generating webpage in release mode")
  generateHtml(False)
