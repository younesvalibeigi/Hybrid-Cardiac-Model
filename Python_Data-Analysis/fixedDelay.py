# -*- coding: utf-8 -*-
"""
Created on Mon Apr 26 16:21:04 2021

@author: Younes Valibeigi
"""

import csv
import numpy as np
import matplotlib.pyplot as plt

names  = ['Apr 30, 2021 5-07-39 PM_FR30Proj10.csv',
          'Apr 30, 2021 4-42-12 PM_FR30Proj20.csv',
          'Apr 30, 2021 7-33-35 PM_FR30Proj30.csv',
          'Apr 30, 2021 5-43-59 PM_FR30_Proj40.csv',
          'Apr 30, 2021 7-15-10 PM_FR30_Proj50.csv',
          'Apr 30, 2021 6-07-13 PM_FR20Proj30.csv',
          'Apr 30, 2021 7-33-35 PM_FR30Proj30.csv',
          'Apr 30, 2021 6-32-13 PM_FR40Proj30.csv',
          'Apr 30, 2021 6-52-36 PM_FR50Proj30.csv']

titles = ['Projector frame = 10 ms (FPS = 30)',
          'Projector frame = 20 ms (FPS = 30)',
          'Projector frame = 30 ms (FPS = 30)',
          'Projector frame = 40 ms (FPS = 30)',
          'Projector frame = 50 ms (FPS = 30)',
          'Camera Frame Rate = 20 FPS (Proj = 30 ms)',
          'Camera Frame Rate = 30 FPS (Proj = 30 ms)',
          'Camera Frame Rate = 40 FPS (Proj = 30 ms)',
          'Camera Frame Rate = 50 FPS (Proj = 30 ms)']

oneFrame = [1000/30, 1000/30, 1000/30, 1000/30, 1000/30, 1000/20, 1000/30, 1000/40, 1000/50]
means = []
stds = []
#fig1, ax1 = plt.subplots(2,5)
#fig2, ax2 = plt.subplots(2,5)
for i in range(len(names)):
    fullData = [];
    with open(names[i], 'r') as file:
        reader = csv.reader(file)
        for row in reader:
            fullData.append(row)
    
    nums = []
    for row in range(1,len(fullData[:])):
        #if (float(fullData[row][1]) !=0):
        nums.append(float(fullData[row][1]))
    
    c = 0
    picks = []
    while(c<len(nums)):
        if(nums[c]<-2000):
            picks.append(c)
            c+=50
        c+=1
    
    diffs = []
    for xx in range(1, len(picks)):
        diffs.append(picks[xx]-picks[xx-1])
    
    #if(i<5):
    plt.hist(diffs)
    plt.xlabel("time interval (ms)")
    plt.ylabel("counts")
    plt.title(titles[i])
    plt.show()
    
    plt.plot(diffs)
    plt.ylabel('cycle period (ms)')
    plt.xlabel('period number')
    plt.title(titles[i])
    plt.show()
    
    if (i==6):
        plt.hist(diffs)
        plt.xlabel("time interval (ms)")
        plt.ylabel("counts")
        plt.title("First Design-Camera Frame Rate=30 FPS")
        plt.show()
        
    
    '''else:
        plt.hist(diffs)
        plt.xlabel("time interval (ms)")
        plt.ylabel("counts")
        plt.title(titles[i])
        plt.show()
        
        plt.plot(diffs)
        plt.ylabel('cycle period (ms)')
        plt.xlabel('period number')
        plt.title(titles[i])
        plt.show()'''
    
    
    means.append(np.mean(diffs))
    stds.append(np.std(diffs))
    
plt.plot([10,20,30,40,50], means[:5])
plt.xlabel("Projector's frame period")
plt.ylabel("Projector's period (ms)")
plt.title("Projector's period = 64 X frame")
plt.show()

plt.bar([10,20,30,40,50], oneFrame[:5], label="Frame Period",color='b',width=2.5)
plt.bar([12.5,22.5,32.5,42.5,52.5], stds[:5], label="STD",color='lightskyblue',width=2.5)
plt.xlabel("Projector's frame period")
plt.ylabel("(ms)")
plt.title("STD vs one camera frame")
plt.legend()
plt.show()

plt.plot([20,30,40,50], means[5:])
plt.ylim([0,4000])
plt.xlabel("camera frame rate")
plt.ylabel("Projector's period (ms)")
plt.title("Projector's period = 65 X frame")
plt.show()

plt.bar([20,30,40,50], oneFrame[5:], label="Frame Period",color='b',width=2.5)
plt.bar([22.5,32.5,42.5,52.5], stds[5:], label="STD",color='lightskyblue',width=2.5)
plt.xlabel("camera frame rate")
plt.ylabel("(ms)")
plt.title("STD vs one camera frame")
plt.legend()
plt.show()

from tabulate import tabulate
table1 = [['Camera FPS', 'Proj 1 frame (ms)', '64X projFrame (ms)', 'mean period (ms)', '1 camera frame (ms)', 'std (ms)']]
for i in range(5):
    proj1frame = i*10+10
    table1.append([30, proj1frame, 64*proj1frame, means[i], oneFrame[i], stds[i]])

print(tabulate(table1, headers='firstrow', tablefmt='fancy_grid'))

table2 = [['Camera FPS', 'Proj 1 frame (ms)', '64X projFrame (ms)', 'mean period (ms)', '1 camera frame (ms)', 'std (ms)']]
for i in range(5,9):
    FRPS = (i-3)*10
    table2.append([FRPS, 30, 64*30, means[i], oneFrame[i], stds[i]])

print(tabulate(table2, headers='firstrow', tablefmt='fancy_grid'))


table3 = [['Camera FPS', 'mean cycle periods (ms)', 'std (ms)', 'camera frame period (ms)']]
for i in range(5,9):
    FRPS = (i-3)*10
    table3.append([FRPS, means[i], stds[i], oneFrame[i]])
print("")
print(tabulate(table3, headers='firstrow', tablefmt='fancy_grid'))

table4 = [['Camera FPS', 'mean cycle periods (ms)', 'std (ms)', 'camera frame period (ms)']]
table4.append([30.0, means[6], stds[6], oneFrame[6]])
print("")
print(tabulate(table4, headers='firstrow', tablefmt='fancy_grid'))


