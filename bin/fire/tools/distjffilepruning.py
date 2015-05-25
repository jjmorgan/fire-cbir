#!/usr/bin/python
import sys,re,math,bisect
from UserList import UserList

# a simple class for a sorted list
class SortedList(UserList): 
    def append(self, item): 
        bisect.insort(self.data, item)

# command line processing
if len(sys.argv)!=4:
    sys.stderr.write("USAGE: distjffilepruning <jfile> <posPerQuery> <negPerQuery>\n")
    sys.exit(20)
    
jf=open(sys.argv[1],"r")
posPerQuery=int(sys.argv[2])
negPerQuery=int(sys.argv[3])

# read file
lines=jf.readlines()
lines=map(lambda line:
          re.sub("\n","",line), lines)
print lines[0]
lines=lines[1:-1]

numberOfLines=len(lines)
numberOfFiles=int(math.sqrt(numberOfLines))
sys.stderr.write("Got "+str(numberOfLines)+" lines, "+str(numberOfFiles)+" files.\n")
if numberOfFiles*numberOfFiles!=numberOfLines:
    sys.stderr.write("Inconsistent!\n")
    sys.exit(20)

# iterate over lines
for i in range(numberOfFiles):
    positives=SortedList([]) # sorted list for relevant vectos
    negatives=SortedList([]) # sorted list for irrelevant vectors
    for j in range(numberOfFiles):
        lineno=i*numberOfFiles+j       # get line to be processed
        l=lines[lineno]
        tokens=re.split(" ",l)
        cls=int(tokens[0])   # get class (1-> relevant, 0->irrelevant)
        summe=0              # calculate sum of distances
        for n in tokens[1:]:
            summe+=float(n)
        if cls==1:           # if relevant -> append to positives, otherwise append to negatives
            positives.append((summe,l))
        else:
            negatives.append((summe,l))
    # print the nearest neighbors of relevant images, as many as requested
    for n in range(min(posPerQuery,len(positives))):
        print positives[n][1]
    # print the nearest neighbors of irrelevant images, as many as requested
    for n in range(min(negPerQuery,len(negatives))):
        print negatives[n][1]

# print end of joergfile
print "-1"
