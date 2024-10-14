Import("env")
import os
import glob
from pathlib import Path
import gzip
import zlib
import sys
import pip

def install(package):
    if hasattr(pip, 'main'):
        pip.main(['install', package])
    else:
        pip._internal.main(['install', package])

try:
    import minify_html
except ImportError:
    install('minify_html')

filePath = 'src/webpages/'
try:

  print("==========================")
  print("Generating webpage")
  print("==========================")
  print("Preparing html.h file from source")
  print("  -insert header") 
  #cpp_output = "#pragma once\n\n#include <Arduino.h>  // PROGMEM\n\n"
  #tmp = ""
  print("  -insert html")

  with open( "./src/html_gz.h",'w') as result_file:
    for x in glob.glob(filePath+"*.html"):
      print("prozessing file:" + Path(x).stem)
      print(Path(x).stem)
      byte_count = 0
      line_count = 0
      infile = open(x, "rb")
      inFileBytes = infile.read()
      request_compressed = gzip.compress( inFileBytes )

      result_file.write( f'const size_t {Path(x).stem}_GZ_SIZE={request_compressed.__len__()};\n')
      result_file.write( f'const uint8_t {Path(x).stem}_GZ[] PROGMEM = {{\n' )
      for b in request_compressed:
         result_file.write( f'0x{b:02X}')
         line_count = line_count+1
         byte_count = byte_count+1
         if byte_count < request_compressed.__len__():
             result_file.write( ',' )
         if line_count == 16:
            result_file.write( "\n" )
            line_count = 0
      result_file.write('};\n')
      #result_file.write( f'const size_t {Path(x).stem}_len={request_compressed.__len__()};')





   #cpp_output += "static const char "+Path(x).stem+"[] PROGMEM = R\"rawliteral("
   #f = open(x, "r")
   #tmp = f.read()

#https://mischianti.org/web-server-with-esp8266-and-esp32-byte-array-gzipped-pages-and-spiffs-2/
#https://docs.micropython.org/en/latest/library/gzip.html
#https://tinkerman.cat/post/embed-your-website-in-your-esp8266-firmware-image/

   #tmp = str(zlib.compress(bytearray(tmp,'utf8'),9))
   #contentlength = len(tmp)
   #cpp_output += tmp  
   

   #f.close()
   #cpp_output += ")rawliteral\";\n"
   #cpp_output += "#define "+Path(x).stem+"_LENGTH " + str(contentlength) +"\n"

   #f = open ("./src/html.h", "w")
   #f.write(cpp_output)
   #f.close()
   #print("==========================\n")

except SyntaxError as e:
  print(e)