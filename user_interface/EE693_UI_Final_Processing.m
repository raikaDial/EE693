% EE693 Fall 2016
% Team Project: General Purpose Bio-Monitoring System
% This program parses log files and plots the sensor data.

tic;
clc;
clear;
close all;

% How many samples per sensor per block
ecgMax=0;
ecgThresh=5*10^4;
k=1;%start of file
pointer=0;
ECG_BLOCKSIZE = 1000;
EMG_BLOCKSIZE = 1000;
ADXL_BLOCKSIZE = 3*1000;
SPO2_BLOCKSIZE = 40;
FLAG_BYTES = 1000;
BLOCKSIZE = 4*ECG_BLOCKSIZE+4*EMG_BLOCKSIZE ...
    + 2*ADXL_BLOCKSIZE+SPO2_BLOCKSIZE+FLAG_BYTES;

% Open the log file
[Name, pathName] = uigetfile('*.bin', 'Select Log File');% need to get pathname
BaseName='log';
%EndFileName='_raw.bin';
EndFileName='_filt.bin';
ShortEndName = EndFileName(1:end-4);
%mkdir(pathName,'figures')
fileName=[BaseName,num2str(k),EndFileName]
savelocation=[pathName,'figures',num2str(k)]

while(fileName)
fileID = fopen([pathName, fileName]);
rawData = fread(fileID, 'uint8');
numBlocks = floor(length(rawData)/BLOCKSIZE); % Only process whole blocks

if(numBlocks)
% Pre-allocate buffers
ecgData = zeros(1, ECG_BLOCKSIZE*numBlocks);
emgData = zeros(1, EMG_BLOCKSIZE*numBlocks);
adxlData = zeros(1, ADXL_BLOCKSIZE*numBlocks);
spo2Data = zeros(1, SPO2_BLOCKSIZE*numBlocks);
flagData = zeros(1, FLAG_BYTES*numBlocks);
flaglocation = zeros(1, FLAG_BYTES*numBlocks);

% Parse the raw data
for n = 0:numBlocks-1
    % Parse ECG Data: 4 bytes -> float
    for i=1:ECG_BLOCKSIZE     
        Datatemp3=rawData(i*4-3+(BLOCKSIZE*n));
        Datatemp2=bitshift(rawData(i*4-2+(BLOCKSIZE*n)), 8, 'uint32');
        Datatemp1=bitshift(rawData(i*4-1+(BLOCKSIZE*n)), 16, 'uint32');
        Datatemp0=bitshift(rawData(i*4+(BLOCKSIZE*n)), 24, 'uint32');  
        ecgData(i+ECG_BLOCKSIZE*n) = ...
            typecast( ...
                uint32(bitor( ...
                    uint32(bitor( Datatemp3,Datatemp2,'uint32')), ...
                    uint32(bitor( Datatemp1,Datatemp0,'uint32')), ...
                'uint32' )),...
             'single');  
pointer=4*ECG_BLOCKSIZE;
    end
    % Parse EMG Data: 2 bytes -> uint16
    for i=1:EMG_BLOCKSIZE
        Datatemp3=rawData(i*4-3+(BLOCKSIZE*n)+pointer);
        Datatemp2=bitshift(rawData(i*4-2+(BLOCKSIZE*n)+pointer), 8, 'uint32');
        Datatemp1=bitshift(rawData(i*4-1+(BLOCKSIZE*n)+pointer), 16, 'uint32');
        Datatemp0=bitshift(rawData(i*4+(BLOCKSIZE*n)+pointer), 24, 'uint32');
        
        emgData(i+ECG_BLOCKSIZE*n) = ...
            typecast( ...
                uint32(bitor( ...
                    uint32(bitor( Datatemp3,Datatemp2,'uint32')), ...
                    uint32(bitor( Datatemp1,Datatemp0,'uint32')), ...
                'uint32' )),...
             'single');   
    end
    pointer=4*ECG_BLOCKSIZE+4*EMG_BLOCKSIZE;
    % Parse ADXL Data: 6 bytes -> 3*int16 (x, y, z ft/s^2)
    for i=1:ADXL_BLOCKSIZE
        adxlData(i+ADXL_BLOCKSIZE*n) = ...
            typecast(uint16(bitor(rawData(2*i-1+(BLOCKSIZE*n)+ pointer), ...
                bitshift(rawData(2*i+(BLOCKSIZE*n)+pointer), 8, 'uint16'),...
                'uint16')), ...
                  'int16');
    end

    pointer=4*ECG_BLOCKSIZE+4*EMG_BLOCKSIZE+2*ADXL_BLOCKSIZE;
    % Parse SPO2 Data: 1 byte -> uint8 (%SPO2)
    for i=1:SPO2_BLOCKSIZE
        spo2Data(i+SPO2_BLOCKSIZE*n) = rawData(i+(BLOCKSIZE*n)+ pointer);
    end
     pointer=4*ECG_BLOCKSIZE+4*EMG_BLOCKSIZE+2*ADXL_BLOCKSIZE+SPO2_BLOCKSIZE;
        % Parse Flag Byte
    for i=1:FLAG_BYTES
        flagData(FLAG_BYTES*n+i) = rawData(i+(BLOCKSIZE*n)+pointer);
       % flagData(FLAG_BYTES*n+i)=500;
    end
    pointer=4*ECG_BLOCKSIZE+4*EMG_BLOCKSIZE+2*ADXL_BLOCKSIZE+SPO2_BLOCKSIZE+FLAG_BYTES;
