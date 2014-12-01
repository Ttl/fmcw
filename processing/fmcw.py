from datetime import datetime
import time
import math
import sys
import serial
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from scipy import interpolate
from Queue import Queue
from threading import Thread

BAUDRATE = 200

def get_measure(ser, queue):
    while True:
        i = 0
        while ser.inWaiting() == 0 and i < 500:
            time.sleep(0.01)
            i += 1
        if i >= 500:
            raise Exception("Measure failed")
        d = []
        while True:
            c = ser.read(1)
            if c == 'M':
                break
        #Gain
        c_mcp = ord(ser.read(1))

        #Number of points
        c = ser.read(2)
        points = (ord(c[0])<<8)|ord(c[1])
        overflows = 0
        abs_max = 0
        prev_c = 0
        while points > 0:
            if 1:
                if len(d) == 0:
                    c = ser.read(2)
                    uint = (ord(c[0])<<8)|ord(c[1])
                else:
                    c = ser.read(1)
                    #Reassemble int
                    uint = ord(c)
                    #Overflow, reading 16 bits
                    if uint == 128:
                        overflows +=1
                        c = ser.read(2)
                        uint = (ord(c[0])<<8)|ord(c[1])
                    else:
                        if uint > 127:
                            uint = -(256-uint)
                        uint *= 2
                        uint += prev_c
                prev_c = uint
                v = abs(uint-32768)
                if v > abs_max:
                    abs_max = v
                d.append(uint/float(2**16)-0.5)
                points -= 1
            else:
                c = ser.read(2)
                #Reassemble int
                uint = (ord(c[0])<<8)|ord(c[1])
                #d.append(uint)
                d.append(uint/float(2**16)-0.5)
                points -= 1
        queue.put((points,c_mcp,d))


class DopplerAnalogPlot:
    def __init__(self, f, sample_rate, freq, interp=False, interp_points=600, logging=False):
        self.f = f
        self.sample_rate = sample_rate
        self.freq = freq
        self.interp = interp
        self.interp_points = interp_points
        self.logging = logging
        if self.logging != False:
            self.log_file = open(self.logging+datetime.now().strftime('%Y-%m-%dT%H-%M.log'), "w")

    def update(self, frameNum, pt, pf, axt, axf):
        mcp,y = self.f()
        if len(y) == 0:
            y = [0]

        if self.logging:
            self.log_file.write(str(time.time())+';')
            self.log_file.write(str(mcp)+';')
            self.log_file.write(','.join(map(str,y)))
            self.log_file.write('\n')
            self.log_file.flush()

        pt.set_ydata(y)
        pt.set_xdata(range(len(y)))
        axt.set_xlim(0,len(y))

        miny = min(y)
        maxy = max(y)
        miny = 1.1*miny if miny<0 else 0.9*miny
        maxy = 0.9*maxy if maxy<0 else 1.1*maxy
        axt.set_ylim(miny,maxy)

        yf,xf = fft(y, self.sample_rate, np.hamming)

        #Frequency to speed conversion
        xf = [3e8*i/(2*self.freq) for i in xf]

        yf, xf = yf[:100], xf[:100]

        if self.interp and self.interp_points:
            fint = interpolate.interp1d(xf, yf, kind=self.interp)
            xnew = np.linspace(xf[0], xf[-1], self.interp_points)
            ynew = [fint(xi) for xi in xnew]
            pf.set_ydata(ynew)
            pf.set_xdata(xnew)
            axf.set_xlim(0,xnew[-1])
            axf.set_ylim(1,max(ynew))
            m = max(ynew)
            i = ynew.index(m)
            print "Speed",xnew[i],"m/s"
        else:
            pf.set_ydata(yf)
            pf.set_xdata(xf)
            axf.set_xlim(0,xf[-1])
            axf.set_ylim(min(yf),max(yf))
        return (pt,pf,axt,axf)

class FMCWAnalogPlot:
    def __init__(self, f, sample_rate, freq_start, freq_end, sweep_length, interp=False, interp_points=600, logging=False):
        self.f = f
        self.sample_rate = sample_rate
        self.freq_start = freq_start
        self.freq_end = freq_end
        self.sweep_length = sweep_length
        self.interp = interp
        self.interp_points = interp_points
        self.logging = logging
        if self.logging != False:
            self.log_file = open(self.logging+datetime.now().strftime('%Y-%m-%dT%H-%M.log'), "w")

        self.k = (self.freq_end - self.freq_start)/self.sweep_length
        self.t_start = None

    def update(self, frameNum, pt, pf, axt, axf):
        mcp,y = self.f()
        t_stop = time.time()
        if self.t_start != None:
            print "Update rate {} Hz".format(1/(t_stop-self.t_start))
        self.t_start = t_stop
        if len(y) == 0:
            y = [0]


        if self.logging:
            self.log_file.write(str(time.time())+';')
            self.log_file.write(str(mcp)+';')
            self.log_file.write(','.join(map(str,y)))
            self.log_file.write('\n')
            self.log_file.flush()

        #Convert to float
        #y = [yi/float(2**16)-0.5 for yi in y]
        #y = [y[i]-y0[i] for i in xrange(min(len(y),len(y0)))]

        pt.set_ydata(y)
        pt.set_xdata(range(len(y)))
        axt.set_xlim(0,len(y))
        miny = min(y)
        maxy = max(y)
        miny = 1.1*miny if miny<0 else 0.9*miny
        maxy = 0.9*maxy if maxy<0 else 1.1*maxy
        axt.set_ylim(miny, maxy)

        yf,xf = fft(y, self.sample_rate, np.hamming)

        #yf = yf[:100]
        #xf = xf[:100]

        #Frequency to range conversion
        xf = [3e8*i/(2*self.k) for i in xf]

        if self.interp and self.interp_points:
            fint = interpolate.interp1d(xf, yf, kind=self.interp)
            xnew = np.linspace(xf[0], xf[-1], self.interp_points)
            ynew = [20*math.log10(fint(xi)) if fint(xi)>0 else -80 for xi in xnew]
            pf.set_ydata(ynew)
            pf.set_xdata(xnew)
            axf.set_xlim(xnew[0],xnew[-1])
            axf.set_ylim(-80,max(ynew))
            m = max(ynew)
            i = ynew.index(m)
            print "Distance",xnew[i],"m"
        else:
            yf = [20*math.log10(yi) if yi>0 else -80 for yi in yf]
            pf.set_ydata(yf)
            pf.set_xdata(xf)
            axf.set_xlim(0,xf[-1])
            axf.set_ylim(-80,max(yf))
        #print time.time()-t_start
        return (pt,pf,axt,axf)

