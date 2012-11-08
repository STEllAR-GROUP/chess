#! /usr/bin/python

num_files=input("Number of log files? ")
input_files=[]
times=[]
for number in range(num_files):
    input_files.append(open("chess_out"+str(number)+".txt"))
    times.append(float(input_files[number].readline()))
output=open("log.txt","w")
while len(input_files)>0:
    min_i=times.index(min(times))
    output.write(str(min_i)+"\n")
    for line in range(8):
        output.write(input_files[min_i].readline())
    next_time=input_files[min_i].readline()
    if next_time=="":
        input_files.pop(min_i)
        times.pop(min_i)
    else:
        times[min_i]=float(next_time)
