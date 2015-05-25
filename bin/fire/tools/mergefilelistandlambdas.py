#!/usr/bin/python

import filelist,sys,re,gzip


class LambdaFile:
    def __init__(self):
        self.lambdas=[]
        self.nofclasses=0
        self.factor=0.0
        self.offset=0.0
        self.nofLCls=0
        self.nofLCom=0
        self.nofLPool=0

    def load(self,filename):
        f=gzip.open(filename,"r")
        lines=f.readlines()
        lines=map(lambda line:
                  re.sub("\n","",line), lines)

        self.nofclasses=int(lines[0])

        tok=re.split(" ",lines[2])
        self.factor=float(tok[0])
        self.offset=float(tok[1])

        tok=re.split(" ",lines[4])
        self.nofLCls=int(tok[0])
        self.nofLCom=int(tok[1])
        self.nofLpool=int(tok[2])

        for c in range(self.nofclasses):
            l=[]
            for i in range(self.nofLCls+self.nofLCom+self.nofLpool):
                l+=[float(lines[c*(1+self.nofLCls+self.nofLCom+self.nofLpool)+i+6])]
            self.lambdas+=[l]
 #       print self.lambdas
    

fl=filelist.FileList()
fl.load(sys.argv[1])

lambdas=LambdaFile()
lambdas.load(sys.argv[2])


print "  lambdas:"
for i in range(len(fl.suffices)):
    print i,fl.suffices[i],
    for j in range(len(lambdas.lambdas)):
        print lambdas.lambdas[j][i],
    print

print len(fl.suffices),"const",
for j in range(len(lambdas.lambdas)):
    print lambdas.lambdas[j][-1],
print

