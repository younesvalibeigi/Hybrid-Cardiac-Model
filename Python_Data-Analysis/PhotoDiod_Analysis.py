# -*- coding: utf-8 -*-
"""
Created on Mon Apr 26 16:21:04 2021

@author: Younes Valibeigi
"""

import csv
import numpy as np
import matplotlib.pyplot as plt

#fullData = np.array()
fullData = [];

with open('Jun 26, 2021 4-30-49 PM.csv', 'r') as file:

    reader = csv.reader(file)
    for row in reader:
        fullData.append(row)

nums = []
thecc = 0
for row in range(1,len(fullData[:])-10):
    #if (float(fullData[row][1]) !=0):
    nums.append(float(fullData[row][1]))
    thecc = thecc+1

c = 0
picks = []
while(c<len(nums)):
    if(nums[c]<-2000):
        picks.append(c)
        c+=50
    c+=1

diffs = []
for i in range(1, len(picks)):
    diffs.append(picks[i]-picks[i-1])

#Drop outliers ==> The missed waves

nonOutlier = []
for i in range(5,len(diffs)):
    if(diffs[i]<1110 and diffs[i]>1090):
        nonOutlier.append(diffs[i])

plt.hist(nonOutlier, bins=1)
plt.xlabel("time interval (ms)")
plt.ylabel("counts")
plt.title("Second Design-Camera Frame Rate=30 FPS")
plt.xlim(1100,1130)
plt.show()

#nonOutlier = nonOutlier[:100]

plt.plot(nonOutlier)
plt.ylabel('cycle period duration (ms)')
plt.xlabel('cycle periods')
plt.title("Real Tissue with Fixed Delay (3000ms) \n The evolution of cycle periods \n 30 FPS")

plt.show()


mean = np.mean(nonOutlier)
variance = np.var(nonOutlier)
std = np.std(nonOutlier)
print("Mean = ", mean, "ms")
print("Standard Deviation = ", std, " ms")
print("1 Frame = 33.33 ms")

from tabulate import tabulate

table4 = [['Camera FPS', 'mean cycle periods (ms)', 'std (ms)', 'camera frame period (ms)']]
table4.append([30.0, mean, std, 33.3333])
print("")
print(tabulate(table4, headers='firstrow', tablefmt='fancy_grid'))


plt.plot(nums[1000:10000])
plt.ylabel('signal intensity')
plt.xlabel('time (ms)')
plt.title("Photodiode Signal")

plt.show()