#Configures serial port
def configure_serial(serial_port):
    return serial.Serial(
        port=serial_port,
        baudrate=BAUDRATE,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_TWO,
        bytesize=serial.EIGHTBITS,
        timeout=0.1
    )

def fft(data, sample_rate, window=None):
    if window:
        w = window(len(data))
        data = [data[i]*w[i] for i in xrange(len(w))]
    fs = 1/float(sample_rate)
    y = map(abs,np.fft.rfft(data))
    fstep = fs/len(y)
    return (y,[fstep*i for i in xrange(0,len(y))])

def write_delay(ser, data, delay):
    for c in data:
        ser.write(c)
        time.sleep(delay)

class Radar():
    def __init__(self, ser):
        self.ser = ser
        self.log = open("fmcw.log","a")
        self.mode = None
        self.m_queue = Queue(2)
        self.m_thread = None


    def pa_on(self):
        ser.write('O')
        return ser.readline() == "PA ON\r\n"

    def pa_off(self):
        ser.write('o')
        return ser.readline() == "PA OFF\r\n"

    def set_gain(self, val):
        if val < 0x00 or val > 0x7F:
            raise ValueError("Invalid potentiometer value")
        write_delay(ser, 'R'+chr(val), 2e-3)
        ret = ser.read(1)
        return ret

    def set_mode(self, mode):
        self.mode = mode
        if mode == "FMCW":
            ser.write('F')
        elif mode == "DOPPLER":
            ser.write('D')
        return False

    def mcp_to_gain(self, val):
        """Convert potentiometer value to gain"""
        r1 = 50.0
        r2 = 1500.0
        mcp_range = 10e3
        var_r = (float(val)/255)*mcp_range+r2
        return (1+var_r/r1)

    def set_sweep_start(self, val):
        write_delay(ser, 's'+chr(val>>8)+chr(val&0xFF), 2e-3)

    def set_sweep_stop(self, val):
        write_delay(ser, 'M'+chr(val>>8)+chr(val&0xFF), 2e-3)

    def meas_toggle(self):
        ser.write('m')
        if self.m_thread == None:
            self.m_thread = Thread(target=get_measure, args=(ser,self.m_queue))
            self.m_thread.start()
        else:
            self.m_thread.join(0)
            self.m_thread = None

    def measure(self):
        if self.m_thread == None:
            self.meas_toggle()
        points, c_mcp, d = self.m_queue.get(True)
        if self.log != None:
            self.log.write(str(points)+':')
            self.log.write(','.join(map(str,d)))
            self.log.write('\n')
        print c_mcp
        return (c_mcp,d)


if __name__ == "__main__":
    device = sys.argv[1]
    try:
        ser = configure_serial(device)
        if not ser.isOpen():
            raise Exception
    except:
        print 'Opening serial port {} failed.'.format(device)
        exit(1)

    try:
        radar = Radar(ser)
        time.sleep(0.1)

        while ser.inWaiting() > 0:
            ser.read(ser.inWaiting())

        if not radar.pa_on():
            print "Enabling PA failed"
            exit(1)
        radar.set_mode("FMCW")
        #radar.set_mode("DOPPLER")

        fig, (axt, axf) = plt.subplots(2)
        pt, = axt.plot([],[])
        pf, = axf.plot([],[])

        radar.meas_toggle()

        if radar.mode == "DOPPLER":
            analogPlot = DopplerAnalogPlot(radar.measure, 4.45e-6, 6e9, None, 1000, logging='doppler')
            anim = animation.FuncAnimation(fig, analogPlot.update,
                                             fargs=(pt,pf,axt,axf),
                                             interval=1)
        else:
            analogPlot = FMCWAnalogPlot(radar.measure, 4e-6, 5.9e9, 6.1e9, 20e-6*(800)/1., None, 1000, logging='fmcw')
            anim = animation.FuncAnimation(fig, analogPlot.update,
                                             fargs=(pt,pf,axt,axf),
                                             interval=0)
        plt.show()

    finally:
        radar.meas_toggle()
        ser.close()
