import re,sys,string
class FileList:
    def __init__(self):
        self.files=[]
        self.classes=False
        self.featuredirectories=False
        self.descriptions=False
        self.path=""
        self.suffices=[]

    def __str__(self):
        result=""
        result+="Suffices: "+str(self.suffices)
        result+="Files: "+str(self.files)
        return result
        
    def __getitem__(self,i):
        return self.files[i]

    def __getslice__(self,i):
        return self.files[i]
        
    def load(self, filename):
        fl=open(filename,"r")
        lines=fl.readlines()

        for l in lines:
            l=re.sub("\n","",l)
            tokens=string.split(l)             
            if tokens[0] == "file":
                f=tokens[1]
                if self.classes==True:
                    cls=int(tokens[2])
                    if self.descriptions:
                        desc=tokens[3:]
                        self.files+=[(f,cls,desc)]
                    else:
                        self.files+=[(f,cls,[])]
                else:
                    self.files+=[(f,)]
                if self.descriptions:
                    desc=tokens[2:]
                    self.files+=[(f,0,desc)]
            elif tokens[0] == "classes":
                if tokens[1]=="yes":
                    self.classes=True
            elif tokens[0] == "descriptions":
                if tokens[1]=="yes":
                    self.descriptions=True
            elif tokens[0] == "featuredirectories":
                if tokens[1]=="yes":
                    self.featuredirectories=True
            elif tokens[0] == "path":
                self.path=tokens[1]
            elif tokens[0] == "suffix":
                self.suffices+=[tokens[1]]
#            else:
#                print >>sys.stderr,"not parsed: ", tokens