end
adxlData = reshape(adxlData,3,[]);
adxlData=adxlData.*0.00376390;
x=adxlData(1,:);
y=adxlData(2,:);
z=adxlData(3,:);
fs = 250; %set sampling frequency 
fs_spo2 = 10;
t = 0:1/fs:(length(ecgData)-1)/fs;
t_spo2 = 0:1/fs_spo2:(length(spo2Data)-1)/fs_spo2;
t_accel = 0:1/fs:(length(adxlData)-1)/fs;

for n=1:100 % clear first 100 bits of ECG and EMG data due to errors
    ecgData(n) = ecgData(100);
    emgData(n) = emgData(100);
end


%*****interpolate the SPO2 data using the Average***%
for n=4:length(spo2Data)
    if (spo2Data(n)==0)
        spo2Avg=floor((spo2Data(n-1)+spo2Data(n-2)+spo2Data(n-3))/3);
        spo2Data(n)=spo2Avg;        
    end  
end 

% %gets rid of random jumps in the data 
% for n=3:length(ecgData)  
%     if (ecgData(n)> ecgMax)
%         if(ecgData(n)< ecgThresh)
%             ecgmax=ecgData(n);
%         else
%             ecgData(n)=floor((ecgData(n-1)+ecgData(n-2))/2);
%        end
%     end  
% end
%toc

mkdir(savelocation)% make the individual directory for each test batch

% Plot ECG Data

