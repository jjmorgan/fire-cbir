#!/usr/bin/python
#$Header: /u/cvs/image/fire/cgi/fire.py,v 1.19 2005/05/11 15:39:59 weyand Exp $

# print cgi-header
print "Content-Type: text/html\n\n"
print
#print "<pre style=\"font-family:Fixedsys,Courier,1px;\">" # to have error messages as readable as possible, this
              # is switched of right before displaying the main part of the page
              

#import lots of stuff              
import  cgi, re, sys, traceback, os
from types import *


# redirect stderr to have error messages on the webpage and not in the
# webserver's log
sys.stderr=sys.stdout

# to be able to find the config
sys.path+=[os.getcwd()] 

#import "my" socket implementation
from firesocket import *

# ----------------------------------------------------------------------
# import the config
# it is assumed there is a file called config in the same file as the cgi.
# an example for a valid config:
## settings for i6
#positiveImg="http://www-i6.informatik.rwth-aachen.de/~deselaers/images/positive.png"
#negativeImg="http://www-i6.informatik.rwth-aachen.de/~deselaers/images/negativ.png"
#neutralImg="http://www-i6.informatik.rwth-aachen.de/~deselaers/images/neutral.png"
#fireLogo="/u/deselaers/work/src/fire/cgi/fire-logo.png"
#i6Logo=  "/u/deselaers/work/src/fire/cgi/i6.png"
#TemplateFile = "/u/deselaers/work/src/fire/cgi/fire-template.html"
#fireServer="deuterium"
#firePort=12960
# ----------------------------------------------------------------------
import config

# settings for the linear (standard) scoring algorithm.  using this
# algorithm the weights can be changed from the web interface
class LinearScoring:
    def __init__(self):
        self.size=0
        self.weights=[]
    def get(self,status):
        tmp=status.pop()
        self.size=status.pop()
        for i in range(int(self.size)):
            tmp=status.pop()
            if(tmp=="weight"):
                idx=status.pop()
                w=status.pop()
                self.weights+=[w]
            else:
                print "Cannot parse LinearScoring settings"
    def getForm(self):
        result=""
        result+="<tr><th colspan=3> Settings for LinearScoring</th></tr>"
        for i in range(len(self.weights)):
            result+="<tr><td> Weight "+str(i)+"</td>"
            result+="<td colspan=2><input name=\"weight"+str(i)+"\" type=\"text\" value=\""+self.weights[i]+"\"></td></tr>"
        return result

# settings for maxent scoring algorithm
class MaxEntScoring:
    def __init__(self):
        self.size=0
        self.factor=0
        self.offset=0
        self.lambdas=[]
    def get(self,status):
        tmp=status.pop()
        self.size=status.pop()
        tmp=status.pop()
        self.factor=status.pop()
        tmp=status.pop()
        self.offset=status.pop()
        for i in range(int(self.size)):
            tmp=status.pop()
            idx=status.pop()
            l=status.pop()
            self.lambdas+=[l]
    def getForm(self):
        result=""
        result+="<tr><th colspan=3> Settings for MaxEntScoring</th></tr>"
        for i in range(len(self.lambdas)/2):
            result+="<tr><td> Lambda "+str(i)+"</td>"
            result+="<td>"+self.lambdas[i]+"</td><td>"+self.lambdas[i+len(self.lambdas)/2]+"</td></tr>"
        return result

