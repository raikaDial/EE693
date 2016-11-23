% EE693 Fall 2016
% Team Project: General Purpose Bio-Monitoring System
% This program parses log files and plots the sensor data.

clc;
clear;
close all;

% How many samples per sensor per block
ECG_BLOCKSIZE = 1000;
EMG_BLOCKSIZE = 1000;
ADXL_BLOCKSIZE = 3*1000;
SPO2_BLOCKSIZE = 40;
BLOCKSIZE = 2*ECG_BLOCKSIZE+2*EMG_BLOCKSIZE ...
    + 2*ADXL_BLOCKSIZE+SPO2_BLOCKSIZE;

% Open the log file
[fileName, pathName] = uigetfile('*.bin', 'Select Log File');
if not(fileName)
    error('No file selected.')  
elseif isempty(regexp(fileName, '.bin$', 'once'))
    error('Invalid file selected.');
end

fileID = fopen([pathName, fileName]);
rawData = fread(fileID, 'uint8');
fileName = fileName(1:end-4);
fileDescrip = fileName(1:end-9);
% magicNums = char(rawData(1:5))';
% 
% if not(strcmp(magicNums, 'EE693'))
%     error('Invalid file selected.');
% end
%rawData = rawData(6:end);

numBlocks = floor(length(rawData)/BLOCKSIZE); % Only process whole blocks
if not(numBlocks)
    error('Not enough data in file.');
end

% Pre-allocate buffers
ecgData = zeros(1, ECG_BLOCKSIZE*numBlocks);
emgData = zeros(1, EMG_BLOCKSIZE*numBlocks);
adxlData = zeros(1, ADXL_BLOCKSIZE*numBlocks);
spo2Data = zeros(1, SPO2_BLOCKSIZE*numBlocks);


for n = 0:numBlocks-1
    for i=1:ECG_BLOCKSIZE
        ecgData(i+ECG_BLOCKSIZE*n) = bitor(rawData(2*i+BLOCKSIZE*n-1), ...
            bitshift(rawData(2*i+BLOCKSIZE*n), 8, 'uint16'), 'uint16');
    end
    for i=ECG_BLOCKSIZE+1:ECG_BLOCKSIZE+EMG_BLOCKSIZE
       emgData(i-ECG_BLOCKSIZE+EMG_BLOCKSIZE*n) = bitor( ...
           rawData(2*i+BLOCKSIZE*n-1), ...
           bitshift(rawData(2*i+BLOCKSIZE*n), 8, 'uint16'), 'uint16'); 
    end
    for i=ECG_BLOCKSIZE+EMG_BLOCKSIZE+1:ECG_BLOCKSIZE+...
            EMG_BLOCKSIZE+ADXL_BLOCKSIZE
        adxlData(i-(ECG_BLOCKSIZE+EMG_BLOCKSIZE)+ADXL_BLOCKSIZE*n) = ...
            typecast(uint16(bitor(rawData(2*i+BLOCKSIZE*n-1), ...
            bitshift(rawData(2*i+BLOCKSIZE*n), 8, 'uint16'), 'uint16')),...
            'int16');
    end
    for i=2*(ECG_BLOCKSIZE+EMG_BLOCKSIZE+ADXL_BLOCKSIZE)+1:BLOCKSIZE
        spo2Data(i+SPO2_BLOCKSIZE*n-2*(ECG_BLOCKSIZE+EMG_BLOCKSIZE+...
            ADXL_BLOCKSIZE)) = rawData(i+BLOCKSIZE*n);
    end
end
adxlData = reshape(adxlData,3,[]);

fs = 250;
fs_spo2 = 10;
t = 0:1/fs:(length(ecgData)-1)/fs;
t_spo2 = 0:1/fs_spo2:(length(spo2Data)-1)/fs_spo2;
t_accel = 0:1/fs:(length(adxlData)-1)/fs;
T = 1/fs;                     % Sampling period
L = length(ecgData);                     % Length of signal
NFFT = 2^nextpow2(L);

% Plot ECG Data
figure('Name', ['ECG_',fileDescrip])
plot(t, ecgData);
title('ECG Data')
xlabel('Time (s)')
saveas(gcf,[pathName,fileDescrip,'_ECG'],'png')
% % Plot EMG Data
% figure('Name', 'EMG Data')
% plot(t, emgData);
% title('EMG Data')
% xlabel('Time (s)')


%% Plot Accelerometer Data

maxadxl_scale=max([adxlData(2,:);adxlData(3,:);adxlData(1,:)]);
maxadxl_scale=max(maxadxl_scale(:,:,:));

minadxl_scale=min([adxlData(2,:);adxlData(3,:);adxlData(1,:)]);
minadxl_scale=min(minadxl_scale(:,:,:));

