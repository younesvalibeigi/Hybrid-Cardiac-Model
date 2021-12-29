# -*- coding: utf-8 -*-
"""
Created on Sat Jul 10 12:37:44 2021

@author: youne
"""
import numpy as np
import matplotlib.pyplot as plt
from tabulate import tabulate
import math
from scipy.misc import electrocardiogram
from scipy.signal import find_peaks
import numpy as np

fullData = []
strcycleP = []



f = open("BubBinaryData_Tissue4_PaceMaker.roi", "r")
for row in f:
    fullData.append(row)
    x1=0
    x2=0
    cc=0
    x1found = 0
    for x in row:
        if (x==' ' and x1found==0):
            x1=cc
            x1found=1
        if(x=='\n'):
            x2=cc
        cc = cc+1
    strcycleP.append(row[x1+1:x2-1])

data = np.zeros(len(strcycleP)-1)
for x in range(len(strcycleP)-1):
    data[x] = (int(strcycleP[x]))


    
peaks, _ = find_peaks(data, height=1050)
distance = np.zeros(len(peaks)-1)
for x in range(len(peaks)-1):
    distance[x] = peaks[x+1] - peaks[x]
    

#plt.plot(dataCam)
#plt.plot(dataNode)
plt.plot(data[38:370])
#plt.plot(peaks, data[peaks], "x")
#plt.plot(np.zeros_like(x), "--", color="gray")
plt.ylabel('Signal intensity')
plt.xlabel('frames')
#plt.title("FK model-set10 (NodeJS) \n ROI=Pacemaker \n 30 FPS")
plt.title("FK model\n ROI=Pacemaker \n 30 FPS")
#xint = range(0, math.ceil(len(data))+1)
#plt.xticks(xint)
plt.rcParams["figure.figsize"] = (10,3)
plt.show()