# a class to store the settings of fire this classes uses the classes
# defined above to manage the settings of the active scoring algorithm
class FireSettings:
    def __init__(self):
        # variable initilization only. all member variables are defined here.
        self.fireServer=config.fireServer
        self.firePort=config.firePort
        self.dbsize=0
        self.scoringname=""
        self.results=0
        self.expansions=0
        self.distances=[]
        self.suffices=[]
        self.path=""
        self.scoring=""
    def get(self,s):
        # get the settings from the server, i.e. read the string s and parse it.
        self.distances=[]
        self.suffices=[]
        s.sendcmd("info")
        msg=s.getline()
        status=re.split(" ",msg)
        print "<!--"+str(status)+"-->"
        status.reverse()
        while(len(status)>0):
            keyword=status.pop()
            if keyword=="filelist":
                self.dbsize=status.pop()
            elif keyword=="results":
                self.results=status.pop()
            elif keyword=="path":
                self.path=status.pop()
            elif keyword=="extensions":
                self.expansions=status.pop()
            elif keyword=="scoring":
                self.scoringname=status.pop()
                if self.scoringname=="linear":
                    self.scoring=LinearScoring()
                elif self.scoringname=="maxent":
                    self.scoring=MaxEntScoring()
                self.scoring.get(status)
            elif keyword=="suffix":
                no=status.pop()
                suffix=status.pop()
                self.suffices+=[suffix]
                distname=status.pop()
                self.distances+=[distname]
            else:
                print "<!-- Unknown keyword in stream : "+keyword+" -->"
    def display(self):
        # display the settings dialog
        self.get(s)
        result="<h4>Settings for fire</h4>"
        result=result+"<form method=\"post\" name=\"settingsForm\">"
        result=result+"<input type=\"hidden\" name=\"server\" value=\""+str(self.fireServer)+"\"/>\n"
        result=result+"<input type=\"hidden\" name=\"port\" value=\""+str(self.firePort)+"\"/>\n"
        result=result+"<input type=\"hidden\" name=\"settings\" value=\"1\"/>\n"
        result=result+"<table border=\"1\">\n"
        result=result+"<tr><th colspan=3> General Settings</th></tr>"
        result=result+"<tr><td>Number of images in database</td><td colspan=\"2\">"+str(self.dbsize)+"</td></tr>\n"
        result=result+"<tr><td>Number of results shown</td><td colspan=\"2\"><input type=\"text\" value=\""+self.results+"\" name=\"results\"/></td></tr>\n"
        result=result+"<tr><td>Scoring</td><td colspan=\"2\">"+self.getScoringChooser()+"\n"
        result=result+"<tr><td>Number of Images for Query Expansion</td><td colspan=\"2\"><input type=\"text\" value=\""+self.expansions+"\" name=\"extensions\"/></td></tr>\n"
        result=result+self.getDistForm()
        result=result+self.getScoringForm()
        result=result+"</table>\n"
        result=result+"Password: <input type=\"password\" name=\"password\">\n"
        result=result+"<input type=\"submit\" name=\"Save\" value=\"Save\"/>\n"
        result=result+"</form>"
        return result
    def getScoringChooser(self):
        # select the chooser for the server
        result="<select name=\"scoring\">\n<option value=\"\">"
        for scor in ['linear','maxent']:
            if self.scoringname==scor:
                result=result+"<option selected value=\""+scor+"\">"+scor+"</option>\n"
            else:
                result=result+"<option value=\""+scor+"\">"+scor+"</option>\n"
        result+="</select>\n"
        return result
    def getScoringForm(self):
        # display the form element to select the scoring algorihtm
        return self.scoring.getForm()
    def getDistForm(self):
        result=""
        result+="<tr><th colspan=3> Settings for Distance Functions</th></tr>"
        for no in range(len(self.distances)):
            result+="<tr><td> Distance "+str(no)+": "+self.suffices[no]+"</td><td>"+self.getDistChooser(no,self.distances[no])+"</td></tr>"
        return result
    def getDistChooser(self,no,distName):
        result="<select name=\"dist"+str(no)+"\">\n<option value=\"\">"
        s.sendcmd("setdist")
        distsString=s.getline()
        dists=re.split(" ",distsString)
        for d in dists:
            if d==distName:
                result=result+"<option selected value=\""+d+"\">"+d+"</option>\n"
            else:
                result=result+"<option value=\""+d+"\">"+d+"</option>\n"
        result=result+"</select>\n"
        return result
    def process(self,form):
        result="<!-- processing settings -->\n"
        if form.has_key("results"):
            if form.has_key("password"):
                password=form["password"].value
            else:
                password=""
            s.sendcmd("password "+password)
            l=s.getline()
            result+="<!-- "+l+"-->"
            if l!="ok":
                result+="Changing settings denied: Not authorized!"
                
        for i in form.keys():
            if i.find("dist")==0:
                no=i[4:]
                d=form[i].value
                if d!=self.distances[int(no)]: # only change when changed
                    s.sendcmd("setdist "+no+" "+d)
                    l=s.getline()
                    result=result+"<!-- "+l+" -->\n"
            elif i.find("weight")==0:
                no=i[6:]
                w=float(form[i].value)
                if self.scoringname=="linear":
                    if(float(self.scoring.weights[int(no)])!=float(w)):
                        s.sendcmd("setweight "+no+" "+form[i].value)
                        l=s.getline()
                        result=result+"<!-- "+l+" -->\n"
                else:
                    print "weights not supported"
            elif i.find("results")==0:
                if int(form[i].value)!=int(self.results):
                    s.sendcmd("setresults "+form[i].value)
                    l=s.getline()
                    result=result+"<!-- "+l+" -->\n"
            elif i.find("extensions")==0:
                if int(self.expansions)!=int(form[i].value):
                    s.sendcmd("setextensions "+form[i].value)
                    l=s.getline()
                    result=result+"<!-- "+l+" -->\n"
            elif i.find("scoring")==0:
                sn=form[i].value
                if(sn!=self.scoringname): # only change when changed
                    s.sendcmd("setscoring "+sn)
                    l=s.getline()
                    result=result+"<!-- "+l+" -->\n"
                    self.scoringname=sn
            else:
                result=result+"<!-- "+i+"-->\n"
        result=result+"<!-- settings processed -->\n"
        return result

        

