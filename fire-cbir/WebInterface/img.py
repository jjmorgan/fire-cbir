#!/usr/bin/python

import sys
import os
import traceback
import cgi
import re
import Image
import images2gif

sys.stderr = sys.stdout
try:

    form = cgi.FieldStorage()
    imgpath = form["image"].value

    if (form.has_key("type")):
      imgpath = os.path.join(os.path.dirname(imgpath), os.path.splitext(os.path.basename(imgpath))[0]) + '-' + form["type"].value + '.gif'

    image=open(imgpath,'rb').read()
    
    contenttype="image/gif"
    print "Content-Type: "+contenttype+"\n"

    sys.stdout.write(image)

except Exception as e:
    contenttype="text/html"
    print "Content-Type: "+contenttype+"\n"

    print type(e).__name__ + '<br>\n'
    print e, '<br>\n'
    print traceback.print_exc(file=sys.stdout)


