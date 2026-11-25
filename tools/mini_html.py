Import("env")
import os
import glob
from importlib.metadata import PackageNotFoundError, version
from pathlib import Path
import sys
import subprocess

def ensure_module_version(package_name, required_version):
    try:
        installed_version = version(package_name)
        if installed_version == required_version:
            print(f"{package_name} {required_version} is already installed.")
            return
        else:
            print(f"{package_name} version {installed_version} found – replacing with {required_version}.")
    except PackageNotFoundError:
        print(f"{package_name} is not installed – installing version {required_version}.")

    # Install the required version (will upgrade or downgrade as needed)
    subprocess.check_call([
        sys.executable,
        '-m',
        'pip',
        'install',
        '--upgrade',
        f'{package_name}=={required_version}',
    ])


ensure_module_version("minify_html", "0.18.1")
import minify_html

filePath = 'src/webpages/'
try:
  print("==========================")
  print("Generating webpage")
  print("==========================")
  print("Preparing html.h file from source")
  print("  -insert header") 
  cpp_output = "#pragma once\n\n#include <Arduino.h>  // PROGMEM\n\n"
  tmp = ""
  print("  -insert html")

  for x in glob.glob(filePath+"*.html"):
   print("prozessing file:" + Path(x).stem)
   print(Path(x).stem)
   cpp_output += "static const char "+Path(x).stem+"[] PROGMEM = R\"rawliteral("
   f = open(x, "r")
   if env.GetProjectOption("build_type") == "debug":
        tmp = f.read()
        contentlength = len(tmp)
        cpp_output += tmp  
   else:
       #tmp = f.read()  #disable compression until fixed that the compressor remove %VARIABLE%
       #tmp = minify_html.minify(f.read(),do_not_minify_doctype=True, minify_js=True)
       tmp = minify_html.minify(f.read(), minify_js=True)
       contentlength = len(tmp)
       cpp_output += tmp

   f.close()
   cpp_output += ")rawliteral\";\n"
   cpp_output += "#define "+Path(x).stem+"_LENGTH " + str(contentlength) +"\n"

   f = open ("./src/html.h", "w")
   f.write(cpp_output)
   f.close()
   print("==========================\n")

except SyntaxError as e:
  print(e)