# ----------------------------------------------------------------------
# Load the template file and display the content in this template file
# the string FIRELOGOSTRINGHERE, I6LOGOSTRINGHERE, and "INSERT CONTENT
# HERE" are replaced by the configured/generated strings
# ----------------------------------------------------------------------
def Display(Content):
    TemplateHandle = open(config.TemplateFile, "r")  # open in read only mode
    # read the entire file as a string
    TemplateInput = TemplateHandle.read() 
    TemplateHandle.close()                    # close the file

    # this defines an exception string in case our
    # template file is messed up
    BadTemplateException = "There was a problem with the HTML template."
    TemplateInput = re.sub("FIRELOGOSTRINGHERE",config.fireLogo,TemplateInput)
    TemplateInput = re.sub("I6LOGOSTRINGHERE",config.i6Logo,TemplateInput)
    SubResult = re.subn("INSERT CONTENT HERE",Content,TemplateInput)
    
    if SubResult[1] == 0:
        raise BadTemplateException
    print SubResult[0]


def sendretrieve(querystring):
    filterstring=""
#    if(form.has_key("filter1")):
#        filterstring+=" -porn2-sorted/009-www.ficken.bz-images-ficken.png"
#    if(form.has_key("filter2")):
#        filterstring+=" -porn2-sorted/037-www.free-porn-free-sex.net-free-porn-free-sex-free-sex-sites.png"
    s.sendcmd("retrieve "+querystring+filterstring)

# ----------------------------------------------------------------------
# handle a retrieval request which was generated by clicking a random
# image, that is:
# 1. process the form
#    - extract the filename of the image which is used as query
# 2. send the query to the server
# 3. wait for the servers answer
# 4. call displayResults with the servers answer to display the results
# ----------------------------------------------------------------------
def retrieve(form):
    result="<h3>Retrieval Result</h3>\n"
    queryimage=form["queryImage"].value
    sendretrieve("+"+queryimage)
    msg=s.getline()
    tokens=re.split(" ",msg)
    tokens.reverse()
    result=result+displayResults(tokens,"querystring",queryimage,0)
    return result


# ----------------------------------------------------------------------
# handle a retrieval request which was generated by relevance feedback.
# That is:
# 1. process the form
#    - extract the filenames of positive and negative examples
# 2. generate the query string
# 3. send the query to the server
# 4. wait for the servers answer
# 5. call displayResults with the servers answer to display the results
# ----------------------------------------------------------------------
def feedbackretrieve(form):
    print "<!-- feedback retrieve \n"
    print form
    print "-->\n"
    result="<h3>Retrieval Result</h3>\n"
    queryimages=""
    for field in form.keys():
        if field.startswith("relevance-"):
            imagename=re.sub("relevance-","",field)
            relevance=form[field].value
            if relevance!="0":
                queryimages=queryimages+" "+relevance+imagename
        else:
            print "<!-- field not processed: "+field+" -->"
    result=result+"\n<!-- "+queryimages+" -->\n"
    sendretrieve(queryimages)
    msg=s.getline()
    tokens=re.split(" ",msg)
    tokens.reverse()
    result=result+displayResults(tokens,"querystring",queryimages,0)
    return result