figure('Name', ['Accel_',fileDescrip])
subplot(3,1,1)
plot(t_accel, adxlData(1,:))
axis([0,max(t_accel),minadxl_scale,maxadxl_scale])
title('Accelerometer x-axis')
xlabel('Time (s)')
ylabel('ft^2/s')
subplot(3,1,2)
plot(t_accel, adxlData(2,:))
axis([0,max(t_accel),minadxl_scale,maxadxl_scale])
title('Accelerometer y-axis')
xlabel('Time (s)')
ylabel('ft^2/s')
subplot(3,1,3)
plot(t_accel, adxlData(3,:))
axis([0,max(t_accel),minadxl_scale,maxadxl_scale])
title('Accelerometer z-axis')
xlabel('Time (s)')
ylabel('ft^2/s')
saveas(gcf,[pathName,fileDescrip,'_ADXL'],'png')

% % Plot SPO2 Data
% figure('Name', 'SPO2')
% plot(t_spo2, spo2Data)
% title('SPO2')
% xlabel('Time (s)')
% ylabel('%')


%All high pass frequency values are in Hz.
F1s = 250;  % Sampling Frequency
N  = 2;    % Order
Fc = .05;  % Cutoff Frequency

% Construct an FDESIGN object and call its BUTTER method.
h  = fdesign.highpass('N,F3dB', N, Fc, F1s);
Hd = design(h, 'butter');
%fvtool(Hd)


%filter the ADXL345 and ECG data 
 X(1,:) = filter(Hd,adxlData(1,:));%applies the filters
 Y(1,:) = filter(Hd,adxlData(2,:));  
 Z(1,:) = filter(Hd,adxlData(3,:));  
 ecgData_f= filter(Hd,ecgData);

nVals=fs*(-NFFT/2:NFFT/2-1)/NFFT;%absolute sampling frequency %0:NFFT-1; %DFT Sample points	

%find FFT of original
X_fft = fftshift(fft(double(adxlData(1,:)),NFFT));
Y_fft = fftshift(fft(double(adxlData(2,:)),NFFT));
Z_fft = fftshift(fft(double(adxlData(3,:)),NFFT));
ECG_fft = fftshift(fft(double(ecgData),NFFT));

%find FFT of filtered 
X_fft_f = fftshift(fft(double( X(1,:)),NFFT));
Y_fft_f = fftshift(fft(double( Y(1,:)),NFFT));
Z_fft_f = fftshift(fft(double( Z(1,:)),NFFT));
ECG_fft_f = fftshift(fft(double(ecgData_f),NFFT));


%     %***** MAX FFT 
% max_fft(H+1,1)=max(X_fft);
% max_fft(H+1,2)=max(Y_fft);
% max_fft(H+1,3)=max(Z_fft);
% 
% %***** PLOT AFTER FILTERING
figure('Name', ['Accel_filtered',fileDescrip])
subplot(3,1,1)
plot(t_accel, X)
axis([0,max(t_accel),minadxl_scale,maxadxl_scale])
title('Accelerometer x-axis')
xlabel('Time (s)')
ylabel('ft^2/s')

subplot(3,1,2)
plot(t_accel, Y)
axis([0,max(t_accel),minadxl_scale,maxadxl_scale])
title('Accelerometer y-axis')
xlabel('Time (s)')
ylabel('ft^2/s')

subplot(3,1,3)
plot(t_accel, Z)
axis([0,max(t_accel),minadxl_scale,maxadxl_scale])
title('Accelerometer z-axis')
xlabel('Time (s)')
ylabel('ft^2/s')
saveas(gcf,[pathName,fileDescrip,'_ADXL_Filtered'],'png')


%****FFT after fitlering
figure('Name', ['oRIGINAL FFT',fileDescrip])
subplot(2,2,1), plot(nVals,abs(X_fft./NFFT))
xlabel('X_FFT')
subplot(2,2,2), plot(nVals,abs(Y_fft./NFFT))
xlabel('Y_FFT')
subplot(2,2,3), plot(nVals,abs(Z_fft./NFFT))
xlabel('Z_FFT')
subplot(2,2,4), plot(nVals,abs(ECG_fft./NFFT))
xlabel('ecg_FFT')

figure('Name', ['ORIGINAL FFT',fileDescrip])
plot(nVals,abs(ECG_fft_f./NFFT))
xlabel('ecg_FFT')
figure('Name', ['ORIGINAL FFT',fileDescrip])
plot(nVals,abs(Z_fft_f./NFFT))
xlabel('Z_FFT')


%****FFT after fitlering
figure('Name', ['Filtered FFT',fileDescrip])
subplot(2,2,1), plot(nVals,abs(X_fft_f./NFFT))
xlabel('X_FFT')
subplot(2,2,2), plot(nVals,abs(Y_fft_f./NFFT))
xlabel('Y_FFT')
subplot(2,2,3), plot(nVals,abs(Z_fft_f./NFFT))
xlabel('Z_FFT')
subplot(2,2,4), plot(nVals,abs(ECG_fft_f./NFFT))
xlabel('ecg_FFT')

%**** Plot New ECG with Filtered ECG
figure('Name', ['Original ECG vs Filtered ECG',fileDescrip])
subplot(2,1,1), plot(ecgData)
xlabel('ECG Original')
subplot(2,1,2), plot(ecgData_f)
xlabel('ECG Filtered')
