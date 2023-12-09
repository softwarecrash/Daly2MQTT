#! /usr/bin/python
import requests
import gzip
import os

def CreateHeader( url, filename ):
    full_file_name = f'src/jslibs/{filename}.gz.h'
    if not os.path.exists(full_file_name):
        byte_count = 0;
        line_count = 0;
        request = requests.get( url )
        request_compressed = gzip.compress( request.content )
        with open( "src/jslibs/" + filename + ".gz.h",'w') as result_file:
            result_file.write( f'const PROGMEM unsigned char {filename}[] = {{\n' )
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
            result_file.write( f'const size_t {filename}_len={request_compressed.__len__()};')


CreateHeader( "https://cdn.jsdelivr.net/npm/bootstrap@5.3.1/dist/css/bootstrap.min.css", "bootstrap_min_css" )
CreateHeader( "https://cdn.jsdelivr.net/npm/bootstrap-icons@1.3.0/font/bootstrap-icons.css", "bootstrap_icons_css" )
CreateHeader( "https://ajax.googleapis.com/ajax/libs/jquery/3.6.0/jquery.min.js", "jquery_min_js" )
CreateHeader( "https://cdn.jsdelivr.net/npm/bootstrap@5.3.1/dist/js/bootstrap.bundle.min.js", "bootstrap_bundle_min_js" )
CreateHeader( "https://cdn.jsdelivr.net/npm/chart.js@4.3.0/dist/chart.umd.min.js", "chart_umd_min_js" )
