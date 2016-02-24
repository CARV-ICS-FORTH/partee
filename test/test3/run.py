#! /usr/bin/python
import subprocess
import sys
cores=["1","2","4","8","16","32","64"]
default_args=["128","30000"];
if(len(sys.argv) < 3):
        print("Please choose an executable file and number of executions.");
        sys.exit()
subprocess.call(["mkdir", "-p", "blsch_dumps"])
for i in cores:
        print("####################################### "+i+" Cores ####################################################\n")
        if(len(sys.argv) == 3): 
                for x in xrange(0,int(sys.argv[2])):
                        print("------------------------------------------- "+str(x)+" ------------------------------------------------\n")
                        print ("./"+str(sys.argv[1])+" "+default_args[0]+" "+default_args[1]+" "+i)
                        subprocess.call(["./"+str(sys.argv[1]), default_args[0], default_args[1], i])
        elif(len(sys.argv) == 5):
                for x in xrange(0,int(sys.argv[4])):
                        print("------------------------------------------- "+str(x)+" ------------------------------------------------\n")
                        print ("./"+str(sys.argv[1])+" "+ str(sys.argv[2])+" "+str(sys.argv[3])+" "+i)
                        subprocess.call(["./"+str(sys.argv[1]), str(sys.argv[2]),str(sys.argv[3]), i])
                        subprocess.call(["cp", "-f", "stats.csv", "blsch_dumps/stats_"+i+"_"+str(x)+"i.csv"])
        else:
                print("Usage: "+str(sys.argv[0])+" <executable_file> <arg1(optional)> <arg2(optinal)> <number of executions>")
                print("Default args: arg1="+default_args[0]+" arg2="+default_args[1])
                sys.exit()
        print("------------------------------------------------------------------------------------------------\n")
        print("########################################################################################################\n")