# ----------------------------------------------------------------------
# given the retrieval result string of the server process this answer
# and create the part of the html page displaying these results.  this
# includes the buttons for relevance feedback and the hidden fields
# for server settings (e.g.\ which server, which port).  also the
# buttons for "more results" and "relevance feedback" are generated.
# the button for saving relevances is deaktivated as it is not used
# ----------------------------------------------------------------------
def displayResults(tokens,querytype,querystring,resultsStep):
    print "<!-- "+str(tokens)+"-->"
    i=0

    result="<form method=\"post\" name=\"resultForm\"><table>\n<tr>\n"
    result=result+"<input type=\"hidden\" name=\"server\" value=\""+settings.fireServer+"\"/>\n"
    result=result+"<input type=\"hidden\" name=\"port\" value=\""+str(settings.firePort)+"\"/>\n"
    result=result+"<input type=\"hidden\" name=\""+querytype+"\" value=\""+querystring+"\"/>\n"
    result=result+"<input type=\"hidden\" name=\"resultsstep\" value=\""+str(resultsStep)+"\"/>\n"

    first=True

    while(len(tokens) > 0):
        img=tokens.pop()
        score=float(tokens.pop())
        result=result+"<td>\n"
        result=result+"<a href=\"img.py?image="+settings.path+"/"+img+"\" target=\"_blank\">"
        result=result+"<img src=\"img.py?max=150&image="+settings.path+"/"+img+"\" title=\""+str(score)+"-"+img+"\" name=\""+str(score)+"-"+img+"\">\n<br>\n"
        result=result+"</a>"

        if first:
            result=result+"<center><input type=\"radio\" name=\"relevance-"+img+"\" value=\"+\" checked><img src=\""+config.positiveImg+"\" alt=\"+\">&nbsp;\n"
            result=result+"<input type=\"radio\" name=\"relevance-"+img+"\" value=\"0\"><img src=\""+config.neutralImg+"\" alt=\"0\">&nbsp;\n"
            result=result+"<input type=\"radio\" name=\"relevance-"+img+"\" value=\"-\"><img src=\""+config.negativeImg+"\" alt=\"-\"></center>\n"
            first=False
        else:
            result=result+"<center><input type=\"radio\" name=\"relevance-"+img+"\" value=\"+\"><img src=\""+config.positiveImg+"\" alt=\"+\">&nbsp;\n"
            result=result+"<input type=\"radio\" name=\"relevance-"+img+"\" value=\"0\" checked><img src=\""+config.neutralImg+"\" alt=\"0\">&nbsp;\n"
            result=result+"<input type=\"radio\" name=\"relevance-"+img+"\" value=\"-\"><img src=\""+config.negativeImg+"\" alt=\"-\"></center>\n"
        result=result+"</td>\n\n"
        i=i+1
        if i%5==0:
            result=result+"</tr>\n\n\n<tr>"
    result=result+"</tr></table>\n"
#    result+=filterbuttons()
    result=result+"<button title=\"expandresults\" name=\"expandresults\" type=\"submit\" onClick=\"document.resultForm.feedback.value='expandresults';document.resultForm.submit()\">more results</button>\n"
    result=result+"<button title=\"requery\" name=\"requery\" type=\"submit\" onClick=\"document.resultForm.feedback.value='requery';document.resultForm.submit()\">requery</button>\n"
#   result=result+"<button title=\"saverelevances\" name=\"relevancessave\" onClick=\"document.resultForm.feedback.value='saverelevances';document.resultForm.submit()\" \"type=\"submit\">save relevances</button>\n"
    result=result+"<input type=\"hidden\" name=\"feedback\" value=\"yes\"/>\n"
    result=result+"</form>\n"
    
    return result

def filterbuttons():
    # display the buttons for the filter
    result=""
    result=result+"filter 1: <input type=\"checkbox\" name=\"filter1\" value=\"1\">\n"
    result=result+"filter 2: <input type=\"checkbox\" name=\"filter2\" value=\"2\">\n<br>\n"
    return result

