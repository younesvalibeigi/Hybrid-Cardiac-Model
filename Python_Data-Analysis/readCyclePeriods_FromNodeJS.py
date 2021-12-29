# -*- coding: utf-8 -*-
"""
Created on Sat Jul 10 12:37:44 2021

@author: youne
"""
import numpy as np
import matplotlib.pyplot as plt
from tabulate import tabulate
import math

fullData = []
strcycleP = []



f = open("cal_Tissue1_ArduinoFD.txt", "r")
for row in f:
    fullData.append(row)
    x1=0
    x2=0
    cc=0
    for x in row:
        if (x==' '):
            x1=cc
        if(x=='\n'):
            x2=cc
        cc = cc+1
    strcycleP.append(row[x1+1:x2])

data = []
for x in range(len(strcycleP)-1):
    data.append(int(strcycleP[x]))


#plt.plot(dataCam)
#plt.plot(dataNode)
plt.plot(data)
plt.ylabel('cycle period duration (ms)')
plt.xlabel('cycle periods')
plt.title("FK model-set4 (NodeJS) \n The evolution of cycle periods \n 30 FPS")
#xint = range(0, math.ceil(len(data))+1)
#plt.xticks(xint)
plt.show()

