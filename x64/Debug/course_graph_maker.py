import matplotlib.pyplot as plt
# import subprocess
import numpy as np
import os

dots_x = []
dots_y = []

# dots_x = [19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0]
# dots_y = [0.001000, 0.000000, 0.002000, 0.002000, 0.003000, 0.003000, 0.004000, 0.004000, 0.006000, 0.014000, 0.034000, 0.041000, 0.076000, 0.109000, 0.207000, 0.339000, 0.313000, 0.433000, 0.587000, 0.881000]

with open(r'C:\Users\puuni\source\repos\balda_3_sem_orig\balda_3_sem\course_work.txt', 'r') as file:
    dots_x = file.readline().split(r', ')
    dots_y = file.readline().split(r', ')

print(dots_x)
print(dots_y)

dots_x = [int(x) for x in dots_x[:-1]]
dots_y = [float(str(x).replace(',', '.')) for x in dots_y[:-1]]

plt.xlabel('Количество свободных клеток, шт ')
plt.plot(dots_x, dots_y)
plt.ylabel('Время работы алгоритм, с')


plt.yticks(np.arange(0.0, 1, 0.1))
plt.xticks(np.arange(0, 21, 5))

plt.grid()
plt.show()