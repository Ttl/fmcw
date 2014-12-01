import sys
import matplotlib.pyplot as plt
import numpy as np
import math
from scipy import interpolate

def fft(data, sample_rate=1, window=lambda x:np.kaiser(x,2)):
    if window:
        w = window(len(data))
        data = [data[i]*w[i] for i in xrange(len(w))]
    fs = 1/float(sample_rate)
    y = map(abs,np.fft.rfft(data))
    fstep = fs/len(y)
    #return (y,[fstep*i for i in xrange(0,len(y))])
    return y

def log(x):
    if type(x) == list:
        return [20*math.log10(i) if i>0 else -100 for i in x]
    else:
        return 20*math.log10(x)

def normalize(x):
    return [(xi-32768.)/2**12 for xi in x]

def interp(y, kind='cubic', points=500):
    fint = interpolate.interp1d(range(len(y)), y, kind=kind)
    x = np.linspace(0, len(y)-1, points)
    return [fint(xi) for xi in x]

def mcp_to_gain(val):
        """Convert potentiometer value to gain"""
        r1 = 50.0
        r2 = 1500.0
        mcp_range = 10e3
        var_r = (float(val)/255)*mcp_range+r2
        return (1+var_r/r1)

def rms(x):
    """Root mean square of a list"""
    return ((1.0/len(x))*sum(i**2 for i in x))**0.5

def rms_normalize(x):
    m = rms(x)
    return [i/m for i in x]

def adc_to_hz(val):
    #Output frequency at integer vtune
    vals = [5.1,5.4,5.58,5.7,5.81,5.92,6.0,6.1,6.17,6.22,6.29]
    vtune = (float(val)/2**12)*3.3*3
    vint = int(vtune)
    if vint < 0 or vint > 10:
        raise ValueError("Value out of range")
    vfrac = vtune-vint
    return (vals[vint]*(1-vfrac)+vfrac*vals[vint+1])*1e9

tu1_interval = 40e-6
adc_sweep = [2257,2757]

#ADC value to frequency
sweep = map(adc_to_hz,adc_sweep)
sweep_length = tu1_interval*(adc_sweep[1]-adc_sweep[0])
k = (sweep[1]-sweep[0])/(sweep_length)
to_ms = 3e8/(2*k)
print "Sweep frequency {} - {} Hz".format(*sweep)
print "Sweep length {}s".format(sweep_length)

samples = []
gains = []
times = []
with open(sys.argv[1],'r') as f:
    for line in f:
        l = line.split(';')
        times.append(float(l[0]))
        gains.append(int(l[1]))
        d = l[2].split(',')
        d = map(float,d)
        d = map(lambda x : x/mcp_to_gain(gains[-1]), d)
        samples.append(d)

#Clutter reduction
if 1:
    data2 = []
    for e in xrange(1,len(samples)):
        data2.append([samples[e][i]-samples[e-1][i] for i in xrange(min(len(samples[e]),len(samples[e-1])))])

    samples = data2

#Normalize and take FFT
#data = map(normalize, data)
#data = map(rms_normalize, data)
data = map(fft, samples)


min_len = len(data[0])
avg_len = 0
for d in data:
    avg_len += len(d)
    if len(d) < min_len:
        min_len = len(d)

avg_len /= float(len(data))

tsample = (sweep_length)/avg_len
print "Avg sample amount",avg_len

print "Sample rate {} Hz".format(1/tsample)
fstep = 1/(tsample*avg_len)

data = [log(di[:min_len]) for di in data]

#data = map(interp,data)

zz = np.zeros((len(data),len(data[0])))
for e,d in enumerate(data):
    zz[e,:] = np.array(d)

xx, yy = np.meshgrid(
    np.linspace(0,len(data[0])*fstep*to_ms, len(data[0])),
    np.linspace(0,times[-1]-times[0], len(data)))

if 1:
    plt.pcolormesh(xx,yy,zz)
    plt.colorbar()
else:
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    ax.plot_surface(xx,yy,zz)


plt.show()