E=figure('Name', 'ECG Data');%,'visible','off')
plot(t, ecgData);
title('ECG Data')
xlabel('Time (s)')
saveas(E,[pathName,'\figures',num2str(k),'\','ECG',ShortEndName],'png')

% Plot EMG Data
M=figure('Name', 'EMG Data');%,'visible','off')
plot(t, emgData);
title('EMG Data')
xlabel('Time (s)')
saveas(M,[pathName,'\figures',num2str(k),'\','EMG',ShortEndName],'png')

%*********Plot Accelerometer Data*******%

%scale accelerometer data 
maxadxl_scale=max([adxlData(2,:);adxlData(3,:);adxlData(1,:)]);
maxadxl_scale=max(maxadxl_scale(:,:,:));
minadxl_scale=min([adxlData(2,:);adxlData(3,:);adxlData(1,:)]);
minadxl_scale=min(minadxl_scale(:,:,:));

A=figure('Name', 'Accel/ECG');%,'visible','off','Position', [100, 100, 1024, 1200]);
subplot(4,1,1)
plot(t_accel, adxlData(1,:))
axis([0,max(t_accel),minadxl_scale,maxadxl_scale+1])
title('Accelerometer x-axis')
xlabel('Time (s)')
ylabel('g')
subplot(4,1,2)
plot(t_accel, adxlData(2,:))
axis([0,max(t_accel),minadxl_scale,maxadxl_scale+1])
title('Accelerometer y-axis')
xlabel('Time (s)')
ylabel('g')
subplot(4,1,3)
plot(t_accel, adxlData(3,:))
axis([0,max(t_accel),minadxl_scale,maxadxl_scale+1])
title('Accelerometer z-axis')
xlabel('Time (s)')
ylabel('g')
subplot(4,1,4)
plot(t, ecgData);
title('ECG Data')
xlabel('Time (s)')
saveas(A,[pathName,'\figures',num2str(k),'\','ALL Axis Motion ECG ',ShortEndName],'png')

A2=figure('Name', 'Accel/Flag/ECG');%,'visible','off','Position', [100, 100, 1024, 1200]);
subplot(3,1,1)
plot(t_accel, adxlData(1,:))
axis([0,max(t_accel),minadxl_scale,maxadxl_scale+1])
title('Accelerometer x-axis')
xlabel('Time (s)')
ylabel('g')
subplot(3,1,2)
plot(t_accel,flagData(1,:))
axis([0,max(t_accel),0,max(flagData)+1])
title('FlagData')
xlabel('Time (s)')
ylabel('ft^2/s')
subplot(3,1,3)
plot(t, ecgData);
title('ECG Data')
xlabel('Time (s)')
saveas(A2,[pathName,'\figures',num2str(k),'\','Motion ECG ',ShortEndName],'png')

% Plot SPO2 Data
s=figure('Name', 'SPO2');%,'visible','off')
plot(t_spo2, spo2Data)
axis([0,max(t_spo2),85,100])
title('SPO2')
xlabel('Time (s)')
ylabel('%')
saveas(s,[pathName,'\figures',num2str(k),'\','SP02',ShortEndName],'png')


end % ends if for empty file and increments to the next 
close all;
clear flagData spo2Data emgData
    k=k+1;
  fileName=[BaseName,num2str(k),EndFileName]
  savelocation=[pathName,'figures',num2str(k)]
toc
% 
% %*******************post processing stuff goes here*******************%
% L = length(ecgData);                     % Length of signal
% NFFT = 2^nextpow2(L);
% 
% X_fft = fftshift(fft(double(adxlData(1,:)),NFFT));
% Y_fft = fftshift(fft(double(adxlData(2,:)),NFFT));
% Z_fft = fftshift(fft(double(adxlData(3,:)),NFFT));
% ECG_fft = fftshift(fft(double(ecgData),NFFT));
% 
% nVals=fs*(-NFFT/2:NFFT/2-1)/NFFT;%absolute sampling frequency %0:NFFT-1; %DFT Sample points	
% 
% OF=figure('Name', ['ORIGINAL FFT'])
% subplot(2,2,1), plot(nVals,abs(X_fft./NFFT))
% xlabel('X_FFT')
% subplot(2,2,2), plot(nVals,abs(Y_fft./NFFT))
% xlabel('Y_FFT')
% subplot(2,2,3), plot(nVals,abs(Z_fft./NFFT))
% xlabel('Z_FFT')
% subplot(2,2,4), plot(nVals,abs(ECG_fft./NFFT))
% xlabel('ecg_FFT')
% saveas(OF,[pathName,'figures',num2str(k-1),'\','Original FFTs',ShortEndName],'png')
% 
% 
% 
% N  = 2;     % Order
% F0 = 2.75;   % Center frequency
% Q  = 0.05;  % Q-factor
% 
% 
% h = fdesign.notch('N,F0,Q', N, F0, Q, fs);
% Hd = design(h, 'butter', ...
%     'SOSScaleNorm', 'Linf');
% Filtered_X=filter(Hd,adxlData(1,:));%applies the filters
% Filtered_Y=filter(Hd,adxlData(2,:));%applies the filters
% Filtered_Z=filter(Hd,adxlData(3,:));%applies the filters
% Filtered_ECG=filter(Hd,ecgData);%applies the filters
% 
% 
% X_filt_fft = fftshift(fft(double(Filtered_X),NFFT));
% Y_filt_fft = fftshift(fft(double(Filtered_Y),NFFT));
% Z_filt_fft = fftshift(fft(double(Filtered_Z),NFFT));
% ECG_filt_fft = fftshift(fft(double(Filtered_ECG),NFFT));
% 
% nVals=fs*(-NFFT/2:NFFT/2-1)/NFFT;%absolute sampling frequency %0:NFFT-1; %DFT Sample points	
% 
% F=figure('Name', ['Filtered FFT'])
% subplot(2,2,1), plot(nVals,abs(X_filt_fft./NFFT))
% xlabel('X_FFT')
% subplot(2,2,2), plot(nVals,abs(Y_filt_fft./NFFT))
% xlabel('Y_FFT')
% subplot(2,2,3), plot(nVals,abs(Z_filt_fft./NFFT))
% xlabel('Z_FFT')
% subplot(2,2,4), plot(nVals,abs(ECG_filt_fft./NFFT))
% xlabel('ecg_FFT')
% saveas(F,[pathName,'\figures',num2str(k-1),'\','Filtered FFT',ShortEndName],'png')
% 
% 
% 
% P=figure('Name', ['Post Filtering Comparison'])
% subplot(2,1,1), plot(ecgData)
% title(['ECG Data', ShortEndName])
% xlabel('Time (s)')
% subplot(2,1,2), plot(Filtered_ECG)
% title(['ECG Data Post Processed ',ShortEndName])
% xlabel('Time (s)')
% saveas(P,[pathName,'\figures',num2str(k-1),'\','Post Filtering Comparison',ShortEndName],'png')

end


