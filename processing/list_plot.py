import matplotlib.pyplot as plt
import sys

def mcp_to_gain(val):
        """Convert potentiometer value to gain"""
        r1 = 50.0
        r2 = 1500.0
        mcp_range = 10e3
        var_r = (float(val)/255)*mcp_range+r2
        return (1+var_r/r1)

ix = int(sys.argv[2])

data = []
gains = []
times = []
with open(sys.argv[1],'r') as f:
    for line in f:
        l = line.split(';')
        times.append(float(l[0]))
        gains.append(int(l[1]))
        d = l[2].split(',')
        d = map(float,d)
        #d = map(lambda x : x/mcp_to_gain(gains[-1]), d)
        data.append(d)

print "{} sweeps".format(len(data))

plt.plot(range(len(data[ix])),data[ix])
plt.show()