# ----------------------------------------------------------------------
# ask the server for filenames of random images and create the part of
# the html page showing random images. Steps:
# 1. send "random" to the server
# 2. receive the servers answer
# 3. process the answer:
#   - get the filenames and for each filename create a querybutton
# 4. put the "more random images" button, such that we can get a new set
#    of random images
# ----------------------------------------------------------------------
def randomImages():
    # display random images
    s.sendcmd("random")
    result="<hr><h3> Random Images - Click one to start a query </h3>"
    result+="""<form method="post" name="queryForm">
    <input type="hidden" name="queryImage" value="">
    """
    result=result+"<input type=\"hidden\" name=\"server\" value=\""+settings.fireServer+"\"/>\n"
    result=result+"<input type=\"hidden\" name=\"port\" value=\""+str(settings.firePort)+"\"/>\n"
    #result+=filterbuttons()

    res=s.getline()
    for img in re.split(" ",res):
        if img != "" and img != "\n":
            result=result+"<button title=\""+img+"\" name=\"searchButton\" type=\"submit\" onClick=\"document.queryForm.queryImage.value='"+img+"';document.queryForm.submit()\" value=\""+img+"\">\n"
            result=result+"<img  src=\"img.py?max=100&image="+settings.path+"/"+img+"\" title=\""+img+"\">\n"
            result=result+"</button>\n"

    if form.has_key("demomode"):
        seconds=form["demomode"].value
        url="http://"
        if(os.environ.has_key("HTTP_HOST") and os.environ.has_key("REQUEST_URI")):
            part1=os.environ["HTTP_HOST"]
            part2=re.sub("&queryImage=.*$","",os.environ["REQUEST_URI"])
            url=url+part1+part2+"&queryImage="+img
        print "<meta http-equiv=\"refresh\" content=\""+seconds+"; URL="+url+"\">\n"
        result=result+"<meta http-equiv=\"refresh\" content=\""+seconds+"; URL="+url+"\">\n"

    result+="<br><div align=\"right\">\n<button title=\"new random images\" name=\"morerandom\" type=\"submit\">more random images</button>\n</div>\n"
    return result

# ----------------------------------------------------------------------
# generate the text field and the query button for text
# queries
# ----------------------------------------------------------------------
def textQueryForm(settings):
    result =  "<hr><h3>Query by "
    if("textfeature" in settings.distances):
        result += "description"
        if("metafeature" in settings.distances):
            result += " or meta information"
    elif("metafeature" in settings.distances):
        result += "meta information"
    result += "</h3>\n"
    result += "<form method=\"post\" name=\"textQueryForm\">\n"
    result += "<input type=\"hidden\" name=\"server\" value=\""+str(settings.fireServer)+"\"/>\n"
    result += "<input type=\"hidden\" name=\"port\" value=\""+str(settings.firePort)+"\"/>\n"
    result += "<input type=\"hidden\" name=\"resultsstep\" value=\"0\"/>\n"
    result += "<input type=\"text\" name=\"textquerystring\" size=\"30\" value=\"\"/>\n"
    result += "<input type=\"submit\" value=\"query\"/>\n"
    
    if("metafeature" in settings.distances):
        result += "<a href=\"#\" onClick=\"window.open('fire.cgi?server="+ \
            settings.fireServer+"&port="+str(settings.firePort)+ \
            "&metafeatureinfo=1','newwindow',"+ \
            "'height=600, width=600')\">help on meta info</a>"
    
    if("textfeature" in settings.distances):
        result += "<a href=\"#\" onClick=\"window.open('fire.cgi?server="+ \
            settings.fireServer+"&port="+str(settings.firePort)+ \
            "&textfeatureinfo=1','newwindow',"+ \
            "'height=600, width=600')\">help on description</a>"

    result += "</form>"
    
    return result

def displayMetaFeatureInfo():
    s.sendcmd("metafeatureinfo")
    mfi = s.getline()
    mfil = mfi.split(" ")

    result = "<h4>Help on queries by meta info</h4>\n"
    result += """If you want to find images with certain meta information attached,
        type in a request of the form
        <h5>key1:val1,key2:val2,...</h5>\n
        The following meta keys are available in this corpus:<br><br>\n"""
    
    #TODO: The table looks ugly!
    result += "<table>\n"
    result += "<tr>\n"
    result += "  <th>key</th>\n"
    result += "  <th>example value</th>\n"
    result += "</tr>\n"
    for mf in mfil:
        mfl = mf.split(":")
        result += "<tr>\n"
        result += "  <td><b>"+mfl[0]+"</b></td>\n" # I know <b> is deprecated, but it still works :-)
        result += "  <td>"+mfl[1]+"</td>\n"
        result += "</tr>\n"
    result += "</table>\n"
    
    return result

