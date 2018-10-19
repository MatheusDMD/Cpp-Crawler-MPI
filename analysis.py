import subprocess
import os
import os.path
import numpy as np
import matplotlib.pyplot as plt
size_list = [10,50,100,200,300,400,500]

def calculate_times(compilation_type):

    calculated = False
    for size in size_list:
        if(not os.path.isfile("outputs/{0}{1}.txt".format(compilation_type,size))):
            calculated = False
            break
        calculated = True
        
    if(not calculated):
        os.environ["STRATEGY"] = str(compilation_type)
        subprocess.run("cmake . && make", shell=True)
    times = []
    for size in size_list:
        if(not calculated):
            if(not os.path.isfile("outputs/{0}{1}.txt".format(compilation_type,size))):
                with open("outputs/{0}{1}.txt".format(compilation_type,size),'w') as output:
                    subprocess.run("COLOR=0 GUI=0 ./visualizador < inputs/input{0}.txt".format(size), shell=True, stdout=output)
            
        with open("outputs/{0}{1}.txt".format(compilation_type,size),'r') as output_file: 
            lines = output_file.readlines()
            times.append(float(lines[-1].split()[-1]))
            
    return times

if __name__ == "__main__":
    sequencial_times = calculate_times("sequencial")
    simd_times = calculate_times("simd")
    omp_times = calculate_times("omp")
    
    # sequencial, = plt.plot(size_list,sequencial_times, 'ro', label="sequencial")
    # simd, = plt.plot(size_list,simd_times, 'bo', label="simd")
    # omp, = plt.plot(size_list,omp_times, 'go', label="omp")
    # plt.legend([sequencial,simd,omp], ["sequencial","simd","omp"])
    # plt.savefig("analysis.png")
    # plt.show()

    def autolabel(rects):
        for rect in rects:
            h = rect.get_height()
            ax.text(rect.get_x()+rect.get_width()/2., 1.05*h, '%d'%int(h),
                    ha='center', va='bottom')

    N = len(size_list)
    width = 0.27
    ind = np.arange(N)
    fig = plt.figure()
    ax = fig.add_subplot(111)
    rects1 = ax.bar(ind, sequencial_times, width, color='r')
    rects2 = ax.bar(ind+width, simd_times, width, color='g')
    rects3 = ax.bar(ind+width*2, omp_times, width, color='b')
    ax.set_ylabel('Time spent by compilation STRATEGY')
    ax.set_xlabel('number of circles in the simulation')
    ax.set_xticks(ind+width)
    ax.set_xticklabels( ("10","50","100","200","300","400", "500") )
    ax.legend( (rects1[0], rects2[0], rects3[0]), ('sequencial', 'simd', 'omp') )
    plt.savefig("analysis.png")
    plt.show()

    perc_simd = [100*(simd_times[i]/sequencial_times[i]) for i in range(0, len(simd_times))]
    perc_omp = [100*(omp_times[i]/sequencial_times[i]) for i in range(0, len(simd_times))]

    N = len(size_list) - 2
    width = 0.27
    ind = np.arange(N)
    fig = plt.figure()
    ax = fig.add_subplot(111)
    rects1 = ax.bar(ind, perc_simd[2::], width, color='g')
    rects2 = ax.bar(ind+width, perc_omp[2::], width, color='b')
    ax.set_ylabel('%\ of time spent in comparison with sequential compilation')
    ax.set_xlabel('number of circles in the simulation')
    ax.set_xticks(ind+width)
    ax.set_xticklabels( ("100","200","300","400","500") )
    ax.legend( (rects1[0], rects2[0]), ('simd', 'omp') )
    autolabel(rects1)
    autolabel(rects2)
    plt.savefig("analysis-comparison.png")
    plt.show()