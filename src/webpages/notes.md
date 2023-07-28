minify html code like this
https://github.com/bertmelis/ledController/blob/main/build_webpage.py

or this?
https://tinkerman.cat/post/optimizing-files-for-spiffs-with-gulp/

this is the heavy method, asyncwebserver support gzip, but dont know htmlprozessor can handle gzipped files
https://github.com/TheNitek/RfidShelf/blob/main/RfidShelf/platformio_script.py

added files for testing

todo for the py file
- grab every html file in folder
- take name and store it for the progmem construct
- minify the content
- put it in the html.h
- repeat for ever .html file and append the content to html.h