def displayTextFeatureInfo():
    result = "<h4>Help on queries by description</h4>\n"
    result += """If you want to find images which have text information attached to them,
	just enter the query into the textbox. Fire will then use an information
	retrieval engine to find the images that best match your query.<br><br>
	If you have text information in multiple languages for each image, you can give
	a query for every language. For example, if you have german and french text
	information and you want to search for "hand" in the respective languages, you enter
	<h5>Ge:"hand" Fr:"mains"</h5>
	Here, "Ge" has to be the suffix for the german textfiles and "Fr" has to be the suffix for the
	fench textfiles. Fire will then use both queries and send them to separate information retrieval
	engines."""

    return result	

# ----------------------------------------------------------------------
# this function makes the server save a relevances logfile
# but it is not used at the moment, because the saverelevances button
# is deaktivated
# ----------------------------------------------------------------------
def saveRelevances(form):
    querystring=form["querystring"].value
    relevancestring=""
    for field in form.keys():
        if field.startswith("relevance-"):
            imagename=re.sub("relevance-","",field)
            relevance=form[field].value
            if relevance!="0":
                relevancestring=relevancestring+" "+relevance+imagename
        else:
            print "<!-- field not processed: "+field+" -->"

    s.sendcmd("saverelevances Q "+querystring+" R "+relevancestring)
    res=s.getline();
    message="<a href=\"javascript:back()\"> Go back to last query </a>"
    return message
 
# ----------------------------------------------------------------------
# this is the "getMoreResults" function it basically does the same as
# retrieve, uses the same query string as the last query used (this is
# gotten from the form) and the calls displayResults
# ----------------------------------------------------------------------
def expandQuery(form):
    if(form.has_key("metaquerystring")):
        result="<h3>Metatag-based retrieval result</h3>\n"
        queryfield = "metaquerystring"
        cmd = "metaexpand"
    elif(form.has_key("textquerystring")):
        result="<h3>Text-based retrieval result</h3>\n"
        queryfield = "textquerystring"
        cmd = "textexpand"
    else:
        result="<h3>Retrieval Result</h3>\n"
        queryfield = "querystring"
        cmd = "expand"
    queryimages=form[queryfield].value
    resultsstep=int(form["resultsstep"].value)
    resultsstep=resultsstep+1
    s.sendcmd(cmd+" "+str(resultsstep)+" "+queryimages)
    msg=s.getline()
    tokens=re.split(" ",msg)
    tokens.reverse()
    result=result+displayResults(tokens,queryfield,queryimages,resultsstep)
    return result
   

# ----------------------------------------------------------------------
# process a query using text- or metainformation from the text input
# field. The query type is set to metaquery when the search string has
# a leading "META "
# This works identical as the retrieve function
# ----------------------------------------------------------------------
def processTextQuery(form):
    # Check if it's meta or text
    
    # It's meta either if there's only the metafeature or it has a leading "META "
    hasmeta = "metafeature" in settings.distances
    hastext = "textfeature" in settings.distances
    querystring = form["textquerystring"].value
    if (hasmeta and (not hastext or querystring[:5] == "META ")):
        if(querystring[:5] == "META "):
            querystring = querystring[5:]        
        querytype = "metaquerystring"
        result="<h3>Metatag-based retrieval result</h3>\n"
        s.sendcmd("metaretrieve +"+querystring)
    else:
        querytype = "textquerystring"
        result="<h3>Text-based retrieval result</h3>\n"
        s.sendcmd("textretrieve +"+querystring)

    msg=s.getline()
    tokens=re.split(" ",msg)
    if( (tokens[0] == "notextinformation") or (tokens[0] == "nometainformation") ):
        result=result+"No images matched your query: "+querystring
        if(querytype == "metaquerystring"):
            if(hastext):
                result=result+"<br><br>Check if the syntax is correct.<br><br>Example: 'META key1:val1,key2:val2,...'<br><br>\n"
            else:
                result=result+"<br><br>Check if the syntax is correct.<br><br>Example: 'key1:val1,key2:val2,...'<br><br>\n"
        result=result+"Also have a look at the help to see which keys are available.<br>\n"
    else:
        tokens.reverse()
        result=result+displayResults(tokens,querytype,querystring,0)
    return result
    

