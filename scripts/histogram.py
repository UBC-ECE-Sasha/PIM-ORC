import numpy as np
import matplotlib.mlab as mlab
import matplotlib.pyplot as plt


def NonZero(variable):
    if (variable == 0):
        return False
    else:
        return True

threads = ['5', '6', '7', '10', '15', '20', '25', '50', '100', '1000']
colors = ['blue', 'red', 'purple', 'green', 'yellow', 'orange', 'pink', 'cyan', 'brown', 'olive']

num_bins = 10
for i in range(10):
    with open("store_sales10_32KB_" + threads[i] + ".log", "r") as f:
        matrix=[x.strip().split('\t') for x in f]
        matrix_rev = list(zip(*matrix))
        matrix_rev_int = [int(num) for num in matrix_rev[1]]
        matrix_rev_int_filtered = filter(NonZero, matrix_rev_int)
        row = list(matrix_rev_int_filtered)
        n, bins, patches = plt.hist(row, num_bins, facecolor=colors[i])
        avg = sum(row)/len(row)
        print("Avg for ",threads[i], "is: ", avg)
        f.close()
plt.xlabel('# Requests Unloaded / 3 seconds')  
plt.ylabel('Frequency')
plt.show()



