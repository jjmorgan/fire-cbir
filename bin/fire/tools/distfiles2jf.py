#!/usr/bin/python

import sys,os,re

import filelist

def USAGE():
    print """
USAGE: distfiles2jf.py [options] -l <filelist> -q <querylist> -o <outfile>
    with
      -l <filelist> to specify the filelist of the database
                    which was used for the creation of these distance files
      -q <querylist> to specify the querylist which was used to create the
                     distancefiles. This is necessary to know the relevances
                     and can be obtained from a filelist
      -o <outfile> to specify the joergfile where all the distances are going
                   to be saved
      -s <suffix> to specify the suffices of the distance files (default: ".dists")
      """
class QueryList:
    def __init__(self):
        self.queries=[]
    def load(self, filename):
        fh=open(filename,'r')
        lines=fh.readlines()
        lines=map(lambda line:
                  re.sub("\n","",line), lines)
        for l in lines:
            tokens=re.split(" ",l)
            self.queries+=[ (tokens[0], tokens[1:]) ]
    
class DistFile:
    def __init__(self):
        self.distances=[]

    def load(self, filename):
        self.distances=[]
        fh=open(filename,'r')
        lines=fh.readlines()
        lines=map(lambda line:
                  re.sub("\n","",line), lines)
        for l in lines:
            tokens=re.split(" ",l)
            if tokens[0] != '#' and tokens[0] != "nofdistances":
                #print "tokens[0] =", tokens[0]
                no=int(tokens[0])
                dists=[]
                for d in tokens[1:]:
                    dists+=[ float(d) ]
                self.distances+=[ (no,dists) ]


#------------------------------ main from here ------------------------------
filelist=filelist.FileList()
querylist=QueryList()
distfile=DistFile()

# processcommandline

distfilelist=[]
if (not "-l" in sys.argv) or (not "-q" in sys.argv) or (not "-o" in sys.argv):
    USAGE()
    sys.exit(1)
suffix = ".dists"
i=1 # start right after program name
while i<len(sys.argv):
    opt=sys.argv[i]
    i=i+1
    if opt=="-l":
        filelistname=sys.argv[i]
        i=i+1
    elif opt=="-q":
        querylistname=sys.argv[i]
        i=i+1
    elif opt=="-o":
        outfilename=sys.argv[i]
        i=i+1
    elif opt=="-s":
        suffix=sys.argv[i]
        i=i+1
    else:
        print "Unprocessed option:",opt
        ++i
    
        
# load filelist and load querylist
filelist.load(filelistname)
querylist.load(querylistname)

outfile=open(outfilename,"w")
N=len(filelist.files)

prozent=0;
prozent10=len(querylist.queries)/10
for i in range(len(querylist.queries)):
    if i%prozent10==0:
        print str(prozent)+"% done."
        prozent+=10
    q=querylist.queries[i][0]
    rels=querylist.queries[i][1]
    #outfile.write(q); outfile.write(str(rels)); outfile.write("\n")
    #distfile.load(q+".dists")
    distfile.load(q+suffix)
    if i==0:
        outfile.write("2 "+str(len(distfile.distances[0][1]))+"\n")
    for n in range(N):
        cls=0
        if (filelist.files[n][0] in rels) or filelist.files[n][0]==q:
            cls=1
        line=""
        #line+=str(q)+" "+str(filelist.basenames[n])+" "
        line+=str(cls)
        for d in distfile.distances[n][1]:
            line+=" "+str(d)
        outfile.write(line+"\n")
outfile.write("-1\n")        
outfile.close()
        
