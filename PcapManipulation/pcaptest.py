import pyshark 
import matplotlib.pyplot as plt
from datetime import datetime
# Filter packets based on source IP
src_ip = '128.138.158.26'
packets_array = []

def counter(*args):
    packets_array.append(args[0])
 
def count_packets(filename):
    cap = pyshark.FileCapture(filename)
    cap.apply_on_packets(counter, timeout=10000)
    return packets_array

filename = "personaCapture.pcap"
persona_packets = count_packets(filename)

#filename = "stefan1HourCapture.pcapng"
#stefan_packets = count_packets(filename)

timestamps = []
num_packets = 0
for packet in persona_packets:
    times = packet.sniff_time
    timestamps.append(times)
    num_packets +=1

time_duration = timestamps[-1] - timestamps[0]



#print(time_duration)
#Plot the number of packets over time
fig, ax = plt.subplots(figsize=(12, 6))
ax.set_xlabel('Time')
ax.set_ylabel('Packet count')
ax.set_title('Packet count over time')
ax.grid(True, linestyle='--', linewidth=0.5, alpha=0.5)
plt.plot(timestamps, range(len(timestamps)))
plt.xlim(min(timestamps), max(timestamps))
plt.show()



#for packet in packets_array:
    #print(packet)