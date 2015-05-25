#!/usr/bin/python

# easy script for automatic batches with fire
# takes database files for queries
# calculates error rate only

import sys, socket, re, os, string, time, traceback
import firesocket,filelist
# forget own name in commandline options
sys.argv.pop(0)

server="localhost"
port=12960
database=""
querybase=""
l1o=False
quit=False

while len(sys.argv)>0:
    argv=sys.argv.pop(0)
    if argv=="-s":
        server=sys.argv.pop(0)
    elif argv=="-p":
        port=int(sys.argv.pop(0))
    elif argv=="-f":
        database=sys.argv.pop(0)
    elif argv=="-q":
        querybase=sys.argv.pop(0)
    elif argv=="-l1i":
        l1o=True
    elif argv=="-x":
        quit=True
    else:
        print "Unknown option:",argv
        print """
USAGE: queryfilelistwcls.py <options>
      -h  show this help
      -s  server (default: localhost)
      -p  port (default: 12960)
      -f  database 
      -q  database for query
      -l1o make leaving one out with database
      -x  exit after having finished
"""
        sys.exit(10)

print "SETTINGS: server=",server,"port=",port,"database=",database,"querybase=",querybase,"l1o=",l1o

if database=="" or querybase=="" and not l1o:
    print """
USAGE: queryfilelistwcls.py <options>
      -h  show this help
      -s  server (default: localhost)
      -p  port (default: 12960)
      -f  database 
      -q  database for query
      -l1o make leaving one out with database
      -x  exit after having finished
"""
    sys.exit(10)
    

print "SETTINGS: server=",server,"port=",port,"database=",database,"querybase=",querybase,"l1o=",l1o
sys.stdout.flush()
f=filelist.FileList()
f.load(database)
q=filelist.FileList()
if  not l1o:
    q.load(querybase)


# get classes from filelist
fname2cls={}
maxcls=0
for i in f.files:
    fname2cls[i[0]]=int(i[1])
    if i[1]>maxcls:
        maxcls=i[1]
print "Database has",maxcls,"classes."

if not f.classes:
    print "Need classes in database file"
    sys.exit(10)

if not l1o and not q.classes:
    print "Need classes in querybase file"
    sys.exit(10)
    

s=firesocket.FIRESocket()
s.connect(server,port)
try:
    s.sendcmd("info")
    res=s.getline()
    status=re.split(" ",res)
    keyword=status.pop(0)
    dbSize=int(status.pop(0))
    if dbSize!=len(f.files):
        print "database in retriever and database loaded are of different size:", dbSize,"!=",len(f.files)
        s.sendcmd("bye")
        time.sleep(1)
        sys.exit(10)

    print "database has",dbSize,"images."
    s.sendcmd("setresults "+str(dbSize)); res=s.getline()

    classified=0; errors=0; correct=0

    if not l1o:
        for qf in q.files:
            qname=qf[0]
            qcls=int(qf[1])
            resultsmap={}
            s.sendcmd("retrieve "+qname)
            res=s.getline()
            res=re.sub('[ ]*$','',res) # and parse the results
            results=re.split(" ",res)
            if len(results)!=2*dbSize:
                print "expected",2*dbSize,"tokens, got",len(results)
                sys.exit(10)

            while len(results)>0:
                score=float(results.pop())
                returnedimage=results.pop()
                dcls=fname2cls[returnedimage]
                resultsmap[dcls]=(returnedimage,score)

            classified+=1
            if dcls==qcls:
                correct+=1
            else:
                errors+=1
            print "Query:",qname, "NN:",returnedimage,"ER:",float(errors)/float(classified)*100

            print "NNs:",
            for i in resultsmap:
                print i,resultsmap[i][0],resultsmap[i][1],
            print
            sys.stdout.flush()
    else:
        print "NYI"
        sys.exit(10)

    print "RESULT: ER:",float(errors)/float(classified)*100
    time.sleep(1)
    if quit:
        s.sendcmd("quit")
    else:
        s.sendcmd("bye")
    time.sleep(1) #don't kill the server by exiting tooo fast 

except KeyboardInterrupt, e:
    s.sendcmd("bye")
    print e

    time.sleep(1) #don't kill the server by exiting tooo fast 

except Exception, e:
    if quit:
        s.sendcmd("quit")
    else:
        s.sendcmd("bye")
    print e
    time.sleep(1)
