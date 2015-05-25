#!/usr/bin/python
import sys, socket, re, os, string, time, math

######################################################################
# settings for casimage
#queryFileList="/u/europium/deselaers/casimage/Query/purefiles"
#reweighter="~/work/src/fire/distancereweighter"
#dbfilelist="/u/europium/deselaers/casimage/alllist.fire"
#
##equal weights:
#weights=[1,1,1,1,1,1];runtag="i6-111111"
##weights=[0,2,0,5,0,0];runtag="i6-020500"
##weights=[0,2,5,5,0,1];runtag="i6-025501"


######################################################################
# settings for st.andrews
queryFileList="/u/europium/deselaers/st.andrews/queries/querylist"
reweighter="~/work/src/fire/distancereweighter"
dbfilelist="/u/europium/deselaers/st.andrews/imageclef01.list"
#0suffix globtexturefeat.oldvec.gz
#1suffix tamura.oldhisto.gz
#2suffix tnrgb.32x32.png
#3suffix tnrgb.Xx32.png
#4suffix X00X40.rel.oldhisto.gz
#5suffix X40X08.rgb.oldhisto.gz
#weights=[1,1,1,1,1,1];runtag="i6-111111"
#weights=[0,1,0,0,1,2];runtag="i6-010001"
weights=[0,1,0,1,0,1];runtag="i6-010101"



def getQueries(queryFileList):
    fp=open(queryFileList,"r")
    lines=fp.readlines()
    lines=map(lambda line:
              re.sub("\n","",line), lines)
    fp.close()
    return lines

def reweight(query,weights):
    result=[]
    cmdline=reweighter+" -f "+dbfilelist+" -r 1030 -d "+query+".dists"
    for i in range(len(weights)):
        cmdline=cmdline+" -w "+str(i)+" "+str(weights[i])

    print >> sys.stderr, cmdline
    reweighterHandle=os.popen(cmdline,"r")
    reweighterOut=reweighterHandle.readlines()
    reweighterOut=map(lambda line:
                      re.sub("\n","",line), reweighterOut)


    for line in reweighterOut:
        if(re.match("^RESULT:", line)):
           fields=re.split(" ",line)
           img=fields[1]
           score=fields[2]
           result=result+[(img,score)]

    return result



queries=getQueries(queryFileList)
i=1
for query in queries:
    reweighted=reweight(query,weights)
    j=0
    for entry in reweighted:
        img=entry[0]
        if j<1000:
            if not re.match("^Query",img):
                img=re.sub("images/","",img)
                score=float(entry[1])
                print "%-3i 1 %-20s %3i %3.5f %-12s" %(i,img,j,math.exp(-score),runtag)
                j=j+1
    i=i+1



