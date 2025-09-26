#
# https://stackoverflow.com/questions/4623572/how-do-i-get-a-list-of-my-devices-audio-sample-rates-using-pyaudio-or-portaudio
#
import pyaudio
import sounddevice as sd

def ScanRates():
    samplerates = 32000, 44100, 48000, 96000, 128000
    device = 0

    supported_samplerates = []
    for fs in samplerates:
        try:
            sd.check_output_settings(device=device, samplerate=fs)
        except Exception as e:
            print(fs, e)
        else:
            supported_samplerates.append(fs)
        print(supported_samplerates)

def CheckInfo():
    devinfo = pyaudio.get_device_info_by_index(1)  # Or whatever device you care about.
    if pyaudio.is_format_supported(44100.0,  # Sample rate
                                   input_device=devinfo['index'],
                                   input_channels=devinfo['maxInputChannels'],
                                   input_format=pyaudio.paInt16):
        print ('Yay!')


CheckInfo()
