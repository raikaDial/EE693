function Hd = getFilter
%GETFILTER Returns a discrete-time filter object.
clear all 
close all
% MATLAB Code
% Generated by MATLAB(R) 9.1 and the DSP System Toolbox 9.3.
% Generated on: 21-Nov-2016 00:06:28

N    = 2;    % Order
F6dB = 70;   % 6-dB Frequency
Fs   = 250;  % Sampling Frequency

h = fdesign.lowpass('n,fc', N, F6dB, Fs);

Hd = design(h, 'window');
[B,A]=tf(Hd)