# ----------------------------------------------------------------------
# create the link for the "settings" at the bottom of the page
# ----------------------------------------------------------------------
def adminLink(fireServer, firePort):
    result=""
    result=result+"<a name=\"#\"></a>"
    result=result+"<a href=\"#\" "+\
            "onClick=\"window.open('fire.cgi?server="+ \
            fireServer+"&port="+str(firePort)+"&settings=1','newwindow',"+\
            "'height=600, width=600')\">settings</a>"
    return result

    #<SCRIPT LANGUAGE="javascript">
    #<!--
    #window.open ('titlepage.html', 'newwindow', config='height=100,
    #width=400, toolbar=no, menubar=no, scrollbars=no, resizable=no,
    #location=no, directories=no, status=no')
    #-->
    #</SCRIPT>

    


# ----------------------------------------------------------------------
# the main program
# ----------------------------------------------------------------------

# get form information in easily accessible manner
form=cgi.FieldStorage()

# make socket
s = FIRESocket()
settings=FireSettings()
# see what server we have and if the webinterface specifed another one
# than the one in the config file
if form.has_key("server"):
    settings.fireServer=form["server"].value
if form.has_key("port"):
    settings.firePort=int(form["port"].value)

try:
    # connect to the server
    s.connect(settings.fireServer, settings.firePort)
except:
    # if the server is down, give an error message
    #    print "</pre>"
    message="""
    <h2>FIRE Server down</h2>

    <p>
    Try again later, complain to
    <a href="mailto:deselaers@informatik.rwth-aachen.de">deselaers@informatik.rwth-aachen.de</a>
    
    or try other servers.
    """
    Display(message)
else:
    # everything is fine, connection to server is ok
    try:
        message=""
        settings.get(s)
        
        # do we want to show the settings window?
        if(form.has_key("settings")):
            # see if there have been new settings defined
            message=settings.process(form)
            # generate the settings dialog
            message+=settings.display()
            # disonnect from server
            s.sendcmd("bye")
            print "</pre>"
            # display the settings dialog
            Display(message)
        # do we want to show the metafeatureinfo window?
        elif(form.has_key("metafeatureinfo")):
            message+=displayMetaFeatureInfo()
            s.sendcmd("bye")
            print "</pre>"
            Display(message)
	elif(form.has_key("textfeatureinfo")):
            message+=displayTextFeatureInfo()
            s.sendcmd("bye")
            print "</pre>"
            Display(message)
        else:
            # we do not want to show the settings window
            if(form.has_key("feedback")):
                # result images have been selected and are part of this query
                if form["feedback"].value=="requery":
                    # relevance feedback
                    message+=feedbackretrieve(form)
                elif form["feedback"].value=="expandresults":
                    # more results
                    message+=expandQuery(form)
                else:
                    # save relevances (this will not happen currently)
                    message=saveRelevances(form)
            elif(form.has_key("queryImage")):
                # no relevance feedback, but just one of the random images was clicked
                message+=retrieve(form)
            elif form.has_key('textquerystring'):
                # no relevance feedback, no query image, but a query string for text queries
                message+=processTextQuery(form)

            # now generate the remainder of the website: queryfield for meta information, random images, adminlink
            if("textfeature" in settings.distances or "metafeature" in settings.distances):
                message+=textQueryForm(settings)

            message+=randomImages()
            message+="<p align=\"right\">"+adminLink(settings.fireServer, settings.firePort)+"</p>"
            # disconnect from server
            s.sendcmd("bye")
            print "</pre>"
            # display the generated webpage
            Display(message)
    
    except Exception, e:
        # if anything went wrong:
        # show the error message as readable as possible,
        # disconnect from the server to keep it alive
        # and end yourself
        print "<pre>"
        print "Exception: ",e
        s.sendcmd("bye")
        print "</pre>"
