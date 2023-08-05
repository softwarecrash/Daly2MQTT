# This script expects to be installed:
# sudo apt install nodejs npm
# sudo npm install html-minifier -g
# sudo npm install clean-css-cli -g
# sudo npm install uglify-js -g


##Import("env")
import os
import glob
from pathlib import Path

#import htmlmin

#from htmlmin import minify
#import sys
#import pip


#def install(package):
 #   if hasattr(pip, 'main'):
 #       pip.main(['install', package])
 #   else:
#        pip._internal.main(['install', package])
#install('minify_html')




try:
    import minify_html

except ImportError:
    env.Execute(
        env.VerboseAction(
            '$PYTHONEXE -m pip install "minify_html" ',
            "Installing ESP-IDF's Python dependencies",
        )
    )


filePath = 'src/webpages/'


try:
  #startpath = os.path.dirname(targetfile);
  print("==========================")
  print("Generating webpage")
  print("==========================")
  print("Preparing html.h file from source")
  print("  -insert header") 
  cpp_output = "#pragma once\n\n#include <Arduino.h>  // PROGMEM\n\n"
  print("  -insert html")


  print("list files:")
  print(glob.glob(filePath+"*.html"))

  for x in glob.glob(filePath+"*.html"):
   print("prozessing file:" + Path(x).stem)
   print(Path(x).stem)
   cpp_output += "const char "+Path(x).stem+"[] PROGMEM = R\"rawliteral("
   f = open(x, "r")
   cpp_output += minify_html.minify(f.read(), minify_js=True, remove_processing_instructions=True)
   f.close()
   cpp_output += ")rawliteral\";\n\n"

   f = open ("./src/html.h", "w")
   f.write(cpp_output)
   f.close()
   print("==========================\n")

except SyntaxError as e:
  print(e)