import numpy as np
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim

from scipy.io import wavfile
import matplotlib.pyplot as plt
import sounddevice as sd





class NeuralNet(torch.nn.Module):
    def __init__(self, lrate, loss_fn, in_size,out_size):
        super(NeuralNet, self).__init__()
        self.encoder = nn.Sequential(
                                     nn.Conv1d(1,8,kernel_size=16),
                                     nn.ReLU(True))
        self.decoder = nn.Sequential(
                                     nn.ConvTranspose1d(8,1,kernel_size=16),
                                     nn.ReLU(True))
        self.loss = loss_fn
        self.opt = torch.optim.SGD(self.parameters(), lrate)

    def get_parameters(self):
        return self.parameters()

    def forward(self, x):
        x = self.encoder(x)
        x = self.decoder(x)
        return x

    def step(self, x):
        
        x = torch.reshape(x, [x.shape[0], 1, x.shape[1]])
        output = self.forward(x)
        loss = self.loss(output, x)
        loss.backward()
        self.opt.zero_grad()
        self.opt.step()



def play_fft(mags, angles):
    rfft = mags*np.exp(1j*angles)
    data = np.fft.irfft(rfft)
    play_wavelet(data)

def play_wavelet(data):
    data = np.concatenate((data, data))
    data = np.concatenate((data, data))
    data = np.concatenate((data, data))
    data = np.concatenate((data, data))
    data = np.concatenate((data, data))
    data = np.concatenate((data, data))
    data = np.concatenate((data, data))
    sd.play(data, 44100, blocking=True)


len_data = 602
template_name = "AKWF/AKWF_{0:04d}.wav"
wavelet_data = []
train_data = np.zeros((100, len_data))
for i in range(0,10):
    sr, temp_data = wavfile.read(template_name.format(i+1))
    data = temp_data / max(temp_data) #convert from int to float
    
    fft = np.fft.rfft(data)
    mags = np.abs(fft)
    mags /= max(mags)
    angles = np.angle(fft) / np.pi
    temp = np.concatenate((mags, angles))
    wavelet_data.append(data)
    train_data[i] = temp

train_data = torch.tensor(train_data)

lrate = .001
loss = torch.nn.MSELoss()
model = NeuralNet(lrate, loss, 0, 0)
n_iter = 1

model = model.float()

for j in range(n_iter):
    l = model.step(train_data.float())
    print("Epoch Loss:", l)
    

#plt.plot(np.transpose(predicted_fft))
#plt.plot(train_data[0])
plt.show()



