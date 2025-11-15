import serial
import numpy as np
import matplotlib.pyplot as plt

OVERSAMPLING = 8
SAMPLE_RATE = 40e3 # Hz
CHIRP_RATE = 30e3 # Hz/s

fig_iq = plt.figure()
ax_iq = fig_iq.gca()
ax_iq.set_xlabel('Time in ms')
ax_iq.set_ylabel('Signal Amplitude in V')
ax_iq.set_title('Raw IQ Signals')

fig_fft = plt.figure()
ax_fft = fig_fft.gca()
ax_fft.set_xlabel('Distance in m')
ax_fft.set_ylabel('Return Signal Strength')
ax_fft.set_title('Range Plot')

line_i, = ax_iq.plot([], [])
line_q, = ax_iq.plot([], [])
line_fft, = ax_fft.plot([], [])

plt.ion()
plt.show()

ser = serial.Serial('/dev/ttyACM0', 115200)

while True:
    fig_iq.canvas.start_event_loop(0.05)
    fig_fft.canvas.start_event_loop(0.05)
    
    line = ser.readline().strip()
    if not line.startswith(b'pio count: '):
        continue

    line = ser.readline().strip()
    if not line == b'data i:':
        print('error data i', repr(line))
        continue

    data_i = bytes.fromhex(ser.readline().strip().decode())

    line = ser.readline().strip()
    if not line == b'data q:':
        print('error data q', repr(line))
        continue

    data_q = bytes.fromhex(ser.readline().strip().decode())

    data_i = np.frombuffer(data_i, dtype=np.int16).astype(np.float32) * (3.3 / (1 << 12) / 16) # 8x oversampling, two samples per cycle
    data_q = np.frombuffer(data_q, dtype=np.int16).astype(np.float32) * (3.3 / (1 << 12) / 16) # 
    data_time = np.arange(len(data_i)) * OVERSAMPLING / SAMPLE_RATE

    fft = np.fft.fft(data_i + 1j * data_q)
    fft_freq = np.fft.fftfreq(len(data_i), OVERSAMPLING / SAMPLE_RATE)
    fft_distance = 340 / 2 * fft_freq / CHIRP_RATE
    
    N_FFT = 200
    line_fft.set_data(fft_distance[:N_FFT], np.abs(fft[:N_FFT]))
    ax_fft.set_xlim(0, fft_distance[N_FFT-1])
    ax_fft.set_ylim(0, np.max(np.abs(fft[:N_FFT])))

    line_i.set_data(data_time * 1e3, data_i)
    line_q.set_data(data_time * 1e3, data_q)
    ax_iq.set_xlim(0, data_time[-1] * 1e3)
    ax_iq.set_ylim(-0.1, 0.1)
